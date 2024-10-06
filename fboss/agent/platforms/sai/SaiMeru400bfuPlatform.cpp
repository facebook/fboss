/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/sai/SaiMeru400bfuPlatform.h"

#include "fboss/agent/hw/switch_asics/RamonAsic.h"
#include "fboss/agent/platforms/common/meru400bfu/Meru400bfuPlatformMapping.h"

#include <cstdio>
#include <cstring>
namespace facebook::fboss {

SaiMeru400bfuPlatform::SaiMeru400bfuPlatform(
    std::unique_ptr<PlatformProductInfo> productInfo,
    folly::MacAddress localMac,
    const std::string& platformMappingStr)
    : SaiBcmPlatform(
          std::move(productInfo),
          platformMappingStr.empty()
              ? std::make_unique<Meru400bfuPlatformMapping>()
              : std::make_unique<Meru400bfuPlatformMapping>(platformMappingStr),
          localMac) {}

void SaiMeru400bfuPlatform::setupAsic(
    cfg::SwitchType switchType,
    std::optional<int64_t> switchId,
    int16_t switchIndex,
    std::optional<cfg::Range64> systemPortRange,
    folly::MacAddress& mac,
    std::optional<HwAsic::FabricNodeRole> fabricNodeRole) {
  CHECK(fabricNodeRole.has_value());
  asic_ = std::make_unique<RamonAsic>(
      switchType,
      switchId,
      switchIndex,
      systemPortRange,
      mac,
      std::nullopt,
      *fabricNodeRole);
}

HwAsic* SaiMeru400bfuPlatform::getAsic() const {
  return asic_.get();
}

SaiMeru400bfuPlatform::~SaiMeru400bfuPlatform() = default;

} // namespace facebook::fboss
