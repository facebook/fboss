/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/protocol/bgp/peer/CmdConfigProtocolBgpPeerDisableIpv4Afi.h"

#include <fmt/core.h>
#include "fboss/cli/fboss2/commands/config/protocol/bgp/BgpConfigSession.h"

namespace facebook::fboss {

CmdConfigProtocolBgpPeerDisableIpv4AfiTraits::RetType
CmdConfigProtocolBgpPeerDisableIpv4Afi::queryClient(
    const HostInfo& /* hostInfo */,
    const utils::IPList& peerAddressList,
    const ObjectArgType& disableMsg) {
  auto& session = BgpConfigSession::getInstance();

  if (peerAddressList.data().empty()) {
    return "Error: peer address is required";
  }
  std::string peerAddress = peerAddressList.data()[0];

  if (disableMsg.data().empty()) {
    return fmt::format(
        "Error: disable value required for peer '{}'", peerAddress);
  }
  std::string disableStr = disableMsg.data()[0];
  bool disable =
      (disableStr == "true" || disableStr == "1" || disableStr == "yes");

  session.createPeer(peerAddress);
  session.setPeerDisableIpv4Afi(peerAddress, disable);
  session.saveConfig();

  return fmt::format(
      "Successfully {} IPv4 AFI for peer '{}'\n"
      "Config saved to: {}",
      disable ? "disabled" : "enabled",
      peerAddress,
      session.getSessionConfigPath());
}

void CmdConfigProtocolBgpPeerDisableIpv4Afi::printOutput(
    const RetType& output) {
  std::cout << output << std::endl;
}

} // namespace facebook::fboss
