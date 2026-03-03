/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/protocol/bgp/peer-group/CmdConfigProtocolBgpPeerGroupRemoteAsn.h"

#include <fmt/core.h>
#include <folly/Conv.h>
#include "fboss/cli/fboss2/commands/config/protocol/bgp/BgpConfigSession.h"

namespace facebook::fboss {

CmdConfigProtocolBgpPeerGroupRemoteAsnTraits::RetType
CmdConfigProtocolBgpPeerGroupRemoteAsn::queryClient(
    const HostInfo& /* hostInfo */,
    const utils::Message& groupNameMsg,
    const ObjectArgType& remoteAsnMsg) {
  auto& session = BgpConfigSession::getInstance();

  if (groupNameMsg.data().empty()) {
    return "Error: peer-group name is required";
  }
  if (remoteAsnMsg.data().empty()) {
    return "Error: remote-asn value is required";
  }

  std::string groupName = groupNameMsg.data()[0];
  std::string asnStr = remoteAsnMsg.data()[0];
  uint64_t remoteAsn;
  try {
    remoteAsn = folly::to<uint64_t>(asnStr);
  } catch (const std::exception& e) {
    return fmt::format("Error: Invalid remote-asn '{}': {}", asnStr, e.what());
  }

  session.setPeerGroupRemoteAsn(groupName, remoteAsn);
  session.saveConfig();

  return fmt::format(
      "Successfully set peer-group {} remote-asn to: {}\nConfig saved to: {}",
      groupName,
      remoteAsn,
      session.getSessionConfigPath());
}

void CmdConfigProtocolBgpPeerGroupRemoteAsn::printOutput(
    const RetType& output) {
  std::cout << output << std::endl;
}

} // namespace facebook::fboss
