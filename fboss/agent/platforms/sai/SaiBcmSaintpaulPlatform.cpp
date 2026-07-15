/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/sai/SaiBcmSaintpaulPlatform.h"

#include "fboss/agent/hw/switch_asics/Jericho4Asic.h"
#include "fboss/agent/platforms/common/saintpaul/SaintpaulPlatformMapping.h"

namespace facebook::fboss {

SaiBcmSaintpaulPlatform::SaiBcmSaintpaulPlatform(
    std::unique_ptr<PlatformProductInfo> productInfo,
    folly::MacAddress localMac,
    const std::string& platformMappingStr)
    : SaiBcmPlatform(
          std::move(productInfo),
          platformMappingStr.empty()
              ? std::make_unique<SaintpaulPlatformMapping>()
              : std::make_unique<SaintpaulPlatformMapping>(platformMappingStr),
          localMac) {}

void SaiBcmSaintpaulPlatform::setupAsic(
    std::optional<int64_t> switchId,
    const cfg::SwitchInfo& switchInfo,
    std::optional<HwAsic::FabricNodeRole> fabricNodeRole) {
  CHECK(!fabricNodeRole.has_value());
  asic_ = std::make_unique<Jericho4Asic>(switchId, switchInfo);
}

HwAsic* SaiBcmSaintpaulPlatform::getAsic() const {
  return asic_.get();
}

SaiBcmSaintpaulPlatform::~SaiBcmSaintpaulPlatform() = default;

} // namespace facebook::fboss
