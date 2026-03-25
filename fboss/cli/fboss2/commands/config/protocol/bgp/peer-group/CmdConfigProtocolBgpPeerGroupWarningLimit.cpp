/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/protocol/bgp/peer-group/CmdConfigProtocolBgpPeerGroupWarningLimit.h"

#include <fmt/core.h>
#include "fboss/cli/fboss2/commands/config/protocol/bgp/BgpConfigSession.h"

namespace facebook::fboss {

CmdConfigProtocolBgpPeerGroupWarningLimitTraits::RetType
CmdConfigProtocolBgpPeerGroupWarningLimit::queryClient(
    const HostInfo& /* hostInfo */,
    const utils::Message& groupNameArg,
    const ObjectArgType& warningLimitMsg) {
  auto& session = BgpConfigSession::getInstance();

  if (groupNameArg.data().empty()) {
    return "Error: peer-group name is required";
  }
  std::string groupName = groupNameArg.data()[0];

  if (warningLimitMsg.data().empty()) {
    return fmt::format(
        "Error: warning-limit value is required for peer-group '{}'",
        groupName);
  }

  int64_t warningLimit;
  try {
    warningLimit = std::stoll(warningLimitMsg.data()[0]);
  } catch (const std::exception& e) {
    return fmt::format(
        "Error: invalid warning-limit value '{}': {}",
        warningLimitMsg.data()[0],
        e.what());
  }

  session.createPeerGroup(groupName);
  session.setPeerGroupPreFilterWarningLimit(groupName, warningLimit);
  session.saveConfig();

  return fmt::format(
      "Successfully set pre_filter warning_limit for peer-group '{}' to: {}\n"
      "Config saved to: {}",
      groupName,
      warningLimit,
      session.getSessionConfigPath());
}

void CmdConfigProtocolBgpPeerGroupWarningLimit::printOutput(
    const RetType& output) {
  std::cout << output << std::endl;
}

} // namespace facebook::fboss
