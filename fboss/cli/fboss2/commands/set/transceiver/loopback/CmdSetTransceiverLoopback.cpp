// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/cli/fboss2/commands/set/transceiver/loopback/CmdSetTransceiverLoopback.h"
#include "fboss/cli/fboss2/CmdHandler.cpp"
#include "fboss/cli/fboss2/utils/LoopbackUtils.h"

#include "fboss/agent/if/gen-cpp2/FbossCtrl.h"
#include "fboss/lib/phy/gen-cpp2/phy_types.h"
#include "fboss/qsfp_service/if/gen-cpp2/QsfpService.h"

#include <fmt/format.h>

namespace facebook::fboss {

using namespace loopback_utils;

namespace {

std::string setLoopbackForPort(
    apache::thrift::Client<FbossCtrl>* agent,
    apache::thrift::Client<QsfpService>* qsfpService,
    const std::string& portName,
    const LoopbackAction& action,
    const std::map<int32_t, PortInfoThrift>& portEntries) {
  int32_t transceiverId = resolveTransceiverId(agent, portName, portEntries);
  auto cap = fetchLoopbackCapability(qsfpService, transceiverId);
  bool capSystem = cap.capSystem;
  bool capLine = cap.capLine;

  if (action.isDisableAll()) {
    if (!capSystem && !capLine) {
      return fmt::format(
          "Port: {}\nLoopback not supported by this module, nothing to disable.\n",
          portName);
    }
  } else {
    if (action.mode() == kModeSystem && !capSystem) {
      return fmt::format(
          "Error ({}): system (media-far) loopback not supported by this module\n",
          portName);
    }
    if (action.mode() == kModeLine && !capLine) {
      return fmt::format(
          "Error ({}): line (media-near) loopback not supported by this module\n",
          portName);
    }
  }

  std::string output;
  output += fmt::format("Port: {}\n", portName);
  output += fmt::format("Transceiver ID: {}\n", transceiverId);

  // Read before state
  uint8_t beforeNear = 0;
  uint8_t beforeFar = 0;
  try {
    beforeNear = readOneByte(
        qsfpService, transceiverId, kLoopbackPage, kMediaNearLbEnOffset);
    beforeFar = readOneByte(
        qsfpService, transceiverId, kLoopbackPage, kMediaFarLbEnOffset);
  } catch (const std::exception& ex) {
    output += fmt::format("Error reading before-state: {}\n", ex.what());
  }

  if (action.isDisableAll()) {
    output += "Action: disable-all\n";
    std::vector<std::pair<phy::PortComponent, std::string_view>> components;
    if (capSystem) {
      components.emplace_back(
          phy::PortComponent::TRANSCEIVER_SYSTEM, kModeSystem);
    }
    if (capLine) {
      components.emplace_back(phy::PortComponent::TRANSCEIVER_LINE, kModeLine);
    }
    int failCount = 0;
    for (const auto& [comp, modeName] : components) {
      try {
        qsfpService->sync_setPortLoopbackState(portName, comp, false);
      } catch (const std::exception& ex) {
        output += fmt::format("Error disabling {}: {}\n", modeName, ex.what());
        ++failCount;
      }
    }
    auto resultStr = failCount == 0                       ? "success"
        : failCount < static_cast<int>(components.size()) ? "partial-failure"
                                                          : "failure";
    output += fmt::format("Result: {}\n", resultStr);
  } else {
    phy::PortComponent component = (action.mode() == kModeSystem)
        ? phy::PortComponent::TRANSCEIVER_SYSTEM
        : phy::PortComponent::TRANSCEIVER_LINE;

    output += fmt::format("Mode: {}\n", action.mode());
    output += fmt::format(
        "Action: {}\n", action.enable() ? kActionEnable : kActionDisable);

    try {
      qsfpService->sync_setPortLoopbackState(
          portName, component, action.enable());
      output += "Result: success\n";
    } catch (const std::exception& ex) {
      output += "Result: error\n";
      output += fmt::format("Reason: {}\n", ex.what());
      return output;
    }
  }

  output += "\nBefore:\n";
  output += formatState(beforeNear, beforeFar);

  // Read after state
  try {
    uint8_t afterNear = readOneByte(
        qsfpService, transceiverId, kLoopbackPage, kMediaNearLbEnOffset);
    uint8_t afterFar = readOneByte(
        qsfpService, transceiverId, kLoopbackPage, kMediaFarLbEnOffset);
    output += "\nAfter:\n";
    output += formatState(afterNear, afterFar);
  } catch (const std::exception& ex) {
    output += fmt::format("\nError reading after-state: {}\n", ex.what());
  }

  return output;
}

} // namespace

CmdSetTransceiverLoopback::RetType CmdSetTransceiverLoopback::queryClient(
    const HostInfo& hostInfo,
    const utils::PortList& queriedPorts,
    const ObjectArgType& action) {
  if (queriedPorts.empty()) {
    return "Error: at least one port must be specified\n"
           "Usage: set transceiver <port> loopback <system|line> <enable|disable>\n"
           "       set transceiver <port> loopback disable\n";
  }

  auto agent = utils::createClient<apache::thrift::Client<FbossCtrl>>(hostInfo);
  auto qsfpService =
      utils::createClient<apache::thrift::Client<QsfpService>>(hostInfo);
  auto portEntries = fetchAllPortInfo(agent.get());

  std::string output;
  for (const auto& portName : queriedPorts) {
    try {
      output += setLoopbackForPort(
          agent.get(), qsfpService.get(), portName, action, portEntries);
      if (queriedPorts.size() > 1) {
        output += "\n";
      }
    } catch (const std::exception& ex) {
      output += fmt::format("Error ({}): {}\n", portName, ex.what());
    }
  }

  return output;
}

void CmdSetTransceiverLoopback::printOutput(
    const RetType& output,
    std::ostream& out) {
  out << output;
}

template void
CmdHandler<CmdSetTransceiverLoopback, CmdSetTransceiverLoopbackTraits>::run();

} // namespace facebook::fboss
