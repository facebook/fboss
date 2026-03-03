/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/protocol/bgp/peer/CmdConfigProtocolBgpPeerLinkBandwidth.h"

#include <fmt/core.h>
#include <folly/Conv.h>
#include "fboss/cli/fboss2/commands/config/protocol/bgp/BgpConfigSession.h"

namespace facebook::fboss {

namespace {
// Parse bandwidth string like "10G", "100M", "1000" to bps
int64_t parseBandwidth(const std::string& bwStr) {
  std::string upper = bwStr;
  for (auto& c : upper) {
    c = std::toupper(c);
  }

  int64_t multiplier = 1;
  std::string numPart = upper;

  if (upper.back() == 'G') {
    multiplier = 1000000000LL;
    numPart = upper.substr(0, upper.length() - 1);
  } else if (upper.back() == 'M') {
    multiplier = 1000000LL;
    numPart = upper.substr(0, upper.length() - 1);
  } else if (upper.back() == 'K') {
    multiplier = 1000LL;
    numPart = upper.substr(0, upper.length() - 1);
  }

  return folly::to<int64_t>(numPart) * multiplier;
}
} // namespace

CmdConfigProtocolBgpPeerLinkBandwidthTraits::RetType
CmdConfigProtocolBgpPeerLinkBandwidth::queryClient(
    const HostInfo& /* hostInfo */,
    const utils::IPList& peerAddressList,
    const ObjectArgType& bandwidthMsg) {
  auto& session = BgpConfigSession::getInstance();

  if (peerAddressList.data().empty()) {
    return "Error: peer address is required";
  }
  std::string peerAddress = peerAddressList.data()[0];

  if (bandwidthMsg.data().empty()) {
    return fmt::format(
        "Error: link-bandwidth value is required for peer '{}'", peerAddress);
  }

  int64_t bps;
  try {
    bps = parseBandwidth(bandwidthMsg.data()[0]);
  } catch (const std::exception& e) {
    return fmt::format(
        "Error: invalid link-bandwidth value '{}': {}",
        bandwidthMsg.data()[0],
        e.what());
  }

  session.createPeer(peerAddress);
  session.setPeerLbwValue(peerAddress, bps);
  session.saveConfig();

  return fmt::format(
      "Successfully set link-bandwidth for peer '{}' to: {} bps\n"
      "Config saved to: {}",
      peerAddress,
      bps,
      session.getSessionConfigPath());
}

void CmdConfigProtocolBgpPeerLinkBandwidth::printOutput(const RetType& output) {
  std::cout << output << std::endl;
}

} // namespace facebook::fboss
