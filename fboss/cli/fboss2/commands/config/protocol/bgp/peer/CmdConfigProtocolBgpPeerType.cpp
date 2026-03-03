/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/protocol/bgp/peer/CmdConfigProtocolBgpPeerType.h"

#include <fmt/core.h>
#include "fboss/cli/fboss2/commands/config/protocol/bgp/BgpConfigSession.h"

namespace facebook::fboss {

CmdConfigProtocolBgpPeerTypeTraits::RetType
CmdConfigProtocolBgpPeerType::queryClient(
    const HostInfo& /* hostInfo */,
    const utils::IPList& peerAddressList,
    const ObjectArgType& peerTypeMsg) {
  auto& session = BgpConfigSession::getInstance();

  if (peerAddressList.data().empty()) {
    return "Error: peer address is required";
  }
  std::string peerAddress = peerAddressList.data()[0];

  if (peerTypeMsg.data().empty()) {
    return fmt::format(
        "Error: type value is required for peer '{}'", peerAddress);
  }

  session.createPeer(peerAddress);
  session.setPeerType(peerAddress, peerTypeMsg.data()[0]);
  session.saveConfig();

  return fmt::format(
      "Successfully set type for peer '{}' to: '{}'\n"
      "Config saved to: {}",
      peerAddress,
      peerTypeMsg.data()[0],
      session.getSessionConfigPath());
}

void CmdConfigProtocolBgpPeerType::printOutput(const RetType& output) {
  std::cout << output << std::endl;
}

} // namespace facebook::fboss
