/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/sai/SaiBcmBlackwolf800banwPlatform.h"

#include "fboss/agent/hw/switch_asics/Qumran4DAsic.h"
#include "fboss/agent/platforms/common/blackwolf800banw/Blackwolf800banwPlatformMapping.h"

namespace facebook::fboss {

SaiBcmBlackwolf800banwPlatform::SaiBcmBlackwolf800banwPlatform(
    std::unique_ptr<PlatformProductInfo> productInfo,
    folly::MacAddress localMac,
    const std::string& platformMappingStr)
    : SaiBcmPlatform(
          std::move(productInfo),
          platformMappingStr.empty()
              ? std::make_unique<Blackwolf800banwPlatformMapping>()
              : std::make_unique<Blackwolf800banwPlatformMapping>(
                    platformMappingStr),
          localMac) {}

void SaiBcmBlackwolf800banwPlatform::setupAsic(
    std::optional<int64_t> switchId,
    const cfg::SwitchInfo& switchInfo,
    std::optional<HwAsic::FabricNodeRole> fabricNodeRole) {
  CHECK(!fabricNodeRole.has_value());
  asic_ = std::make_unique<Qumran4DAsic>(switchId, switchInfo);
}

HwAsic* SaiBcmBlackwolf800banwPlatform::getAsic() const {
  return asic_.get();
}

SaiBcmBlackwolf800banwPlatform::~SaiBcmBlackwolf800banwPlatform() = default;

} // namespace facebook::fboss
