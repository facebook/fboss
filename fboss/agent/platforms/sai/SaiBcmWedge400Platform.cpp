/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/sai/SaiBcmWedge400Platform.h"

#include <cstdio>
#include <cstring>
#include "fboss/agent/hw/switch_asics/Tomahawk3Asic.h"
#include "fboss/agent/platforms/common/wedge400/Wedge400GrandTetonPlatformMapping.h"
#include "fboss/agent/platforms/common/wedge400/Wedge400PlatformMapping.h"
#include "fboss/agent/platforms/common/wedge400/Wedge400PlatformUtil.h"
namespace facebook::fboss {

SaiBcmWedge400Platform::SaiBcmWedge400Platform(
    std::unique_ptr<PlatformProductInfo> productInfo,
    PlatformType type,
    folly::MacAddress localMac,
    const std::string& platformMappingStr)
    : SaiBcmPlatform(
          std::move(productInfo),
          createWedge400PlatformMapping(type, platformMappingStr),
          localMac) {}

void SaiBcmWedge400Platform::setupAsic(
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

HwAsic* SaiBcmWedge400Platform::getAsic() const {
  return asic_.get();
}

void SaiBcmWedge400Platform::initLEDs() {
  // TODO skhare
}

SaiBcmWedge400Platform::~SaiBcmWedge400Platform() = default;

std::unique_ptr<PlatformMapping>
SaiBcmWedge400Platform::createWedge400PlatformMapping(
    PlatformType type,
    const std::string& platformMappingStr) {
  if (utility::isWedge400PlatformRackTypeInference() ||
      type == PlatformType::PLATFORM_WEDGE400_GRANDTETON) {
    return platformMappingStr.empty()
        ? std::make_unique<Wedge400GrandTetonPlatformMapping>()
        : std::make_unique<Wedge400GrandTetonPlatformMapping>(
              platformMappingStr);
  }
  return platformMappingStr.empty()
      ? std::make_unique<Wedge400PlatformMapping>()
      : std::make_unique<Wedge400PlatformMapping>(platformMappingStr);
}

} // namespace facebook::fboss
