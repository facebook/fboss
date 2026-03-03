/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/protocol/bgp/peer/CmdConfigProtocolBgpPeerTimers.h"

#include <fmt/core.h>
#include "fboss/cli/fboss2/commands/config/protocol/bgp/BgpConfigSession.h"

namespace facebook::fboss {

CmdConfigProtocolBgpPeerTimersTraits::RetType
CmdConfigProtocolBgpPeerTimers::queryClient(
    const HostInfo& /* hostInfo */,
    const utils::IPList& peerAddressList) {
  if (peerAddressList.data().empty()) {
    return "Error: peer address is required";
  }
  std::string peerAddress = peerAddressList.data()[0];

  return fmt::format(
      "BGP peer timers configuration for peer '{}'.\n"
      "Available subcommands:\n"
      "  hold-time <seconds>     - Set hold time\n"
      "  keepalive <seconds>     - Set keepalive interval\n"
      "  out-delay <seconds>     - Set output delay\n"
      "  withdraw-unprog-delay <seconds> - Set withdraw unprog delay",
      peerAddress);
}

void CmdConfigProtocolBgpPeerTimers::printOutput(const RetType& output) {
  std::cout << output << std::endl;
}

} // namespace facebook::fboss
