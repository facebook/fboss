/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/sai/SaiKametPlatform.h"

#include "fboss/agent/hw/switch_asics/RamonAsic.h"
#include "fboss/agent/platforms/common/kamet/KametPlatformMapping.h"

#include <cstdio>
#include <cstring>
namespace facebook::fboss {

SaiKametPlatform::SaiKametPlatform(
    std::unique_ptr<PlatformProductInfo> productInfo,
    folly::MacAddress localMac,
    const std::string& platformMappingStr)
    : SaiBcmPlatform(
          std::move(productInfo),
          platformMappingStr.empty()
              ? std::make_unique<KametPlatformMapping>()
              : std::make_unique<KametPlatformMapping>(platformMappingStr),
          localMac) {}

void SaiKametPlatform::setupAsic(
    cfg::SwitchType switchType,
    std::optional<int64_t> switchId,
    std::optional<cfg::Range64> systemPortRange) {
  asic_ = std::make_unique<RamonAsic>(switchType, switchId, systemPortRange);
}

HwAsic* SaiKametPlatform::getAsic() const {
  return asic_.get();
}

SaiKametPlatform::~SaiKametPlatform() {}

} // namespace facebook::fboss
