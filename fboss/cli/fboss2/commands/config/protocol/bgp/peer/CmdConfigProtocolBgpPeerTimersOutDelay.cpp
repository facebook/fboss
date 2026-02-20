/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/protocol/bgp/peer/CmdConfigProtocolBgpPeerTimersOutDelay.h"

#include <fmt/core.h>
#include "fboss/cli/fboss2/commands/config/protocol/bgp/BgpConfigSession.h"

namespace facebook::fboss {

CmdConfigProtocolBgpPeerTimersOutDelayTraits::RetType
CmdConfigProtocolBgpPeerTimersOutDelay::queryClient(
    const HostInfo& /* hostInfo */,
    const utils::IPList& peerAddressList,
    const ObjectArgType& outDelayMsg) {
  auto& session = BgpConfigSession::getInstance();

  if (peerAddressList.data().empty()) {
    return "Error: peer address is required";
  }
  std::string peerAddress = peerAddressList.data()[0];

  if (outDelayMsg.data().empty()) {
    return fmt::format(
        "Error: out-delay value (seconds) is required for peer '{}'",
        peerAddress);
  }

  int32_t outDelay;
  try {
    outDelay = std::stoi(outDelayMsg.data()[0]);
  } catch (const std::exception& e) {
    return fmt::format(
        "Error: invalid out-delay value '{}': {}",
        outDelayMsg.data()[0],
        e.what());
  }

  session.createPeer(peerAddress);
  session.setPeerOutDelay(peerAddress, outDelay);
  session.saveConfig();

  return fmt::format(
      "Successfully set out-delay for peer '{}' to: {} seconds\n"
      "Config saved to: {}",
      peerAddress,
      outDelay,
      session.getSessionConfigPath());
}

void CmdConfigProtocolBgpPeerTimersOutDelay::printOutput(
    const RetType& output) {
  std::cout << output << std::endl;
}

} // namespace facebook::fboss
