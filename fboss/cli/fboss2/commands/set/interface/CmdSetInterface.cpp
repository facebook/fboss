// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/cli/fboss2/commands/set/interface/CmdSetInterface.h"
#include "fboss/cli/fboss2/CmdHandler.cpp"

namespace facebook::fboss {

CmdSetInterface::RetType CmdSetInterface::queryClient(
    const HostInfo& /* hostInfo */,
    const std::vector<std::string>& /* queriedIfs */) {
  throw std::runtime_error(
      "Incomplete command, please use one the subcommands");
}

void CmdSetInterface::printOutput(const RetType& /* model */) {}

// Explicit template instantiation
template void CmdHandler<CmdSetInterface, CmdSetInterfaceTraits>::run();
template const ValidFilterMapType
CmdHandler<CmdSetInterface, CmdSetInterfaceTraits>::getValidFilters();

} // namespace facebook::fboss
