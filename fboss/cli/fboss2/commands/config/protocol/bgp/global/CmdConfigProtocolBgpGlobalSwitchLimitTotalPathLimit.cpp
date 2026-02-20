/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/protocol/bgp/global/CmdConfigProtocolBgpGlobalSwitchLimitTotalPathLimit.h"

#include <fmt/core.h>
#include "fboss/cli/fboss2/commands/config/protocol/bgp/BgpConfigSession.h"

namespace facebook::fboss {

CmdConfigProtocolBgpGlobalSwitchLimitTotalPathLimitTraits::RetType
CmdConfigProtocolBgpGlobalSwitchLimitTotalPathLimit::queryClient(
    const HostInfo& /* hostInfo */,
    const ObjectArgType& limitStr) {
  auto& session = BgpConfigSession::getInstance();

  if (limitStr.data().empty()) {
    return "Error: total-path-limit value is required";
  }

  int64_t limit = std::stoll(limitStr.data()[0]);

  session.setSwitchLimitTotalPathLimit(limit);
  session.saveConfig();

  return fmt::format(
      "Successfully set switch_limit_config total_path_limit to: {}\nConfig saved to: {}",
      limit,
      session.getSessionConfigPath());
}

void CmdConfigProtocolBgpGlobalSwitchLimitTotalPathLimit::printOutput(
    const RetType& output) {
  std::cout << output << std::endl;
}

} // namespace facebook::fboss
