/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/protocol/bgp/peer/CmdConfigProtocolBgpPeerWarningOnly.h"

#include <fmt/core.h>
#include "fboss/cli/fboss2/commands/config/protocol/bgp/BgpConfigSession.h"

namespace facebook::fboss {

CmdConfigProtocolBgpPeerWarningOnlyTraits::RetType
CmdConfigProtocolBgpPeerWarningOnly::queryClient(
    const HostInfo& /* hostInfo */,
    const utils::IPList& peerAddressList,
    const ObjectArgType& warningOnlyMsg) {
  auto& session = BgpConfigSession::getInstance();

  if (peerAddressList.data().empty()) {
    return "Error: peer address is required";
  }
  std::string peerAddress = peerAddressList.data()[0];

  if (warningOnlyMsg.data().empty()) {
    return fmt::format(
        "Error: warning-only value (true|false) is required for peer '{}'",
        peerAddress);
  }

  std::string value = warningOnlyMsg.data()[0];
  bool warningOnly;
  if (value == "true" || value == "1") {
    warningOnly = true;
  } else if (value == "false" || value == "0") {
    warningOnly = false;
  } else {
    return fmt::format(
        "Error: invalid warning-only value '{}', expected 'true' or 'false'",
        value);
  }

  session.createPeer(peerAddress);
  session.setPeerPreFilterWarningOnly(peerAddress, warningOnly);
  session.saveConfig();

  return fmt::format(
      "Successfully set pre_filter warning_only for peer '{}' to: {}\n"
      "Config saved to: {}",
      peerAddress,
      warningOnly ? "true" : "false",
      session.getSessionConfigPath());
}

void CmdConfigProtocolBgpPeerWarningOnly::printOutput(const RetType& output) {
  std::cout << output << std::endl;
}

} // namespace facebook::fboss
