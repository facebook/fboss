/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/sai/SaiBcmWedge100Platform.h"

#include "fboss/agent/hw/switch_asics/TomahawkAsic.h"
#include "fboss/agent/platforms/common/utils/Wedge100LedUtils.h"
#include "fboss/agent/platforms/common/wedge100/Wedge100PlatformMapping.h"

#include <cstdio>
#include <cstring>
namespace facebook::fboss {

SaiBcmWedge100Platform::SaiBcmWedge100Platform(
    std::unique_ptr<PlatformProductInfo> productInfo,
    folly::MacAddress localMac)
    : SaiBcmPlatform(
          std::move(productInfo),
          std::make_unique<Wedge100PlatformMapping>(),
          localMac) {}

void SaiBcmWedge100Platform::setupAsic(
    cfg::SwitchType switchType,
    std::optional<int64_t> switchId,
    std::optional<cfg::Range64> systemPortRange) {
  asic_ = std::make_unique<TomahawkAsic>(switchType, switchId, systemPortRange);
}
HwAsic* SaiBcmWedge100Platform::getAsic() const {
  return asic_.get();
}

SaiBcmWedge100Platform::~SaiBcmWedge100Platform() {}

void SaiBcmWedge100Platform::initLEDs() {
  // The LED mode can be reset by power cycle or by BMC push. Set the LED
  // mode every time we start agent to resolve this.
  Wedge100LedUtils::enableLedMode();
  initWedgeLED(0, Wedge100LedUtils::defaultLedCode());
  initWedgeLED(1, Wedge100LedUtils::defaultLedCode());
}
} // namespace facebook::fboss
