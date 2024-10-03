/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/sai/SaiBcmElbertPlatform.h"
#include "fboss/agent/platforms/common/elbert/ElbertPlatformMapping.h"

#include <cstdio>
#include <cstring>
namespace facebook::fboss {

SaiBcmElbertPlatform::SaiBcmElbertPlatform(
    std::unique_ptr<PlatformProductInfo> productInfo,
    folly::MacAddress localMac,
    const std::string& platformMappingStr)
    : SaiBcmPlatform(
          std::move(productInfo),
          std::make_unique<ElbertPlatformMapping>(platformMappingStr),
          localMac) {}

void SaiBcmElbertPlatform::setupAsic(
    cfg::SwitchType switchType,
    std::optional<int64_t> switchId,
    int16_t switchIndex,
    std::optional<cfg::Range64> systemPortRange,
    folly::MacAddress& mac,
    std::optional<HwAsic::FabricNodeRole> fabricNodeRole) {
  CHECK(!fabricNodeRole.has_value());
  asic_ = std::make_unique<Tomahawk4Asic>(
      switchType, switchId, switchIndex, systemPortRange, mac);
}
HwAsic* SaiBcmElbertPlatform::getAsic() const {
  return asic_.get();
}

SaiBcmElbertPlatform::~SaiBcmElbertPlatform() = default;

} // namespace facebook::fboss
