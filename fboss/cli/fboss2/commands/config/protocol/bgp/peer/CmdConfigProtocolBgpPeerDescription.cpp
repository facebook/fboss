/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/protocol/bgp/peer/CmdConfigProtocolBgpPeerDescription.h"

#include <fmt/core.h>
#include "fboss/cli/fboss2/commands/config/protocol/bgp/BgpConfigSession.h"

namespace facebook::fboss {

CmdConfigProtocolBgpPeerDescriptionTraits::RetType
CmdConfigProtocolBgpPeerDescription::queryClient(
    const HostInfo& /* hostInfo */,
    const utils::IPList& peerAddressList,
    const ObjectArgType& descriptionMsg) {
  auto& session = BgpConfigSession::getInstance();

  if (peerAddressList.data().empty()) {
    return "Error: peer address is required";
  }
  std::string peerAddress = peerAddressList.data()[0];

  if (descriptionMsg.data().empty()) {
    return fmt::format(
        "Error: description value is required for peer '{}'", peerAddress);
  }

  session.createPeer(peerAddress);
  session.setPeerDescription(peerAddress, descriptionMsg.data()[0]);
  session.saveConfig();

  return fmt::format(
      "Successfully set description for peer '{}' to: '{}'\n"
      "Config saved to: {}",
      peerAddress,
      descriptionMsg.data()[0],
      session.getSessionConfigPath());
}

void CmdConfigProtocolBgpPeerDescription::printOutput(const RetType& output) {
  std::cout << output << std::endl;
}

} // namespace facebook::fboss
