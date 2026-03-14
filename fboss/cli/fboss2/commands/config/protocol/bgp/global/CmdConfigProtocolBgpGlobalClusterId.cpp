/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/protocol/bgp/global/CmdConfigProtocolBgpGlobalClusterId.h"

#include <fmt/core.h>
#include "fboss/cli/fboss2/commands/config/protocol/bgp/BgpConfigSession.h"

namespace facebook::fboss {

CmdConfigProtocolBgpGlobalClusterIdTraits::RetType
CmdConfigProtocolBgpGlobalClusterId::queryClient(
    const HostInfo& /* hostInfo */,
    const ObjectArgType& clusterIdList) {
  auto& session = BgpConfigSession::getInstance();

  if (clusterIdList.data().empty()) {
    return "Error: cluster-id is required";
  }

  std::string clusterId = clusterIdList.data()[0];
  session.setClusterId(clusterId);
  session.saveConfig();

  return fmt::format(
      "Successfully set BGP cluster-id to: {}\nConfig saved to: {}",
      clusterId,
      session.getSessionConfigPath());
}

void CmdConfigProtocolBgpGlobalClusterId::printOutput(const RetType& output) {
  std::cout << output << std::endl;
}

} // namespace facebook::fboss
