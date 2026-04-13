// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/cli/fboss2/commands/get/pcap/CmdGetPcap.h"
#include "fboss/cli/fboss2/CmdHandler.cpp"

#include "fboss/cli/fboss2/CmdLocalOptions.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"

namespace facebook::fboss {

CmdGetPcap::RetType CmdGetPcap::queryClient(const HostInfo& hostInfo) {
  std::string nameStr;
  auto name =
      CmdLocalOptions::getInstance()->getLocalOption("get_pcap", "--name");
  nameStr = name.empty() ? "packet_capture.pcap" : fmt::format("{}.pcap", name);
  std::string remoteStr = fmt::format(
      "netops@{}:/var/facebook/fboss/captures/{}", hostInfo.getName(), nameStr);
  std::string cmdStr = fmt::format("scp {} .", remoteStr);
  utils::runCmd(cmdStr);
  return fmt::format(
      "Getting a copy of the packet capture \"{}\" from {}...",
      nameStr,
      hostInfo.getName());
}

void CmdGetPcap::printOutput(const RetType& captureOutput, std::ostream& out) {
  out << captureOutput << std::endl;
}

// Explicit template instantiation
template void CmdHandler<CmdGetPcap, CmdGetPcapTraits>::run();

} // namespace facebook::fboss
