// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/cli/fboss2/commands/show/dsf/CmdShowDsf.h"
#include "fboss/cli/fboss2/CmdHandler.cpp"

namespace facebook::fboss {

CmdShowDsf::RetType CmdShowDsf::queryClient(const HostInfo& /* hostInfo */) {
  throw std::runtime_error(
      "Incomplete command, please use one the subcommands");
}

void CmdShowDsf::printOutput(const RetType& /* model */) {}

// Template instantiations
template void CmdHandler<CmdShowDsf, CmdShowDsfTraits>::run();
template const ValidFilterMapType
CmdHandler<CmdShowDsf, CmdShowDsfTraits>::getValidFilters();

} // namespace facebook::fboss
