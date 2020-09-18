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
#include "fboss/agent/platforms/wedge/utils/BcmLedUtils.h"

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

bool BcmTestGalaxyPlatform::verifyLEDStatus(PortID port, bool up) {
  uint32_t value = BcmLedUtils::getGalaxyPortStatus(0, port);
  uint32_t expectedValue{0};
  expectedValue = GalaxyLedUtils::getExpectedLEDState(up, up);
  return (value == expectedValue) && ((value & 0x1) != 0) == (up == true);
}

} // namespace facebook::fboss
