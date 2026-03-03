/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/protocol/bgp/peer-group/CmdConfigProtocolBgpPeerGroupTimersKeepalive.h"

#include <fmt/core.h>
#include <folly/Conv.h>
#include "fboss/cli/fboss2/commands/config/protocol/bgp/BgpConfigSession.h"

namespace facebook::fboss {

CmdConfigProtocolBgpPeerGroupTimersKeepaliveTraits::RetType
CmdConfigProtocolBgpPeerGroupTimersKeepalive::queryClient(
    const HostInfo& /* hostInfo */,
    const utils::Message& peerGroupNameMsg,
    const ObjectArgType& keepaliveMsg) {
  auto& session = BgpConfigSession::getInstance();

  if (peerGroupNameMsg.data().empty()) {
    return "Error: peer-group name is required";
  }
  std::string peerGroupName = peerGroupNameMsg.data()[0];

  if (keepaliveMsg.data().empty()) {
    return fmt::format(
        "Error: keepalive interval is required for peer-group '{}'",
        peerGroupName);
  }

  int32_t keepalive;
  try {
    keepalive = folly::to<int32_t>(keepaliveMsg.data()[0]);
  } catch (const std::exception& e) {
    return fmt::format(
        "Error: invalid keepalive value '{}': {}",
        keepaliveMsg.data()[0],
        e.what());
  }

  session.createPeerGroup(peerGroupName);
  session.setPeerGroupKeepalive(peerGroupName, keepalive);
  session.saveConfig();

  return fmt::format(
      "Successfully set keepalive interval for peer-group '{}' to: {} seconds\n"
      "Config saved to: {}",
      peerGroupName,
      keepalive,
      session.getSessionConfigPath());
}

void CmdConfigProtocolBgpPeerGroupTimersKeepalive::printOutput(
    const RetType& output) {
  std::cout << output << std::endl;
}

} // namespace facebook::fboss
