/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/sai/SaiBcmLadakh800bclsPlatform.h"

#include "fboss/agent/hw/switch_asics/Tomahawk6Asic.h"
#include "fboss/agent/platforms/common/ladakh800bcls/Ladakh800bclsPlatformMapping.h"

#include <cstring>
namespace facebook::fboss {

SaiBcmLadakh800bclsPlatform::SaiBcmLadakh800bclsPlatform(
    std::unique_ptr<PlatformProductInfo> productInfo,
    folly::MacAddress localMac,
    const std::string& platformMappingStr)
    : SaiBcmPlatform(
          std::move(productInfo),
          platformMappingStr.empty()
              ? std::make_unique<Ladakh800bclsPlatformMapping>()
              : std::make_unique<Ladakh800bclsPlatformMapping>(
                    platformMappingStr),
          localMac) {}

void SaiBcmLadakh800bclsPlatform::setupAsic(
    std::optional<int64_t> switchId,
    const cfg::SwitchInfo& switchInfo,
    std::optional<HwAsic::FabricNodeRole> fabricNodeRole) {
  CHECK(!fabricNodeRole.has_value());
  asic_ = std::make_unique<Tomahawk6Asic>(switchId, switchInfo);
}

HwAsic* SaiBcmLadakh800bclsPlatform::getAsic() const {
  return asic_.get();
}

SaiBcmLadakh800bclsPlatform::~SaiBcmLadakh800bclsPlatform() = default;

} // namespace facebook::fboss
