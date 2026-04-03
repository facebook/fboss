// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/cli/fboss2/commands/show/interface/prbs/CmdShowInterfacePrbs.h"
#include "fboss/cli/fboss2/CmdHandler.cpp"

namespace facebook::fboss {

CmdShowInterfacePrbsTraits::RetType CmdShowInterfacePrbs::queryClient(
    const HostInfo& /* hostInfo */,
    const std::vector<std::string>& /* queriedIfs */,
    const ObjectArgType& /* components */) {
  throw std::runtime_error(
      "Incomplete command, please use one the subcommands");
}

void CmdShowInterfacePrbs::printOutput(const RetType& /* model */) {}

// Template instantiations
template void
CmdHandler<CmdShowInterfacePrbs, CmdShowInterfacePrbsTraits>::run();
template const ValidFilterMapType
CmdHandler<CmdShowInterfacePrbs, CmdShowInterfacePrbsTraits>::getValidFilters();

} // namespace facebook::fboss
