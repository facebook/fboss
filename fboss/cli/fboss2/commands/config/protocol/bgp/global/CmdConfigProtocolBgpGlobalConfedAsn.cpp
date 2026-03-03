/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/protocol/bgp/global/CmdConfigProtocolBgpGlobalConfedAsn.h"

#include <fmt/core.h>
#include <folly/Conv.h>
#include "fboss/cli/fboss2/commands/config/protocol/bgp/BgpConfigSession.h"

namespace facebook::fboss {

CmdConfigProtocolBgpGlobalConfedAsnTraits::RetType
CmdConfigProtocolBgpGlobalConfedAsn::queryClient(
    const HostInfo& /* hostInfo */,
    const ObjectArgType& confedAsnMsg) {
  auto& session = BgpConfigSession::getInstance();

  if (confedAsnMsg.data().empty()) {
    return "Error: confed-asn value is required";
  }

  std::string asnStr = confedAsnMsg.data()[0];
  uint64_t confedAsn;
  try {
    confedAsn = folly::to<uint64_t>(asnStr);
  } catch (const std::exception& e) {
    return fmt::format("Error: Invalid confed-asn '{}': {}", asnStr, e.what());
  }

  session.setConfedAsn(confedAsn);
  session.saveConfig();

  return fmt::format(
      "Successfully set BGP confederation AS number to: {}\nConfig saved to: {}",
      confedAsn,
      session.getSessionConfigPath());
}

void CmdConfigProtocolBgpGlobalConfedAsn::printOutput(const RetType& output) {
  std::cout << output << std::endl;
}

} // namespace facebook::fboss
