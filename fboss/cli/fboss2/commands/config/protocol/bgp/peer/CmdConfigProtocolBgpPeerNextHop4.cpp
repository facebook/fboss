/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/protocol/bgp/peer/CmdConfigProtocolBgpPeerNextHop4.h"

#include <fmt/core.h>
#include "fboss/cli/fboss2/commands/config/protocol/bgp/BgpConfigSession.h"

namespace facebook::fboss {

CmdConfigProtocolBgpPeerNextHop4Traits::RetType
CmdConfigProtocolBgpPeerNextHop4::queryClient(
    const HostInfo& /* hostInfo */,
    const utils::IPList& peerAddressList,
    const ObjectArgType& nextHop4Msg) {
  auto& session = BgpConfigSession::getInstance();

  if (peerAddressList.data().empty()) {
    return "Error: peer address is required";
  }
  std::string peerAddress = peerAddressList.data()[0];

  if (nextHop4Msg.data().empty()) {
    return fmt::format(
        "Error: next-hop4 value required for peer '{}'", peerAddress);
  }
  std::string nextHop4 = nextHop4Msg.data()[0];

  session.createPeer(peerAddress);
  session.setPeerNextHop4(peerAddress, nextHop4);
  session.saveConfig();

  return fmt::format(
      "Successfully set next-hop4 for peer '{}' to: '{}'\n"
      "Config saved to: {}",
      peerAddress,
      nextHop4,
      session.getSessionConfigPath());
}

void CmdConfigProtocolBgpPeerNextHop4::printOutput(const RetType& output) {
  std::cout << output << std::endl;
}

} // namespace facebook::fboss
