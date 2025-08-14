/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/sai/SaiGlath05a-64oPlatform.h"

#include "fboss/agent/hw/switch_asics/Tomahawk5Asic.h"
#include "fboss/agent/platforms/common/glath05a-64o/Glath05a-64oPlatformMapping.h"

#include <cstdio>
#include <cstring>
namespace facebook::fboss {

SaiGlath05a_64oPlatform::SaiGlath05a_64oPlatform(
    std::unique_ptr<PlatformProductInfo> productInfo,
    folly::MacAddress localMac,
    const std::string& platformMappingStr)
    : SaiBcmPlatform(
          std::move(productInfo),
          platformMappingStr.empty()
              ? std::make_unique<Glath05a_64oPlatformMapping>()
              : std::make_unique<Glath05a_64oPlatformMapping>(platformMappingStr),
          localMac) {}

void SaiGlath05a_64oPlatform::setupAsic(
    std::optional<int64_t> switchId,
    const cfg::SwitchInfo& switchInfo,
    std::optional<HwAsic::FabricNodeRole> fabricNodeRole) {
  CHECK(!fabricNodeRole.has_value());
  asic_ = std::make_unique<Tomahawk5Asic>(switchId, switchInfo);
}

HwAsic* SaiGlath05a_64oPlatform::getAsic() const {
  return asic_.get();
}

SaiGlath05a_64oPlatform::~SaiGlath05a_64oPlatform() = default;

} // namespace facebook::fboss
