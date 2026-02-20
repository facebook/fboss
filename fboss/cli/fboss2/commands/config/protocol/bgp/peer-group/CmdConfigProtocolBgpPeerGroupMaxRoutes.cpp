/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/protocol/bgp/peer-group/CmdConfigProtocolBgpPeerGroupMaxRoutes.h"

#include <fmt/core.h>
#include <folly/Conv.h>
#include "fboss/cli/fboss2/commands/config/protocol/bgp/BgpConfigSession.h"

namespace facebook::fboss {

CmdConfigProtocolBgpPeerGroupMaxRoutesTraits::RetType
CmdConfigProtocolBgpPeerGroupMaxRoutes::queryClient(
    const HostInfo& /* hostInfo */,
    const utils::Message& peerGroupNameMsg,
    const ObjectArgType& maxRoutesMsg) {
  auto& session = BgpConfigSession::getInstance();

  if (peerGroupNameMsg.data().empty()) {
    return "Error: peer-group name is required";
  }
  std::string peerGroupName = peerGroupNameMsg.data()[0];

  if (maxRoutesMsg.data().empty()) {
    return fmt::format(
        "Error: max-routes value is required for peer-group '{}'",
        peerGroupName);
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

  session.createPeerGroup(peerGroupName);
  session.setPeerGroupPreFilterMaxRoutes(peerGroupName, maxRoutes);
  session.saveConfig();

  return fmt::format(
      "Successfully set max-routes for peer-group '{}' to: {}\n"
      "Config saved to: {}",
      peerGroupName,
      maxRoutes,
      session.getSessionConfigPath());
}

void CmdConfigProtocolBgpPeerGroupMaxRoutes::printOutput(
    const RetType& output) {
  std::cout << output << std::endl;
}

} // namespace facebook::fboss
