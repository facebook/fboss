/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/protocol/bgp/peer-group/CmdConfigProtocolBgpPeerGroupTimersHoldTime.h"

#include <fmt/core.h>
#include <folly/Conv.h>
#include "fboss/cli/fboss2/commands/config/protocol/bgp/BgpConfigSession.h"

namespace facebook::fboss {

CmdConfigProtocolBgpPeerGroupTimersHoldTimeTraits::RetType
CmdConfigProtocolBgpPeerGroupTimersHoldTime::queryClient(
    const HostInfo& /* hostInfo */,
    const utils::Message& peerGroupNameMsg,
    const ObjectArgType& holdTimeMsg) {
  auto& session = BgpConfigSession::getInstance();

  if (peerGroupNameMsg.data().empty()) {
    return "Error: peer-group name is required";
  }
  if (holdTimeMsg.data().empty()) {
    return "Error: hold-time value is required";
  }

  std::string groupName = peerGroupNameMsg.data()[0];
  std::string holdTimeStr = holdTimeMsg.data()[0];
  int32_t holdTime;
  try {
    holdTime = folly::to<int32_t>(holdTimeStr);
  } catch (const std::exception& e) {
    return fmt::format(
        "Error: Invalid hold-time '{}': {}", holdTimeStr, e.what());
  }

  session.setPeerGroupHoldTime(groupName, holdTime);
  session.saveConfig();

  return fmt::format(
      "Successfully set peer-group {} hold-time to: {} seconds\nConfig saved to: {}",
      groupName,
      holdTime,
      session.getSessionConfigPath());
}

void CmdConfigProtocolBgpPeerGroupTimersHoldTime::printOutput(
    const RetType& output) {
  std::cout << output << std::endl;
}

} // namespace facebook::fboss
