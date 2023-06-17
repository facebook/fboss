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

void SaiMeru800bfaPlatform::setupAsic(
    cfg::SwitchType switchType,
    std::optional<int64_t> switchId,
    std::optional<cfg::Range64> systemPortRange,
    folly::MacAddress& mac) {
  asic_ =
      std::make_unique<Ramon3Asic>(switchType, switchId, systemPortRange, mac);
}

HwAsic* SaiMeru800bfaPlatform::getAsic() const {
  return asic_.get();
}

SaiMeru800bfaPlatform::~SaiMeru800bfaPlatform() {}

} // namespace facebook::fboss
