/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/platforms/common/PlatformProductInfo.h"

namespace facebook::fboss {

void PlatformProductInfo::initFromFbWhoAmI() {}

void PlatformProductInfo::setFBSerial() {}

std::optional<PlatformMode> PlatformProductInfo::getDevPlatformMode() {
  return std::nullopt;
}

} // namespace facebook::fboss
