/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/platforms/sai/SaiBcmGalaxyPlatform.h"

#include "fboss/agent/platforms/common/utils/GalaxyLedUtils.h"

#include <folly/FileUtil.h>
#include <folly/container/Array.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

namespace facebook::fboss {
void SaiBcmGalaxyPlatform::initLEDs() {
  initWedgeLED(0, GalaxyLedUtils::defaultLed0Code());
  initWedgeLED(1, GalaxyLedUtils::defaultLed1Code());
}
} // namespace facebook::fboss
