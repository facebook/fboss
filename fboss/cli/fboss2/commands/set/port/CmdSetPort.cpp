// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/cli/fboss2/commands/set/port/CmdSetPort.h"
#include "fboss/cli/fboss2/CmdHandler.cpp"

namespace facebook::fboss {

CmdSetPort::RetType CmdSetPort::queryClient(
    const HostInfo& /* hostInfo */,
    const ObjectArgType& /* queriedPortIds */) {
  throw std::runtime_error(
      "Incomplete command, please use one the subcommands");
}

void CmdSetPort::printOutput(const RetType& /* model */) {}

// Explicit template instantiation
template void CmdHandler<CmdSetPort, CmdSetPortTraits>::run();
template const ValidFilterMapType
CmdHandler<CmdSetPort, CmdSetPortTraits>::getValidFilters();

} // namespace facebook::fboss
