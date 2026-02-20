/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/protocol/bgp/global/CmdConfigProtocolBgpGlobalLocalAsn.h"

#include <fmt/core.h>
#include "fboss/cli/fboss2/commands/config/protocol/bgp/BgpConfigSession.h"

namespace facebook::fboss {

CmdConfigProtocolBgpGlobalLocalAsnTraits::RetType
CmdConfigProtocolBgpGlobalLocalAsn::queryClient(
    const HostInfo& /* hostInfo */,
    const ObjectArgType& localAsnMsg) {
  auto& session = BgpConfigSession::getInstance();

  if (localAsnMsg.data().empty()) {
    return "Error: local-asn value is required";
  }

  // Extract string and parse to uint32_t
  std::string asnStr = localAsnMsg.data()[0];
  uint32_t localAsn;
  try {
    localAsn = folly::to<uint32_t>(asnStr);
  } catch (const std::exception& e) {
    return fmt::format(
        "Error: Invalid local-asn value '{}': {}", asnStr, e.what());
  }

  // Set the local-asn in the BGP config session
  session.setLocalAsn(localAsn);
  session.saveConfig();

  return fmt::format(
      "Successfully set BGP local-asn to: {}\nConfig saved to: {}",
      localAsn,
      session.getSessionConfigPath());
}

void CmdConfigProtocolBgpGlobalLocalAsn::printOutput(const RetType& output) {
  std::cout << output << std::endl;
}

} // namespace facebook::fboss
