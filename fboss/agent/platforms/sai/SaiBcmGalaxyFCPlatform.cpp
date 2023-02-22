/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/sai/SaiBcmGalaxyFCPlatform.h"
#include "fboss/agent/hw/switch_asics/TomahawkAsic.h"
#include "fboss/agent/platforms/common/galaxy/GalaxyFCPlatformMapping.h"

namespace facebook::fboss {

SaiBcmGalaxyFCPlatform::SaiBcmGalaxyFCPlatform(
    std::unique_ptr<PlatformProductInfo> productInfo,
    folly::MacAddress localMac,
    const std::string& /* platformMappingStr */)
    : SaiBcmGalaxyPlatform(
          std::move(productInfo),
          std::make_unique<GalaxyFCPlatformMapping>(
              GalaxyFCPlatformMapping::getFabriccardName()),
          localMac) {}

void SaiBcmGalaxyFCPlatform::setupAsic(
    cfg::SwitchType switchType,
    std::optional<int64_t> switchId,
    std::optional<cfg::Range64> systemPortRange) {
  asic_ = std::make_unique<TomahawkAsic>(switchType, switchId, systemPortRange);
}
HwAsic* SaiBcmGalaxyFCPlatform::getAsic() const {
  return asic_.get();
}

SaiBcmGalaxyFCPlatform::~SaiBcmGalaxyFCPlatform() {}
} // namespace facebook::fboss
