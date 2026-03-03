/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/protocol/bgp/peer/CmdConfigProtocolBgpPeerPeerId.h"

#include <fmt/core.h>
#include "fboss/cli/fboss2/commands/config/protocol/bgp/BgpConfigSession.h"

namespace facebook::fboss {

CmdConfigProtocolBgpPeerPeerIdTraits::RetType
CmdConfigProtocolBgpPeerPeerId::queryClient(
    const HostInfo& /* hostInfo */,
    const utils::IPList& peerAddressList,
    const ObjectArgType& peerIdMsg) {
  auto& session = BgpConfigSession::getInstance();

  if (peerAddressList.data().empty()) {
    return "Error: peer address is required";
  }
  std::string peerAddress = peerAddressList.data()[0];

  if (peerIdMsg.data().empty()) {
    return fmt::format(
        "Error: peer-id value is required for peer '{}'", peerAddress);
  }

  session.createPeer(peerAddress);
  session.setPeerPeerId(peerAddress, peerIdMsg.data()[0]);
  session.saveConfig();

  return fmt::format(
      "Successfully set peer-id for peer '{}' to: '{}'\n"
      "Config saved to: {}",
      peerAddress,
      peerIdMsg.data()[0],
      session.getSessionConfigPath());
}

void CmdConfigProtocolBgpPeerPeerId::printOutput(const RetType& output) {
  std::cout << output << std::endl;
}

} // namespace facebook::fboss
