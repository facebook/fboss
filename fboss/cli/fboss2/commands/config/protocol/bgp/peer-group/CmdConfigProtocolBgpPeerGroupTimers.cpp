/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/protocol/bgp/peer-group/CmdConfigProtocolBgpPeerGroupTimers.h"

#include <fmt/core.h>

namespace facebook::fboss {

CmdConfigProtocolBgpPeerGroupTimersTraits::RetType
CmdConfigProtocolBgpPeerGroupTimers::queryClient(
    const HostInfo& /* hostInfo */,
    const utils::Message& peerGroupNameMsg) {
  if (peerGroupNameMsg.data().empty()) {
    return "Error: peer-group name is required";
  }

  std::string groupName = peerGroupNameMsg.data()[0];

  return fmt::format(
      "Configure timers for peer-group {}.\n"
      "Use subcommands: hold-time, keepalive, out-delay",
      groupName);
}

void CmdConfigProtocolBgpPeerGroupTimers::printOutput(const RetType& output) {
  std::cout << output << std::endl;
}

} // namespace facebook::fboss
