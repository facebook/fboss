/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/sai/SaiMeru800bfaPlatform.h"

#include "fboss/agent/hw/switch_asics/Ramon3Asic.h"
#include "fboss/agent/platforms/common/meru800bfa/Meru800bfaP1PlatformMapping.h"
#include "fboss/agent/platforms/common/meru800bfa/Meru800bfaPlatformMapping.h"

#include <cstdio>
#include <cstring>
namespace facebook::fboss {

SaiMeru800bfaPlatform::SaiMeru800bfaPlatform(
    std::unique_ptr<PlatformProductInfo> productInfo,
    folly::MacAddress localMac,
    const std::string& platformMappingStr)
    : SaiBcmPlatform(
          std::move(productInfo),
          platformMappingStr.empty()
              ? std::make_unique<Meru800bfaPlatformMapping>()
              : std::make_unique<Meru800bfaPlatformMapping>(platformMappingStr),
          localMac) {}

SaiMeru800bfaPlatform::SaiMeru800bfaPlatform(
    std::unique_ptr<PlatformProductInfo> productInfo,
    std::unique_ptr<Meru800bfaP1PlatformMapping> mapping,
    folly::MacAddress localMac)
    : SaiBcmPlatform(std::move(productInfo), std::move(mapping), localMac) {}

void SaiMeru800bfaPlatform::setupAsic(
    cfg::SwitchType switchType,
    std::optional<int64_t> switchId,
    int16_t switchIndex,
    std::optional<cfg::Range64> systemPortRange,
    folly::MacAddress& mac,
    std::optional<HwAsic::FabricNodeRole> fabricNodeRole) {
  CHECK(fabricNodeRole.has_value());
  asic_ = std::make_unique<Ramon3Asic>(
      switchType,
      switchId,
      switchIndex,
      systemPortRange,
      mac,
      std::nullopt,
      *fabricNodeRole);
}

HwAsic* SaiMeru800bfaPlatform::getAsic() const {
  return asic_.get();
}

SaiMeru800bfaPlatform::~SaiMeru800bfaPlatform() = default;

SaiMeru800P1bfaPlatform::SaiMeru800P1bfaPlatform(
    std::unique_ptr<PlatformProductInfo> productInfo,
    folly::MacAddress localMac,
    const std::string& platformMappingStr)
    : SaiMeru800bfaPlatform(
          std::move(productInfo),
          platformMappingStr.empty()
              ? std::make_unique<Meru800bfaP1PlatformMapping>()
              : std::make_unique<Meru800bfaP1PlatformMapping>(
                    platformMappingStr),
          localMac) {}

} // namespace facebook::fboss
