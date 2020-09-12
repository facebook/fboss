/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/platforms/tests/utils/BcmTestGalaxyPlatform.h"

#include "fboss/agent/platforms/common/PlatformProductInfo.h"
#include "fboss/agent/platforms/common/galaxy/GalaxyFCPlatformMapping.h"
#include "fboss/agent/platforms/common/galaxy/GalaxyLCPlatformMapping.h"
#include "fboss/agent/platforms/tests/utils/BcmTestGalaxyPort.h"

#include "fboss/agent/platforms/common/utils/GalaxyLedUtils.h"

namespace facebook::fboss {
std::unique_ptr<BcmTestPort> BcmTestGalaxyPlatform::createTestPort(PortID id) {
  return std::make_unique<BcmTestGalaxyPort>(id, this);
}

BcmTestGalaxyLCPlatform::BcmTestGalaxyLCPlatform(
    std::unique_ptr<PlatformProductInfo> productInfo)
    : BcmTestGalaxyPlatform(
          std::move(productInfo),
          std::make_unique<GalaxyLCPlatformMapping>("lc101")) {}

BcmTestGalaxyFCPlatform::BcmTestGalaxyFCPlatform(
    std::unique_ptr<PlatformProductInfo> productInfo)
    : BcmTestGalaxyPlatform(
          std::move(productInfo),
          std::make_unique<GalaxyFCPlatformMapping>("fc001")) {}

void BcmTestGalaxyPlatform::initLEDs(int unit) {
  BcmTestPlatform::initLEDs(
      unit,
      GalaxyLedUtils::defaultLed0Code(),
      GalaxyLedUtils::defaultLed1Code());
}
} // namespace facebook::fboss
