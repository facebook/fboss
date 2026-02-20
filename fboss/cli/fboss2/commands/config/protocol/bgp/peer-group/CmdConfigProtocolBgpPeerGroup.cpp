/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/protocol/bgp/peer-group/CmdConfigProtocolBgpPeerGroup.h"

#include <fmt/core.h>
#include "fboss/cli/fboss2/commands/config/protocol/bgp/BgpConfigSession.h"

namespace facebook::fboss {

CmdConfigProtocolBgpPeerGroupTraits::RetType
CmdConfigProtocolBgpPeerGroup::queryClient(
    const HostInfo& /* hostInfo */,
    const ObjectArgType& groupNameMsg) {
  auto& session = BgpConfigSession::getInstance();

  if (groupNameMsg.data().empty()) {
    return "Error: peer-group name is required";
  }

  std::string groupName = groupNameMsg.data()[0];
  session.createPeerGroup(groupName);
  session.saveConfig();

  return fmt::format(
      "BGP peer-group {} created.\n"
      "Use subcommands: remote-asn, description, hold-time, keepalive\n"
      "Config saved to: {}",
      groupName,
      session.getSessionConfigPath());
}

void CmdConfigProtocolBgpPeerGroup::printOutput(const RetType& output) {
  std::cout << output << std::endl;
}

} // namespace facebook::fboss
