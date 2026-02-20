/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license
 * found in the LICENSE file in the root directory of this
 * source tree. An additional grant of patent rights can be
 * found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/platforms/sai/SaiBcmMinipack3BTAPlatform.h"
#include "fboss/agent/hw/switch_asics/TomahawkUltra1Asic.h"
#include "fboss/agent/platforms/common/minipack3bta/Minipack3BTAPlatformMapping.h"

#include <cstring>
namespace facebook::fboss {

SaiBcmMinipack3BTAPlatform::SaiBcmMinipack3BTAPlatform(
    std::unique_ptr<PlatformProductInfo> productInfo,
    folly::MacAddress localMac,
    const std::string& platformMappingStr)
    : SaiBcmPlatform(
          std::move(productInfo),
          platformMappingStr.empty()
              ? std::make_unique<Minipack3BTAPlatformMapping>()
              : std::make_unique<Minipack3BTAPlatformMapping>(
                    platformMappingStr),
          localMac) {}

void SaiBcmMinipack3BTAPlatform::setupAsic(
    std::optional<int64_t> switchId,
    const cfg::SwitchInfo& switchInfo,
    std::optional<HwAsic::FabricNodeRole> fabricNodeRole) {
  CHECK(!fabricNodeRole.has_value());
  asic_ = std::make_unique<TomahawkUltra1Asic>(switchId, switchInfo);
}

HwAsic* SaiBcmMinipack3BTAPlatform::getAsic() const {
  return asic_.get();
}

SaiBcmMinipack3BTAPlatform::~SaiBcmMinipack3BTAPlatform() = default;

} // namespace facebook::fboss
