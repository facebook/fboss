/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/protocol/bgp/peer/CmdConfigProtocolBgpPeerHoldTime.h"

#include <fmt/core.h>
#include <folly/Conv.h>
#include "fboss/cli/fboss2/commands/config/protocol/bgp/BgpConfigSession.h"

namespace facebook::fboss {

CmdConfigProtocolBgpPeerHoldTimeTraits::RetType
CmdConfigProtocolBgpPeerHoldTime::queryClient(
    const HostInfo& /* hostInfo */,
    const utils::IPList& peerAddressList,
    const ObjectArgType& holdTimeMsg) {
  auto& session = BgpConfigSession::getInstance();

  if (peerAddressList.data().empty()) {
    return "Error: peer address is required";
  }
  std::string peerAddress = peerAddressList.data()[0];

  if (holdTimeMsg.data().empty()) {
    return fmt::format(
        "Error: hold-time value is required for peer '{}'", peerAddress);
  }

  int32_t holdTime;
  try {
    holdTime = folly::to<int32_t>(holdTimeMsg.data()[0]);
  } catch (const std::exception& e) {
    return fmt::format(
        "Error: invalid hold-time value '{}': {}",
        holdTimeMsg.data()[0],
        e.what());
  }

  session.createPeer(peerAddress);
  session.setPeerHoldTime(peerAddress, holdTime);
  session.saveConfig();

  return fmt::format(
      "Successfully set hold-time for peer '{}' to: {} seconds\n"
      "Config saved to: {}",
      peerAddress,
      holdTime,
      session.getSessionConfigPath());
}

void CmdConfigProtocolBgpPeerHoldTime::printOutput(const RetType& output) {
  std::cout << output << std::endl;
}

} // namespace facebook::fboss
