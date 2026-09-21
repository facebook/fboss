// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/cli/fboss2/commands/set/transceiver/CmdSetTransceiver.h"
#include "fboss/cli/fboss2/CmdHandler.cpp"

namespace facebook::fboss {

CmdSetTransceiver::RetType CmdSetTransceiver::queryClient(
    const HostInfo& /* hostInfo */,
    const ObjectArgType& /* queriedPorts */) {
  return "Usage: set transceiver <port> loopback <system|line> <enable|disable>\n"
         "       set transceiver <port> loopback disable\n";
}

void CmdSetTransceiver::printOutput(const RetType& output, std::ostream& out) {
  out << output;
}

template void CmdHandler<CmdSetTransceiver, CmdSetTransceiverTraits>::run();

} // namespace facebook::fboss
