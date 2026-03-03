/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/protocol/bgp/peer/CmdConfigProtocolBgpPeerNextHop6.h"

#include <fmt/core.h>
#include "fboss/cli/fboss2/commands/config/protocol/bgp/BgpConfigSession.h"

namespace facebook::fboss {

CmdConfigProtocolBgpPeerNextHop6Traits::RetType
CmdConfigProtocolBgpPeerNextHop6::queryClient(
    const HostInfo& /* hostInfo */,
    const utils::IPList& peerAddressList,
    const ObjectArgType& nextHop6Msg) {
  auto& session = BgpConfigSession::getInstance();

  if (peerAddressList.data().empty()) {
    return "Error: peer address is required";
  }
  std::string peerAddress = peerAddressList.data()[0];

  if (nextHop6Msg.data().empty()) {
    return fmt::format(
        "Error: next-hop6 value required for peer '{}'", peerAddress);
  }
  std::string nextHop6 = nextHop6Msg.data()[0];

  session.createPeer(peerAddress);
  session.setPeerNextHop6(peerAddress, nextHop6);
  session.saveConfig();

  return fmt::format(
      "Successfully set next-hop6 for peer '{}' to: '{}'\n"
      "Config saved to: {}",
      peerAddress,
      nextHop6,
      session.getSessionConfigPath());
}

void CmdConfigProtocolBgpPeerNextHop6::printOutput(const RetType& output) {
  std::cout << output << std::endl;
}

} // namespace facebook::fboss
