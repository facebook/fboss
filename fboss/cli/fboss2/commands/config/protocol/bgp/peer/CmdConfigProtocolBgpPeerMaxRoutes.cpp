/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/protocol/bgp/peer/CmdConfigProtocolBgpPeerMaxRoutes.h"

#include <fmt/core.h>
#include <folly/Conv.h>
#include "fboss/cli/fboss2/commands/config/protocol/bgp/BgpConfigSession.h"

namespace facebook::fboss {

CmdConfigProtocolBgpPeerMaxRoutesTraits::RetType
CmdConfigProtocolBgpPeerMaxRoutes::queryClient(
    const HostInfo& /* hostInfo */,
    const utils::IPList& peerAddressList,
    const ObjectArgType& maxRoutesMsg) {
  auto& session = BgpConfigSession::getInstance();

  if (peerAddressList.data().empty()) {
    return "Error: peer address is required";
  }
  std::string peerAddress = peerAddressList.data()[0];

  if (maxRoutesMsg.data().empty()) {
    return fmt::format(
        "Error: max-routes value is required for peer '{}'", peerAddress);
  }

  int64_t maxRoutes;
  try {
    maxRoutes = folly::to<int64_t>(maxRoutesMsg.data()[0]);
  } catch (const std::exception& e) {
    return fmt::format(
        "Error: invalid max-routes value '{}': {}",
        maxRoutesMsg.data()[0],
        e.what());
  }

  session.createPeer(peerAddress);
  session.setPeerPreFilterMaxRoutes(peerAddress, maxRoutes);
  session.saveConfig();

  return fmt::format(
      "Successfully set max-routes for peer '{}' to: {}\n"
      "Config saved to: {}",
      peerAddress,
      maxRoutes,
      session.getSessionConfigPath());
}

void CmdConfigProtocolBgpPeerMaxRoutes::printOutput(const RetType& output) {
  std::cout << output << std::endl;
}

} // namespace facebook::fboss
