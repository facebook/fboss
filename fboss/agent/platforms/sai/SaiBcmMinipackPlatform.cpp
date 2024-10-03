/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/sai/SaiBcmMinipackPlatform.h"

#include "fboss/agent/hw/switch_asics/Tomahawk3Asic.h"

#include <cstdio>
#include <cstring>
namespace facebook::fboss {

HwAsic* SaiBcmMinipackPlatform::getAsic() const {
  return asic_.get();
}

void SaiBcmMinipackPlatform::setupAsic(
    cfg::SwitchType switchType,
    std::optional<int64_t> switchId,
    int16_t switchIndex,
    std::optional<cfg::Range64> systemPortRange,
    folly::MacAddress& mac,
    std::optional<HwAsic::FabricNodeRole> fabricNodeRole) {
  CHECK(!fabricNodeRole.has_value());
  asic_ = std::make_unique<Tomahawk3Asic>(
      switchType, switchId, switchIndex, systemPortRange, mac);
}

void SaiBcmMinipackPlatform::initLEDs() {
  // TODO skhare
}

SaiBcmMinipackPlatform::~SaiBcmMinipackPlatform() = default;

} // namespace facebook::fboss
