/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/protocol/bgp/global/CmdConfigProtocolBgpGlobalHoldTime.h"

#include <fmt/core.h>
#include <folly/Conv.h>
#include "fboss/cli/fboss2/commands/config/protocol/bgp/BgpConfigSession.h"

namespace facebook::fboss {

CmdConfigProtocolBgpGlobalHoldTimeTraits::RetType
CmdConfigProtocolBgpGlobalHoldTime::queryClient(
    const HostInfo& /* hostInfo */,
    const ObjectArgType& holdTimeMsg) {
  auto& session = BgpConfigSession::getInstance();

  if (holdTimeMsg.data().empty()) {
    return "Error: hold-time value is required";
  }

  // Extract string and parse to uint32_t
  std::string holdTimeStr = holdTimeMsg.data()[0];
  uint32_t holdTime;
  try {
    holdTime = folly::to<uint32_t>(holdTimeStr);
  } catch (const std::exception& e) {
    return fmt::format(
        "Error: Invalid hold-time value '{}': {}", holdTimeStr, e.what());
  }

  // Set the hold-time in the BGP config session
  session.setHoldTime(holdTime);
  session.saveConfig();

  return fmt::format(
      "Successfully set BGP hold-time to: {} seconds\nConfig saved to: {}",
      holdTime,
      session.getSessionConfigPath());
}

void CmdConfigProtocolBgpGlobalHoldTime::printOutput(const RetType& output) {
  std::cout << output << std::endl;
}

} // namespace facebook::fboss
