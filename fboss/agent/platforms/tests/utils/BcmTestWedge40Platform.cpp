/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/platforms/tests/utils/BcmTestWedge40Platform.h"

#include "fboss/agent/hw/bcm/BcmCosQueueManagerUtils.h"
#include "fboss/agent/hw/switch_asics/Trident2Asic.h"
#include "fboss/agent/platforms/common/wedge40/Wedge40PlatformMapping.h"
#include "fboss/agent/platforms/tests/utils/BcmTestWedge40Port.h"
#include "fboss/lib/platforms/PlatformProductInfo.h"

#include "fboss/agent/platforms/common/utils/Wedge40LedUtils.h"
#include "fboss/agent/platforms/wedge/utils/BcmLedUtils.h"

namespace facebook::fboss {

BcmTestWedge40Platform::BcmTestWedge40Platform(
    std::unique_ptr<PlatformProductInfo> productInfo)
    : BcmTestWedgePlatform(
          std::move(productInfo),
          std::make_unique<Wedge40PlatformMapping>()) {}

void BcmTestWedge40Platform::setupAsic(
    cfg::SwitchType switchType,
    std::optional<int64_t> switchId,
    std::optional<cfg::Range64> systemPortRange,
    folly::MacAddress& mac) {
  asic_ = std::make_unique<Trident2Asic>(
      switchType, switchId, systemPortRange, mac);
}

std::unique_ptr<BcmTestPort> BcmTestWedge40Platform::createTestPort(PortID id) {
  return std::make_unique<BcmTestWedge40Port>(id, this);
}

HwAsic* BcmTestWedge40Platform::getAsic() const {
  return asic_.get();
}

BcmTestWedge40Platform::~BcmTestWedge40Platform() {}

const PortQueue& BcmTestWedge40Platform::getDefaultPortQueueSettings(
    cfg::StreamType streamType) const {
  return utility::getDefaultPortQueueSettings(
      utility::BcmChip::TRIDENT2, streamType);
}

const PortQueue& BcmTestWedge40Platform::getDefaultControlPlaneQueueSettings(
    cfg::StreamType streamType) const {
  return utility::getDefaultControlPlaneQueueSettings(
      utility::BcmChip::TRIDENT2, streamType);
}

void BcmTestWedge40Platform::initLEDs(int unit) {
  BcmTestPlatform::initLEDs(
      unit,
      Wedge40LedUtils::defaultLedCode(),
      Wedge40LedUtils::defaultLedCode());
}

bool BcmTestWedge40Platform::verifyLEDStatus(PortID port, bool up) {
  uint32_t value = BcmLedUtils::getWedge40PortStatus(0, port);
  uint32_t expectedValue =
      static_cast<uint32_t>(Wedge40LedUtils::getExpectedLEDState(up, up));
  return (value == expectedValue) && (value != 0) == (up == true);
}

} // namespace facebook::fboss
