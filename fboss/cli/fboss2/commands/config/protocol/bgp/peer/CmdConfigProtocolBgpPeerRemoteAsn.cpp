/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/protocol/bgp/peer/CmdConfigProtocolBgpPeerRemoteAsn.h"

#include <fmt/core.h>
#include <folly/Conv.h>
#include "fboss/cli/fboss2/commands/config/protocol/bgp/BgpConfigSession.h"

namespace facebook::fboss {

CmdConfigProtocolBgpPeerRemoteAsnTraits::RetType
CmdConfigProtocolBgpPeerRemoteAsn::queryClient(
    const HostInfo& /* hostInfo */,
    const utils::IPList& peerAddressList,
    const ObjectArgType& remoteAsnMsg) {
  auto& session = BgpConfigSession::getInstance();

  if (peerAddressList.data().empty()) {
    return "Error: peer address is required";
  }

  if (remoteAsnMsg.data().empty()) {
    return "Error: remote-asn value is required";
  }

  // Extract the first IP from the list as peer address
  std::string peerAddress = peerAddressList.data()[0];

  // Extract string and parse to uint64_t (4-byte ASN per RFC 6793)
  std::string asnStr = remoteAsnMsg.data()[0];
  uint64_t remoteAsn;
  try {
    remoteAsn = folly::to<uint64_t>(asnStr);
  } catch (const std::exception& e) {
    return fmt::format(
        "Error: Invalid remote-asn value '{}': {}", asnStr, e.what());
  }

  // Set the remote-asn for the peer (internally maps to remote_as_4_byte)
  session.setPeerRemoteAsn(peerAddress, remoteAsn);
  session.saveConfig();

  return fmt::format(
      "Successfully set BGP peer {} remote-asn to: {}\nConfig saved to: {}",
      peerAddress,
      remoteAsn,
      session.getSessionConfigPath());
}

void CmdConfigProtocolBgpPeerRemoteAsn::printOutput(const RetType& output) {
  std::cout << output << std::endl;
}

} // namespace facebook::fboss
