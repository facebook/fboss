/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/protocol/bgp/peer-group/CmdConfigProtocolBgpPeerGroupTimersWithdrawUnprogDelay.h"

#include <fmt/core.h>
#include "fboss/cli/fboss2/commands/config/protocol/bgp/BgpConfigSession.h"

namespace facebook::fboss {

CmdConfigProtocolBgpPeerGroupTimersWithdrawUnprogDelayTraits::RetType
CmdConfigProtocolBgpPeerGroupTimersWithdrawUnprogDelay::queryClient(
    const HostInfo& /* hostInfo */,
    const utils::Message& groupNameArg,
    const ObjectArgType& delayMsg) {
  auto& session = BgpConfigSession::getInstance();

  if (groupNameArg.data().empty()) {
    return "Error: peer-group name is required";
  }
  std::string groupName = groupNameArg.data()[0];

  if (delayMsg.data().empty()) {
    return fmt::format(
        "Error: withdraw-unprog-delay value (seconds) is required for peer-group '{}'",
        groupName);
  }

  int32_t delay;
  try {
    delay = std::stoi(delayMsg.data()[0]);
  } catch (const std::exception& e) {
    return fmt::format(
        "Error: invalid withdraw-unprog-delay value '{}': {}",
        delayMsg.data()[0],
        e.what());
  }

  session.createPeerGroup(groupName);
  session.setPeerGroupWithdrawUnprogDelay(groupName, delay);
  session.saveConfig();

  return fmt::format(
      "Successfully set withdraw-unprog-delay for peer-group '{}' to: {} seconds\n"
      "Config saved to: {}",
      groupName,
      delay,
      session.getSessionConfigPath());
}

void CmdConfigProtocolBgpPeerGroupTimersWithdrawUnprogDelay::printOutput(
    const RetType& output) {
  std::cout << output << std::endl;
}

} // namespace facebook::fboss
