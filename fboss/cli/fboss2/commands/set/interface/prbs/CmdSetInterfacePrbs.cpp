// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/cli/fboss2/commands/set/interface/prbs/CmdSetInterfacePrbs.h"
#include "fboss/cli/fboss2/CmdHandler.cpp"

namespace facebook::fboss {

CmdSetInterfacePrbs::RetType CmdSetInterfacePrbs::queryClient(
    const HostInfo& /* hostInfo */,
    const utils::PortList& /* queriedIfs */,
    const ObjectArgType& /* components */) {
  throw std::runtime_error(
      "Incomplete command, please use one the subcommands");
}

void CmdSetInterfacePrbs::printOutput(const RetType& /* model */) {}

// Explicit template instantiation
template void CmdHandler<CmdSetInterfacePrbs, CmdSetInterfacePrbsTraits>::run();
template const ValidFilterMapType
CmdHandler<CmdSetInterfacePrbs, CmdSetInterfacePrbsTraits>::getValidFilters();

} // namespace facebook::fboss
