/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/protocol/bgp/peer-group/CmdConfigProtocolBgpPeerGroupWarningOnly.h"

#include <fmt/core.h>
#include "fboss/cli/fboss2/commands/config/protocol/bgp/BgpConfigSession.h"

namespace facebook::fboss {

CmdConfigProtocolBgpPeerGroupWarningOnlyTraits::RetType
CmdConfigProtocolBgpPeerGroupWarningOnly::queryClient(
    const HostInfo& /* hostInfo */,
    const utils::Message& groupNameArg,
    const ObjectArgType& warningOnlyMsg) {
  auto& session = BgpConfigSession::getInstance();

  if (groupNameArg.data().empty()) {
    return "Error: peer-group name is required";
  }
  std::string groupName = groupNameArg.data()[0];

  if (warningOnlyMsg.data().empty()) {
    return fmt::format(
        "Error: warning-only value (true|false) is required for peer-group '{}'",
        groupName);
  }

  std::string value = warningOnlyMsg.data()[0];
  bool warningOnly;
  if (value == "true" || value == "1") {
    warningOnly = true;
  } else if (value == "false" || value == "0") {
    warningOnly = false;
  } else {
    return fmt::format(
        "Error: invalid warning-only value '{}', expected 'true' or 'false'",
        value);
  }

  session.createPeerGroup(groupName);
  session.setPeerGroupPreFilterWarningOnly(groupName, warningOnly);
  session.saveConfig();

  return fmt::format(
      "Successfully set pre_filter warning_only for peer-group '{}' to: {}\n"
      "Config saved to: {}",
      groupName,
      warningOnly ? "true" : "false",
      session.getSessionConfigPath());
}

void CmdConfigProtocolBgpPeerGroupWarningOnly::printOutput(
    const RetType& output) {
  std::cout << output << std::endl;
}

} // namespace facebook::fboss
