/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/protocol/bgp/peer-group/CmdConfigProtocolBgpPeerGroupPeerTag.h"

#include <fmt/core.h>
#include "fboss/cli/fboss2/commands/config/protocol/bgp/BgpConfigSession.h"

namespace facebook::fboss {

CmdConfigProtocolBgpPeerGroupPeerTagTraits::RetType
CmdConfigProtocolBgpPeerGroupPeerTag::queryClient(
    const HostInfo& /* hostInfo */,
    const utils::Message& groupNameArg,
    const ObjectArgType& peerTagMsg) {
  auto& session = BgpConfigSession::getInstance();

  if (groupNameArg.data().empty()) {
    return "Error: peer-group name is required";
  }
  std::string groupName = groupNameArg.data()[0];

  if (peerTagMsg.data().empty()) {
    return fmt::format(
        "Error: peer-tag value is required for peer-group '{}'", groupName);
  }

  session.createPeerGroup(groupName);
  session.setPeerGroupPeerTag(groupName, peerTagMsg.data()[0]);
  session.saveConfig();

  return fmt::format(
      "Successfully set peer_tag for peer-group '{}' to: '{}'\n"
      "Config saved to: {}",
      groupName,
      peerTagMsg.data()[0],
      session.getSessionConfigPath());
}

void CmdConfigProtocolBgpPeerGroupPeerTag::printOutput(const RetType& output) {
  std::cout << output << std::endl;
}

} // namespace facebook::fboss
