/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/sai/SaiBcmIcetea800bcPlatform.h"

#include "fboss/agent/hw/switch_asics/Tomahawk6Asic.h"
#include "fboss/agent/platforms/common/icetea800bc/Icetea800bcPlatformMapping.h"

#include <cstring>
namespace facebook::fboss {

SaiBcmIcetea800bcPlatform::SaiBcmIcetea800bcPlatform(
    std::unique_ptr<PlatformProductInfo> productInfo,
    folly::MacAddress localMac,
    const std::string& platformMappingStr)
    : SaiBcmPlatform(
          std::move(productInfo),
          platformMappingStr.empty()
              ? std::make_unique<Icetea800bcPlatformMapping>()
              : std::make_unique<Icetea800bcPlatformMapping>(
                    platformMappingStr),
          localMac) {}

void SaiBcmIcetea800bcPlatform::setupAsic(
    std::optional<int64_t> switchId,
    const cfg::SwitchInfo& switchInfo,
    std::optional<HwAsic::FabricNodeRole> fabricNodeRole) {
  CHECK(!fabricNodeRole.has_value());
  asic_ = std::make_unique<Tomahawk6Asic>(switchId, switchInfo);
}

HwAsic* SaiBcmIcetea800bcPlatform::getAsic() const {
  return asic_.get();
}

SaiBcmIcetea800bcPlatform::~SaiBcmIcetea800bcPlatform() = default;

} // namespace facebook::fboss
