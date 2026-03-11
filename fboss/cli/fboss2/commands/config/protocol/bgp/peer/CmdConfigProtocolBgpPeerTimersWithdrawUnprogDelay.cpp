/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/protocol/bgp/peer/CmdConfigProtocolBgpPeerTimersWithdrawUnprogDelay.h"

#include <fmt/core.h>
#include "fboss/cli/fboss2/commands/config/protocol/bgp/BgpConfigSession.h"

namespace facebook::fboss {

CmdConfigProtocolBgpPeerTimersWithdrawUnprogDelayTraits::RetType
CmdConfigProtocolBgpPeerTimersWithdrawUnprogDelay::queryClient(
    const HostInfo& /* hostInfo */,
    const utils::IPList& peerAddressList,
    const ObjectArgType& delayMsg) {
  auto& session = BgpConfigSession::getInstance();

  if (peerAddressList.data().empty()) {
    return "Error: peer address is required";
  }
  std::string peerAddress = peerAddressList.data()[0];

  if (delayMsg.data().empty()) {
    return fmt::format(
        "Error: withdraw-unprog-delay value (seconds) is required for peer '{}'",
        peerAddress);
  }

  int32_t delay;
  try {
    delay = std::stoi(delayMsg.data()[0]);
  } catch (const std::exception& e) {
    return fmt::format(
        "Error: invalid withdraw-unprog-delay value '{}': {}",
        delayMsg.data()[0],
        e.what());
  }

  session.createPeer(peerAddress);
  session.setPeerWithdrawUnprogDelay(peerAddress, delay);
  session.saveConfig();

  return fmt::format(
      "Successfully set withdraw-unprog-delay for peer '{}' to: {} seconds\n"
      "Config saved to: {}",
      peerAddress,
      delay,
      session.getSessionConfigPath());
}

void CmdConfigProtocolBgpPeerTimersWithdrawUnprogDelay::printOutput(
    const RetType& output) {
  std::cout << output << std::endl;
}

} // namespace facebook::fboss
