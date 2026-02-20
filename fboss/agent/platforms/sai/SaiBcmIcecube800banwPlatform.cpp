/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/sai/SaiBcmIcecube800banwPlatform.h"

#include "fboss/agent/hw/switch_asics/Tomahawk6Asic.h"
#include "fboss/agent/platforms/common/icecube800banw/Icecube800banwPlatformMapping.h"

#include <cstring>
namespace facebook::fboss {

SaiBcmIcecube800banwPlatform::SaiBcmIcecube800banwPlatform(
    std::unique_ptr<PlatformProductInfo> productInfo,
    folly::MacAddress localMac,
    const std::string& platformMappingStr)
    : SaiBcmPlatform(
          std::move(productInfo),
          platformMappingStr.empty()
              ? std::make_unique<Icecube800banwPlatformMapping>()
              : std::make_unique<Icecube800banwPlatformMapping>(
                    platformMappingStr),
          localMac) {}

void SaiBcmIcecube800banwPlatform::setupAsic(
    std::optional<int64_t> switchId,
    const cfg::SwitchInfo& switchInfo,
    std::optional<HwAsic::FabricNodeRole> fabricNodeRole) {
  CHECK(!fabricNodeRole.has_value());
  asic_ = std::make_unique<Tomahawk6Asic>(switchId, switchInfo);
}

HwAsic* SaiBcmIcecube800banwPlatform::getAsic() const {
  return asic_.get();
}

SaiBcmIcecube800banwPlatform::~SaiBcmIcecube800banwPlatform() = default;

} // namespace facebook::fboss
