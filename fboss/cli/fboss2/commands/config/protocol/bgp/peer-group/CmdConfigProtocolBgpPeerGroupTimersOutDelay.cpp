/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/protocol/bgp/peer-group/CmdConfigProtocolBgpPeerGroupTimersOutDelay.h"

#include <fmt/core.h>
#include <folly/Conv.h>
#include "fboss/cli/fboss2/commands/config/protocol/bgp/BgpConfigSession.h"

namespace facebook::fboss {

CmdConfigProtocolBgpPeerGroupTimersOutDelayTraits::RetType
CmdConfigProtocolBgpPeerGroupTimersOutDelay::queryClient(
    const HostInfo& /* hostInfo */,
    const utils::Message& peerGroupNameMsg,
    const ObjectArgType& outDelayMsg) {
  auto& session = BgpConfigSession::getInstance();

  if (peerGroupNameMsg.data().empty()) {
    return "Error: peer-group name is required";
  }
  std::string peerGroupName = peerGroupNameMsg.data()[0];

  if (outDelayMsg.data().empty()) {
    return fmt::format(
        "Error: out-delay value is required for peer-group '{}'",
        peerGroupName);
  }

  int32_t outDelay;
  try {
    outDelay = folly::to<int32_t>(outDelayMsg.data()[0]);
  } catch (const std::exception& e) {
    return fmt::format(
        "Error: invalid out-delay value '{}': {}",
        outDelayMsg.data()[0],
        e.what());
  }

  session.createPeerGroup(peerGroupName);
  session.setPeerGroupOutDelay(peerGroupName, outDelay);
  session.saveConfig();

  return fmt::format(
      "Successfully set out-delay for peer-group '{}' to: {} seconds\n"
      "Config saved to: {}",
      peerGroupName,
      outDelay,
      session.getSessionConfigPath());
}

void CmdConfigProtocolBgpPeerGroupTimersOutDelay::printOutput(
    const RetType& output) {
  std::cout << output << std::endl;
}

} // namespace facebook::fboss
