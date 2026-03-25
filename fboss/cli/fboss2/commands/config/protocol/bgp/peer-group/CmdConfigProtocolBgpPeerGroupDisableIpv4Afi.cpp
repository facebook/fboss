/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/protocol/bgp/peer-group/CmdConfigProtocolBgpPeerGroupDisableIpv4Afi.h"

#include <fmt/core.h>
#include "fboss/cli/fboss2/commands/config/protocol/bgp/BgpConfigSession.h"

namespace facebook::fboss {

CmdConfigProtocolBgpPeerGroupDisableIpv4AfiTraits::RetType
CmdConfigProtocolBgpPeerGroupDisableIpv4Afi::queryClient(
    const HostInfo& /* hostInfo */,
    const utils::Message& peerGroupNameMsg,
    const ObjectArgType& disableMsg) {
  auto& session = BgpConfigSession::getInstance();

  if (peerGroupNameMsg.data().empty()) {
    return "Error: peer-group name is required";
  }
  std::string peerGroupName = peerGroupNameMsg.data()[0];

  if (disableMsg.data().empty()) {
    return fmt::format(
        "Error: disable value required for peer-group '{}'", peerGroupName);
  }
  std::string disableStr = disableMsg.data()[0];
  bool disable =
      (disableStr == "true" || disableStr == "1" || disableStr == "yes");

  session.createPeerGroup(peerGroupName);
  session.setPeerGroupDisableIpv4Afi(peerGroupName, disable);
  session.saveConfig();

  return fmt::format(
      "Successfully {} IPv4 AFI for peer-group '{}'\n"
      "Config saved to: {}",
      disable ? "disabled" : "enabled",
      peerGroupName,
      session.getSessionConfigPath());
}

void CmdConfigProtocolBgpPeerGroupDisableIpv4Afi::printOutput(
    const RetType& output) {
  std::cout << output << std::endl;
}

} // namespace facebook::fboss
