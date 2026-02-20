/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/protocol/bgp/peer/CmdConfigProtocolBgpPeerLocalAddr.h"

#include <fmt/core.h>
#include "fboss/cli/fboss2/commands/config/protocol/bgp/BgpConfigSession.h"

namespace facebook::fboss {

CmdConfigProtocolBgpPeerLocalAddrTraits::RetType
CmdConfigProtocolBgpPeerLocalAddr::queryClient(
    const HostInfo& /* hostInfo */,
    const utils::IPList& peerAddressList,
    const ObjectArgType& localAddrMsg) {
  auto& session = BgpConfigSession::getInstance();

  if (peerAddressList.data().empty()) {
    return "Error: peer address is required";
  }
  std::string peerAddress = peerAddressList.data()[0];

  if (localAddrMsg.data().empty()) {
    return fmt::format(
        "Error: local-addr value required for peer '{}'", peerAddress);
  }
  std::string localAddr = localAddrMsg.data()[0];

  session.createPeer(peerAddress);
  session.setPeerLocalAddr(peerAddress, localAddr);
  session.saveConfig();

  return fmt::format(
      "Successfully set local-addr for peer '{}' to: '{}'\n"
      "Config saved to: {}",
      peerAddress,
      localAddr,
      session.getSessionConfigPath());
}

void CmdConfigProtocolBgpPeerLocalAddr::printOutput(const RetType& output) {
  std::cout << output << std::endl;
}

} // namespace facebook::fboss
