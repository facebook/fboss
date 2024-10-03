/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/sai/SaiTahan800bcPlatform.h"

#include "fboss/agent/hw/switch_asics/Tomahawk5Asic.h"
#include "fboss/agent/platforms/common/tahan800bc/Tahan800bcPlatformMapping.h"

#include <cstdio>
#include <cstring>
namespace facebook::fboss {

SaiTahan800bcPlatform::SaiTahan800bcPlatform(
    std::unique_ptr<PlatformProductInfo> productInfo,
    folly::MacAddress localMac,
    const std::string& platformMappingStr)
    : SaiBcmPlatform(
          std::move(productInfo),
          platformMappingStr.empty()
              ? std::make_unique<Tahan800bcPlatformMapping>()
              : std::make_unique<Tahan800bcPlatformMapping>(platformMappingStr),
          localMac) {}

void SaiTahan800bcPlatform::setupAsic(
    cfg::SwitchType switchType,
    std::optional<int64_t> switchId,
    int16_t switchIndex,
    std::optional<cfg::Range64> systemPortRange,
    folly::MacAddress& mac,
    std::optional<HwAsic::FabricNodeRole> fabricNodeRole) {
  CHECK(!fabricNodeRole.has_value());
  asic_ = std::make_unique<Tomahawk5Asic>(
      switchType, switchId, switchIndex, systemPortRange, mac);
}

HwAsic* SaiTahan800bcPlatform::getAsic() const {
  return asic_.get();
}

SaiTahan800bcPlatform::~SaiTahan800bcPlatform() = default;

} // namespace facebook::fboss
