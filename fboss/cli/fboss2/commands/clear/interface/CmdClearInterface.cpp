// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/cli/fboss2/commands/clear/interface/CmdClearInterface.h"
#include "fboss/cli/fboss2/CmdHandler.cpp"

namespace facebook::fboss {

CmdClearInterface::RetType CmdClearInterface::queryClient(
    const HostInfo& /* hostInfo */,
    const std::vector<std::string>& /* queriedIfs */) {
  throw std::runtime_error(
      "Incomplete command, please use one the subcommands");
}

void CmdClearInterface::printOutput(const RetType& /* model */) {}

// Explicit template instantiation
template void CmdHandler<CmdClearInterface, CmdClearInterfaceTraits>::run();
template const ValidFilterMapType
CmdHandler<CmdClearInterface, CmdClearInterfaceTraits>::getValidFilters();

} // namespace facebook::fboss
