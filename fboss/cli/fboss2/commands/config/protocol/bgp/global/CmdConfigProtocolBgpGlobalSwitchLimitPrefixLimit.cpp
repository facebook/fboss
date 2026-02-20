/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/protocol/bgp/global/CmdConfigProtocolBgpGlobalSwitchLimitPrefixLimit.h"

#include <fmt/core.h>
#include "fboss/cli/fboss2/commands/config/protocol/bgp/BgpConfigSession.h"

namespace facebook::fboss {

CmdConfigProtocolBgpGlobalSwitchLimitPrefixLimitTraits::RetType
CmdConfigProtocolBgpGlobalSwitchLimitPrefixLimit::queryClient(
    const HostInfo& /* hostInfo */,
    const ObjectArgType& limitStr) {
  auto& session = BgpConfigSession::getInstance();

  // Command: "config protocol bgp global switch-limit <value>"
  // After "switch-limit", args contains just ["<value>"]
  const auto& tokens = limitStr.data();
  if (tokens.empty()) {
    return "Error: prefix-limit value is required. Usage: switch-limit <value>";
  }

  int64_t limit = std::stoll(tokens[0]);

  session.setSwitchLimitPrefixLimit(limit);
  session.saveConfig();

  return fmt::format(
      "Successfully set switch_limit_config prefix_limit to: {}\nConfig saved to: {}",
      limit,
      session.getSessionConfigPath());
}

void CmdConfigProtocolBgpGlobalSwitchLimitPrefixLimit::printOutput(
    const RetType& output) {
  std::cout << output << std::endl;
}

} // namespace facebook::fboss
