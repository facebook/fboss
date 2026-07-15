/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/sai/SaiBcmJ4SimPlatform.h"

#include "fboss/agent/hw/switch_asics/Jericho4Asic.h"
#include "fboss/agent/platforms/common/j4sim/J4SimPlatformMapping.h"

namespace facebook::fboss {

SaiBcmJ4SimPlatform::SaiBcmJ4SimPlatform(
    std::unique_ptr<PlatformProductInfo> productInfo,
    folly::MacAddress localMac,
    const std::string& platformMappingStr)
    : SaiBcmPlatform(
          std::move(productInfo),
          platformMappingStr.empty()
              ? std::make_unique<J4SimPlatformMapping>()
              : std::make_unique<J4SimPlatformMapping>(platformMappingStr),
          localMac) {}

void SaiBcmJ4SimPlatform::setupAsic(
    std::optional<int64_t> switchId,
    const cfg::SwitchInfo& switchInfo,
    std::optional<HwAsic::FabricNodeRole> fabricNodeRole) {
  CHECK(!fabricNodeRole.has_value());
  asic_ = std::make_unique<Jericho4Asic>(switchId, switchInfo);
}

HwAsic* SaiBcmJ4SimPlatform::getAsic() const {
  return asic_.get();
}

SaiBcmJ4SimPlatform::~SaiBcmJ4SimPlatform() = default;

} // namespace facebook::fboss
