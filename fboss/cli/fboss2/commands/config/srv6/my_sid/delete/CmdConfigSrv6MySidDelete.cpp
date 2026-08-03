/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/srv6/my_sid/delete/CmdConfigSrv6MySidDelete.h"

#include "fboss/cli/fboss2/CmdHandler.cpp"

#include <fmt/format.h>
#include <iostream>
#include "fboss/cli/fboss2/session/ConfigSession.h"

namespace facebook::fboss {

// Validates prefix, then erases one function entry (idempotent if missing).
CmdConfigSrv6MySidDeleteTraits::RetType CmdConfigSrv6MySidDelete::queryClient(
    const HostInfo& /* hostInfo */,
    const LocatorPrefixArg& prefixArg,
    const ObjectArgType& deleteArg) {
  auto& session = ConfigSession::getInstance();
  auto& swConfig = *session.getAgentConfig().sw();

  if (!hasMySidConfig(swConfig)) {
    return fmt::format(
        "MySID entry {} does not exist (no mySidConfig configured)",
        deleteArg.getFunctionValue());
  }

  auto& mySidConfig = *swConfig.mySidConfig();
  requireMatchingLocatorPrefix(mySidConfig, prefixArg.getPrefix());

  auto& entries = ensureMySidEntries(mySidConfig);
  auto functionValue = deleteArg.getFunctionValue();
  auto it = entries.find(functionValue);
  if (it == entries.end()) {
    return fmt::format("MySID entry {} does not exist", functionValue);
  }

  entries.erase(it);
  session.saveConfig(cli::ServiceType::AGENT, cli::ConfigActionLevel::HITLESS);
  return fmt::format("Successfully deleted MySID entry {}", functionValue);
}

void CmdConfigSrv6MySidDelete::printOutput(const RetType& output) {
  std::cout << output << std::endl;
}

template void
CmdHandler<CmdConfigSrv6MySidDelete, CmdConfigSrv6MySidDeleteTraits>::run();

} // namespace facebook::fboss
