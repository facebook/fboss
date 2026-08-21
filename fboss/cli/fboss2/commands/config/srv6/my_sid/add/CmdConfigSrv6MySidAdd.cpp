/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/srv6/my_sid/add/CmdConfigSrv6MySidAdd.h"

#include "fboss/cli/fboss2/CmdHandler.cpp"

#include <fmt/format.h>
#include <iostream>
#include "fboss/cli/fboss2/session/ConfigSession.h"

namespace facebook::fboss {

// Validates prefix, then upserts mySidConfig.entries[function].
CmdConfigSrv6MySidAddTraits::RetType CmdConfigSrv6MySidAdd::queryClient(
    const HostInfo& /* hostInfo */,
    const LocatorPrefixArg& prefixArg,
    const ObjectArgType& addArg) {
  auto& session = ConfigSession::getInstance();
  auto& swConfig = *session.getAgentConfig().sw();

  auto& mySidConfig = requireMySidConfig(swConfig);
  requireMatchingLocatorPrefix(mySidConfig, prefixArg.getPrefix());

  auto entry = addArg.buildEntryConfig();
  auto functionValue = addArg.getFunctionValue();
  auto& entries = *mySidConfig.entries();

  auto it = entries.find(functionValue);
  if (it != entries.end()) {
    if (it->second == entry) {
      return fmt::format(
          "MySID entry {} already set, no change", functionValue);
    }
    it->second = entry;
    session.saveConfig(
        cli::ServiceType::AGENT, cli::ConfigActionLevel::HITLESS);
    return fmt::format(
        "Warning: MySID entry {} overwritten with new {} SID",
        functionValue,
        addArg.getTypeStr());
  }

  entries[functionValue] = entry;
  session.saveConfig(cli::ServiceType::AGENT, cli::ConfigActionLevel::HITLESS);
  return fmt::format(
      "Successfully added {} SID: entry={}",
      addArg.getTypeStr(),
      functionValue);
}

void CmdConfigSrv6MySidAdd::printOutput(const RetType& output) {
  std::cout << output << std::endl;
}

template void
CmdHandler<CmdConfigSrv6MySidAdd, CmdConfigSrv6MySidAddTraits>::run();

} // namespace facebook::fboss
