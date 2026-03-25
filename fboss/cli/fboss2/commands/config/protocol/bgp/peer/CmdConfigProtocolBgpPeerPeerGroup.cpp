/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/protocol/bgp/peer/CmdConfigProtocolBgpPeerPeerGroup.h"

#include <fmt/core.h>
#include "fboss/cli/fboss2/commands/config/protocol/bgp/BgpConfigSession.h"

namespace facebook::fboss {

CmdConfigProtocolBgpPeerPeerGroupTraits::RetType
CmdConfigProtocolBgpPeerPeerGroup::queryClient(
    const HostInfo& /* hostInfo */,
    const utils::IPList& peerAddressList,
    const ObjectArgType& peerGroupNameMsg) {
  auto& session = BgpConfigSession::getInstance();

  if (peerAddressList.data().empty()) {
    return "Error: peer address is required";
  }
  if (peerGroupNameMsg.data().empty()) {
    return "Error: peer-group name is required";
  }

  std::string peerAddress = peerAddressList.data()[0];
  std::string peerGroupName = peerGroupNameMsg.data()[0];

  session.setPeerGroupName(peerAddress, peerGroupName);
  session.saveConfig();

  return fmt::format(
      "Successfully set peer {} peer-group to: {}\nConfig saved to: {}",
      peerAddress,
      peerGroupName,
      session.getSessionConfigPath());
}

void CmdConfigProtocolBgpPeerPeerGroup::printOutput(const RetType& output) {
  std::cout << output << std::endl;
}

} // namespace facebook::fboss
