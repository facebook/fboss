// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/cli/fboss2/utils/LoopbackUtils.h"

#include "fboss/agent/FbossError.h"
#include "fboss/agent/if/gen-cpp2/FbossCtrl.h"
#include "fboss/cli/fboss2/utils/PrbsUtils.h"
#include "fboss/qsfp_service/if/gen-cpp2/QsfpService.h"
#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"

#include <boost/algorithm/string.hpp>
#include <fmt/format.h>
#include <folly/MapUtil.h>

namespace facebook::fboss::loopback_utils {

LoopbackComponentAction::LoopbackComponentAction(std::vector<std::string> v)
    : BaseObjectArgType(v) {
  if (v.size() != 2) {
    throw FbossError(
        "Incomplete command, expecting 'loopback <asic|xphy_system|xphy_line|"
        "transceiver_system|transceiver_line> <enable|disable>'");
  }
  componentName_ = boost::to_lower_copy(v[0]);
  // Throws "Unsupported component: <x>" for anything outside the shared list.
  auto components =
      prbsComponents({componentName_}, false /* returnAllIfEmpty */);
  if (components.size() != 1) {
    throw FbossError("Unknown component '", v[0], "'");
  }
  component_ = components[0];

  const auto action = boost::to_lower_copy(v[1]);
  if (action == kActionEnable) {
    enable_ = true;
  } else if (action == kActionDisable) {
    enable_ = false;
  } else {
    throw FbossError(
        "Expected '",
        kActionEnable,
        "' or '",
        kActionDisable,
        "', got '",
        v[1],
        "'");
  }
}

std::map<int32_t, PortInfoThrift> fetchAllPortInfo(
    apache::thrift::Client<FbossCtrl>* agent) {
  std::map<int32_t, PortInfoThrift> portEntries;
  agent->sync_getAllPortInfo(portEntries);
  return portEntries;
}

int32_t resolveTransceiverId(
    apache::thrift::Client<FbossCtrl>* agent,
    const std::string& portName,
    const std::map<int32_t, PortInfoThrift>& portEntries) {
  int32_t portId = -1;
  for (const auto& [id, info] : portEntries) {
    if (*info.name() == portName) {
      portId = id;
      break;
    }
  }
  if (portId < 0) {
    throw FbossError("Port '", portName, "' not found");
  }

  std::map<int32_t, PortStatus> portStatusEntries;
  std::vector<int32_t> portIds = {portId};
  agent->sync_getPortStatus(portStatusEntries, portIds);

  auto* statusPtr = folly::get_ptr(portStatusEntries, portId);
  if (!statusPtr || !statusPtr->transceiverIdx().has_value() ||
      !statusPtr->transceiverIdx()->transceiverId().has_value()) {
    throw FbossError("No transceiver found for port '", portName, "'");
  }
  return *statusPtr->transceiverIdx()->transceiverId();
}

LoopbackCapability fetchLoopbackCapability(
    apache::thrift::Client<QsfpService>* qsfpService,
    int32_t transceiverId) {
  std::map<int32_t, TransceiverInfo> transceiverEntries;
  qsfpService->sync_getTransceiverInfo(
      transceiverEntries, std::vector<int32_t>{transceiverId});

  auto* tcvrInfo = folly::get_ptr(transceiverEntries, transceiverId);
  if (!tcvrInfo) {
    throw FbossError("No transceiver info for transceiver ID ", transceiverId);
  }

  const auto& diags = *tcvrInfo->tcvrState()->diagCapability();
  return LoopbackCapability{
      .capSystem = *diags.loopbackSystem(),
      .capLine = *diags.loopbackLine(),
  };
}

uint8_t readOneByte(
    apache::thrift::Client<QsfpService>* qsfpService,
    int32_t transceiverId,
    int page,
    int offset) {
  ReadRequest req;
  TransceiverIOParameters param;
  req.ids() = {transceiverId};
  param.offset() = offset;
  param.length() = 1;
  if (page >= 0) {
    param.page() = page;
  }
  req.parameter() = param;

  std::map<int32_t, ReadResponse> resp;
  qsfpService->sync_readTransceiverRegister(resp, req);
  auto* respPtr = folly::get_ptr(resp, transceiverId);
  if (!respPtr || respPtr->data()->length() < 1) {
    throw FbossError(
        "Failed to read byte at page 0x",
        fmt::format("{:02X}", page),
        " offset ",
        offset);
  }
  return respPtr->data()->data()[0];
}

std::string formatState(uint8_t mediaNear, uint8_t mediaFar) {
  std::string out;
  out += fmt::format(
      "  system (media-far):   0x{:02X}  {}\n",
      mediaFar,
      mediaFar ? "enabled" : "disabled");
  out += fmt::format(
      "  line (media-near):    0x{:02X}  {}\n",
      mediaNear,
      mediaNear ? "enabled" : "disabled");
  return out;
}

std::string setTransceiverLoopbackForPort(
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

} // namespace facebook::fboss::loopback_utils
