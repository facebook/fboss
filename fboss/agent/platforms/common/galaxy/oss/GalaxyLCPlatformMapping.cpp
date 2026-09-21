/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/common/galaxy/GalaxyLCPlatformMapping.h"

namespace facebook {
namespace fboss {

std::string GalaxyLCPlatformMapping::getLinecardName() {
  auto defaultLCName = "lc101";
  return defaultLCName;
}
} // namespace fboss
} // namespace facebook
