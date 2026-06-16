// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/cli/fboss2/commands/show/transceiver/loopback/CmdShowTransceiverLoopback.h"
#include "fboss/cli/fboss2/CmdHandler.cpp"
#include "fboss/cli/fboss2/utils/LoopbackUtils.h"

#include "fboss/agent/if/gen-cpp2/FbossCtrl.h"
#include "fboss/qsfp_service/if/gen-cpp2/QsfpService.h"

#include <fmt/format.h>

namespace facebook::fboss {

using namespace loopback_utils;

namespace {

std::string showLoopbackForPort(
    apache::thrift::Client<FbossCtrl>* agent,
    apache::thrift::Client<QsfpService>* qsfpService,
    const std::string& portName,
    const std::map<int32_t, PortInfoThrift>& portEntries) {
  int32_t transceiverId = resolveTransceiverId(agent, portName, portEntries);
  auto cap = fetchLoopbackCapability(qsfpService, transceiverId);

  std::string output;
  output += fmt::format("Port: {}\n", portName);
  output += fmt::format("Transceiver ID: {}\n", transceiverId);

  bool capSystem = cap.capSystem;
  bool capLine = cap.capLine;

  output += "\nCapability:\n";
  output += fmt::format(
      "  system (media-far):   {}\n",
      capSystem ? "supported" : "not-supported");
  output += fmt::format(
      "  line (media-near):    {}\n", capLine ? "supported" : "not-supported");

  if (!capLine && !capSystem) {
    output += "\nLoopback not supported by this module.\n";
    return output;
  }

  try {
    uint8_t mediaNear = readOneByte(
        qsfpService, transceiverId, kLoopbackPage, kMediaNearLbEnOffset);
    uint8_t mediaFar = readOneByte(
        qsfpService, transceiverId, kLoopbackPage, kMediaFarLbEnOffset);

    output += "\nState:\n";
    output += formatState(mediaNear, mediaFar);
  } catch (const std::exception& ex) {
    output += fmt::format("\nError reading state: {}\n", ex.what());
  }

  return output;
}

} // namespace

CmdShowTransceiverLoopback::RetType CmdShowTransceiverLoopback::queryClient(
    const HostInfo& hostInfo,
    const utils::PortList& queriedPorts) {
  if (queriedPorts.empty()) {
    return "Usage: show transceiver <port> loopback\n"
           "\nShows loopback capability and current state.\n"
           "\nExample:\n"
           "  show transceiver eth1/25/1 loopback\n";
  }

  auto agent = utils::createClient<apache::thrift::Client<FbossCtrl>>(hostInfo);
  auto qsfpService =
      utils::createClient<apache::thrift::Client<QsfpService>>(hostInfo);
  auto portEntries = fetchAllPortInfo(agent.get());

  std::string output;
  for (const auto& portName : queriedPorts) {
    try {
      output += showLoopbackForPort(
          agent.get(), qsfpService.get(), portName, portEntries);
      if (queriedPorts.size() > 1) {
        output += "\n";
      }
    } catch (const std::exception& ex) {
      output += fmt::format("Error ({}): {}\n", portName, ex.what());
    }
  }

  return output;
}

void CmdShowTransceiverLoopback::printOutput(
    const RetType& output,
    std::ostream& out) {
  out << output;
}

template void
CmdHandler<CmdShowTransceiverLoopback, CmdShowTransceiverLoopbackTraits>::run();

} // namespace facebook::fboss
