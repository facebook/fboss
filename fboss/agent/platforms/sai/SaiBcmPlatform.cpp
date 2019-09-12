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

#include <cstdio>
#include <cstring>
namespace facebook {
namespace fboss {

std::string SaiBcmPlatform::getHwConfig() {
  auto platformConfig = config()->thrift.get_platform();
  if (!platformConfig) {
    throw FbossError(
        "failed to generate hw config file. platform config does not exist");
  }
  auto& config = platformConfig->chip.get_bcm().config;
  std::vector<std::string> nameValStrs;
  for (const auto& entry : config) {
    nameValStrs.emplace_back(
        folly::to<std::string>(entry.first, '=', entry.second));
  }
  auto hwConfig = folly::join('\n', nameValStrs);
  return hwConfig;
}
} // namespace fboss
} // namespace facebook
