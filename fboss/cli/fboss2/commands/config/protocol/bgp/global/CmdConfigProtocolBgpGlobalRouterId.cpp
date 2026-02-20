/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/protocol/bgp/global/CmdConfigProtocolBgpGlobalRouterId.h"

#include <fmt/core.h>
#include "fboss/cli/fboss2/commands/config/protocol/bgp/BgpConfigSession.h"

namespace facebook::fboss {

CmdConfigProtocolBgpGlobalRouterIdTraits::RetType
CmdConfigProtocolBgpGlobalRouterId::queryClient(
    const HostInfo& /* hostInfo */,
    const ObjectArgType& routerIdList) {
  auto& session = BgpConfigSession::getInstance();

  if (routerIdList.data().empty()) {
    return "Error: router-id is required";
  }

  // Extract the first IP from the list as router-id
  std::string routerId = routerIdList.data()[0];

  // Set the router-id in the BGP config session
  session.setRouterId(routerId);
  session.saveConfig();

  return fmt::format(
      "Successfully set BGP router-id to: {}\nConfig saved to: {}",
      routerId,
      session.getSessionConfigPath());
}

void CmdConfigProtocolBgpGlobalRouterId::printOutput(const RetType& output) {
  std::cout << output << std::endl;
}

} // namespace facebook::fboss
