/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/interface/switchport/trunk/CmdConfigInterfaceSwitchportTrunk.h"

#include "fboss/cli/fboss2/CmdHandler.cpp"

namespace facebook::fboss {

CmdConfigInterfaceSwitchportTrunkTraits::RetType
CmdConfigInterfaceSwitchportTrunk::queryClient(
    const HostInfo& /* hostInfo */,
    const utils::InterfaceList& interfaces) {
  if (interfaces.empty()) {
    throw std::invalid_argument("No interface name provided");
  }
  throw std::runtime_error(
      "Incomplete command, please use one of the subcommands");
}

void CmdConfigInterfaceSwitchportTrunk::printOutput(
    const CmdConfigInterfaceSwitchportTrunkTraits::RetType& /* model */) {}

// Explicit template instantiation
template void CmdHandler<
    CmdConfigInterfaceSwitchportTrunk,
    CmdConfigInterfaceSwitchportTrunkTraits>::run();

} // namespace facebook::fboss
