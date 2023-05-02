/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/sai/SaiBcmGalaxyLCPlatform.h"
#include "fboss/agent/hw/switch_asics/TomahawkAsic.h"
#include "fboss/agent/platforms/common/galaxy/GalaxyLCPlatformMapping.h"

namespace facebook::fboss {

SaiBcmGalaxyLCPlatform::SaiBcmGalaxyLCPlatform(
    std::unique_ptr<PlatformProductInfo> productInfo,
    folly::MacAddress localMac,
    const std::string& /* platformMappingStr */)
    : SaiBcmGalaxyPlatform(
          std::move(productInfo),
          std::make_unique<GalaxyLCPlatformMapping>(
              GalaxyLCPlatformMapping::getLinecardName()),
          localMac) {}

void SaiBcmGalaxyLCPlatform::setupAsic(
    cfg::SwitchType switchType,
    std::optional<int64_t> switchId,
    std::optional<cfg::Range64> systemPortRange,
    folly::MacAddress& mac) {
  asic_ = std::make_unique<TomahawkAsic>(
      switchType, switchId, systemPortRange, mac);
}

HwAsic* SaiBcmGalaxyLCPlatform::getAsic() const {
  return asic_.get();
}

SaiBcmGalaxyLCPlatform::~SaiBcmGalaxyLCPlatform() {}
} // namespace facebook::fboss
