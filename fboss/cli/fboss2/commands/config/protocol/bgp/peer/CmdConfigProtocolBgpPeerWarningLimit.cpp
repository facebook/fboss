/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/protocol/bgp/peer/CmdConfigProtocolBgpPeerWarningLimit.h"

#include <fmt/core.h>
#include "fboss/cli/fboss2/commands/config/protocol/bgp/BgpConfigSession.h"

namespace facebook::fboss {

CmdConfigProtocolBgpPeerWarningLimitTraits::RetType
CmdConfigProtocolBgpPeerWarningLimit::queryClient(
    const HostInfo& /* hostInfo */,
    const utils::IPList& peerAddressList,
    const ObjectArgType& warningLimitMsg) {
  auto& session = BgpConfigSession::getInstance();

  if (peerAddressList.data().empty()) {
    return "Error: peer address is required";
  }
  std::string peerAddress = peerAddressList.data()[0];

  if (warningLimitMsg.data().empty()) {
    return fmt::format(
        "Error: warning-limit value is required for peer '{}'", peerAddress);
  }

  int64_t warningLimit;
  try {
    warningLimit = std::stoll(warningLimitMsg.data()[0]);
  } catch (const std::exception& e) {
    return fmt::format(
        "Error: invalid warning-limit value '{}': {}",
        warningLimitMsg.data()[0],
        e.what());
  }

  session.createPeer(peerAddress);
  session.setPeerPreFilterWarningLimit(peerAddress, warningLimit);
  session.saveConfig();

  return fmt::format(
      "Successfully set pre_filter warning_limit for peer '{}' to: {}\n"
      "Config saved to: {}",
      peerAddress,
      warningLimit,
      session.getSessionConfigPath());
}

void CmdConfigProtocolBgpPeerWarningLimit::printOutput(const RetType& output) {
  std::cout << output << std::endl;
}

} // namespace facebook::fboss
