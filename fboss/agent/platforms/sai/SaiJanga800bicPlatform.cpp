/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/sai/SaiJanga800bicPlatform.h"

#include "fboss/agent/hw/switch_asics/Jericho3Asic.h"
#include "fboss/agent/platforms/common/janga800bic/Janga800bicPlatformMapping.h"

namespace facebook::fboss {

SaiJanga800bicPlatform::SaiJanga800bicPlatform(
    std::unique_ptr<PlatformProductInfo> productInfo,
    folly::MacAddress localMac,
    const std::string& platformMappingStr)
    : SaiBcmPlatform(
          std::move(productInfo),
          platformMappingStr.empty()
              ? std::make_unique<Janga800bicPlatformMapping>()
              : std::make_unique<Janga800bicPlatformMapping>(
                    platformMappingStr),
          localMac) {}

void SaiJanga800bicPlatform::setupAsic(
    std::optional<int64_t> switchId,
    const cfg::SwitchInfo& switchInfo,
    std::optional<HwAsic::FabricNodeRole> fabricNodeRole) {
  CHECK(!fabricNodeRole.has_value());
  asic_ = std::make_unique<Jericho3Asic>(switchId, switchInfo);
}

HwAsic* SaiJanga800bicPlatform::getAsic() const {
  return asic_.get();
}

SaiJanga800bicPlatform::~SaiJanga800bicPlatform() = default;

} // namespace facebook::fboss
