/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/sai/SaiBcmPlatform.h"
#include "fboss/lib/config/PlatformConfigUtils.h"

#include <cstdio>
#include <cstring>
namespace facebook::fboss {

std::string SaiBcmPlatform::getHwConfig() {
  auto& cfg = config()->thrift.platform.chip.get_bcm().config;
  std::vector<std::string> nameValStrs;
  for (const auto& entry : cfg) {
    nameValStrs.emplace_back(
        folly::to<std::string>(entry.first, '=', entry.second));
  }
  auto hwConfig = folly::join('\n', nameValStrs);
  return hwConfig;
}

std::vector<PortID> SaiBcmPlatform::getAllPortsInGroup(PortID portID) const {
  std::vector<PortID> allPortsinGroup;
  if (const auto& platformPorts = getPlatformPorts(); !platformPorts.empty()) {
    const auto& portList =
        utility::getPlatformPortsByControllingPort(platformPorts, portID);
    for (const auto& port : portList) {
      allPortsinGroup.push_back(PortID(port.mapping.id));
    }
  }
  return allPortsinGroup;
}

} // namespace facebook::fboss
