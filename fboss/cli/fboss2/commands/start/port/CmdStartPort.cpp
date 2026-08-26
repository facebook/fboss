// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/cli/fboss2/commands/start/port/CmdStartPort.h"
#include "fboss/cli/fboss2/CmdHandler.cpp" // NOLINT(facebook-unused-include-check)

#include <stdexcept>

namespace facebook::fboss {

CmdStartPort::RetType CmdStartPort::queryClient(
    const HostInfo& /* hostInfo */,
    const ObjectArgType& /* ports */) {
  throw std::runtime_error(
      "Incomplete command, please use one of the subcommands");
}

void CmdStartPort::printOutput(const RetType& /* model */) {}

// Explicit template instantiation
template void CmdHandler<CmdStartPort, CmdStartPortTraits>::run();
template const ValidFilterMapType
CmdHandler<CmdStartPort, CmdStartPortTraits>::getValidFilters();

} // namespace facebook::fboss
