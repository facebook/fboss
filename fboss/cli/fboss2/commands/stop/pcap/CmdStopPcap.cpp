// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/cli/fboss2/commands/stop/pcap/CmdStopPcap.h"
#include "fboss/cli/fboss2/CmdHandler.cpp"

#include "fboss/agent/if/gen-cpp2/FbossCtrl.h"
#include "fboss/cli/fboss2/CmdLocalOptions.h"

namespace facebook::fboss {

CmdStopPcap::RetType CmdStopPcap::queryClient(const HostInfo& hostInfo) {
  std::string nameStr;
  auto name =
      CmdLocalOptions::getInstance()->getLocalOption("stop_pcap", "--name");
  nameStr = name.empty() ? "packet_capture" : name;
  auto client =
      utils::createClient<apache::thrift::Client<FbossCtrl>>(hostInfo);
  client->sync_stopPktCapture(nameStr);
  return fmt::format("Stopping packet capture \"{}\"", nameStr);
}

void CmdStopPcap::printOutput(const RetType& captureOutput, std::ostream& out) {
  out << captureOutput << std::endl;
}

// Explicit template instantiation
template void CmdHandler<CmdStopPcap, CmdStopPcapTraits>::run();

} // namespace facebook::fboss
