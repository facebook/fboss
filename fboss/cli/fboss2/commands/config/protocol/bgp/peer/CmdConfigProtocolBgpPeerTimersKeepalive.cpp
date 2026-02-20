/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/protocol/bgp/peer/CmdConfigProtocolBgpPeerTimersKeepalive.h"

#include <fmt/core.h>
#include "fboss/cli/fboss2/commands/config/protocol/bgp/BgpConfigSession.h"

namespace facebook::fboss {

CmdConfigProtocolBgpPeerTimersKeepaliveTraits::RetType
CmdConfigProtocolBgpPeerTimersKeepalive::queryClient(
    const HostInfo& /* hostInfo */,
    const utils::IPList& peerAddressList,
    const ObjectArgType& keepaliveMsg) {
  auto& session = BgpConfigSession::getInstance();

  if (peerAddressList.data().empty()) {
    return "Error: peer address is required";
  }
  std::string peerAddress = peerAddressList.data()[0];

  if (keepaliveMsg.data().empty()) {
    return fmt::format(
        "Error: keepalive value (seconds) is required for peer '{}'",
        peerAddress);
  }

  int32_t keepalive;
  try {
    keepalive = std::stoi(keepaliveMsg.data()[0]);
  } catch (const std::exception& e) {
    return fmt::format(
        "Error: invalid keepalive value '{}': {}",
        keepaliveMsg.data()[0],
        e.what());
  }

  session.createPeer(peerAddress);
  session.setPeerKeepalive(peerAddress, keepalive);
  session.saveConfig();

  return fmt::format(
      "Successfully set keepalive for peer '{}' to: {} seconds\n"
      "Config saved to: {}",
      peerAddress,
      keepalive,
      session.getSessionConfigPath());
}

void CmdConfigProtocolBgpPeerTimersKeepalive::printOutput(
    const RetType& output) {
  std::cout << output << std::endl;
}

} // namespace facebook::fboss
