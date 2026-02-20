/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/protocol/bgp/peer/CmdConfigProtocolBgpPeer.h"

#include <fmt/core.h>
#include "fboss/cli/fboss2/commands/config/protocol/bgp/BgpConfigSession.h"

namespace facebook::fboss {

CmdConfigProtocolBgpPeerTraits::RetType CmdConfigProtocolBgpPeer::queryClient(
    const HostInfo& /* hostInfo */,
    const ObjectArgType& peerAddressList) {
  auto& session = BgpConfigSession::getInstance();

  if (peerAddressList.data().empty()) {
    return "Error: peer address is required";
  }

  // Extract the first IP from the list as peer address
  std::string peerAddress = peerAddressList.data()[0];

  // Create peer entry in the peers list (translates to BGP++ thrift format)
  session.createPeer(peerAddress);
  session.saveConfig();

  return fmt::format(
      "BGP peer {} created.\n"
      "Use subcommands: remote-asn, local-addr, next-hop4, next-hop6, peer-group\n"
      "Config saved to: {}",
      peerAddress,
      session.getSessionConfigPath());
}

void CmdConfigProtocolBgpPeer::printOutput(const RetType& output) {
  std::cout << output << std::endl;
}

} // namespace facebook::fboss
