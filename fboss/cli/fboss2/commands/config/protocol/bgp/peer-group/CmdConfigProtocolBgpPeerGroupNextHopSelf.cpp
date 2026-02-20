/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/protocol/bgp/peer-group/CmdConfigProtocolBgpPeerGroupNextHopSelf.h"

#include <fmt/core.h>
#include "fboss/cli/fboss2/commands/config/protocol/bgp/BgpConfigSession.h"

namespace facebook::fboss {

CmdConfigProtocolBgpPeerGroupNextHopSelfTraits::RetType
CmdConfigProtocolBgpPeerGroupNextHopSelf::queryClient(
    const HostInfo& /* hostInfo */,
    const utils::Message& peerGroupNameMsg,
    const ObjectArgType& enableMsg) {
  auto& session = BgpConfigSession::getInstance();

  if (peerGroupNameMsg.data().empty()) {
    return "Error: peer-group name is required";
  }
  std::string peerGroupName = peerGroupNameMsg.data()[0];

  if (enableMsg.data().empty()) {
    return fmt::format(
        "Error: enable value (true/false) is required for peer-group '{}'",
        peerGroupName);
  }

  std::string enableStr = enableMsg.data()[0];
  bool enable = (enableStr == "true" || enableStr == "1" || enableStr == "yes");

  session.createPeerGroup(peerGroupName);
  session.setPeerGroupNextHopSelf(peerGroupName, enable);
  session.saveConfig();

  return fmt::format(
      "Successfully {} next-hop-self for peer-group '{}'\n"
      "Config saved to: {}",
      enable ? "enabled" : "disabled",
      peerGroupName,
      session.getSessionConfigPath());
}

void CmdConfigProtocolBgpPeerGroupNextHopSelf::printOutput(
    const RetType& output) {
  std::cout << output << std::endl;
}

} // namespace facebook::fboss
