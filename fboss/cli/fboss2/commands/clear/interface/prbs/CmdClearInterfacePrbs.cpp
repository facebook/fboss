// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/cli/fboss2/commands/clear/interface/prbs/CmdClearInterfacePrbs.h"
#include "fboss/cli/fboss2/CmdHandler.cpp"

namespace facebook::fboss {

CmdClearInterfacePrbs::RetType CmdClearInterfacePrbs::queryClient(
    const HostInfo& /* hostInfo */,
    const std::vector<std::string>& /* queriedIfs */,
    const ObjectArgType& /* components */) {
  throw std::runtime_error(
      "Incomplete command, please use one the subcommands");
}

void CmdClearInterfacePrbs::printOutput(const RetType& /* model */) {}

// Explicit template instantiation
template void
CmdHandler<CmdClearInterfacePrbs, CmdClearInterfacePrbsTraits>::run();
template const ValidFilterMapType CmdHandler<
    CmdClearInterfacePrbs,
    CmdClearInterfacePrbsTraits>::getValidFilters();

} // namespace facebook::fboss
