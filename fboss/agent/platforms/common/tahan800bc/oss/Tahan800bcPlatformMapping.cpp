/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/common/tahan800bc/Tahan800bcPlatformMapping.h"
#include "fboss/agent/platforms/common/tahan800bc/Tahan800bcTestFixturePlatformMapping.h"

namespace facebook::fboss {
const std::string Tahan800bcPlatformMapping::getPlatformMappingStr() {
  return kJsonTestFixturePlatformMappingStr;
}

} // namespace facebook::fboss
