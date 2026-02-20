/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/protocol/bgp/peer/CmdConfigProtocolBgpPeerNextHopSelf.h"

#include <fmt/core.h>
#include "fboss/cli/fboss2/commands/config/protocol/bgp/BgpConfigSession.h"

namespace facebook::fboss {

CmdConfigProtocolBgpPeerNextHopSelfTraits::RetType
CmdConfigProtocolBgpPeerNextHopSelf::queryClient(
    const HostInfo& /* hostInfo */,
    const utils::IPList& peerAddressList,
    const ObjectArgType& enableMsg) {
  auto& session = BgpConfigSession::getInstance();

  if (peerAddressList.data().empty()) {
    return "Error: peer address is required";
  }
  std::string peerAddress = peerAddressList.data()[0];

  if (enableMsg.data().empty()) {
    return fmt::format(
        "Error: enable value (true/false) is required for peer '{}'",
        peerAddress);
  }

  std::string enableStr = enableMsg.data()[0];
  bool enable = (enableStr == "true" || enableStr == "1" || enableStr == "yes");

  session.createPeer(peerAddress);
  session.setPeerNextHopSelf(peerAddress, enable);
  session.saveConfig();

  return fmt::format(
      "Successfully {} next-hop-self for peer '{}'\n"
      "Config saved to: {}",
      enable ? "enabled" : "disabled",
      peerAddress,
      session.getSessionConfigPath());
}

void CmdConfigProtocolBgpPeerNextHopSelf::printOutput(const RetType& output) {
  std::cout << output << std::endl;
}

} // namespace facebook::fboss
