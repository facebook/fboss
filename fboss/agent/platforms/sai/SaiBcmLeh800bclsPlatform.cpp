/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/sai/SaiBcmLeh800bclsPlatform.h"

#include "fboss/agent/hw/switch_asics/Tomahawk6Asic.h"
#include "fboss/agent/platforms/common/leh800bcls/Leh800bclsPlatformMapping.h"

#include <cstring>
namespace facebook::fboss {

SaiBcmLeh800bclsPlatform::SaiBcmLeh800bclsPlatform(
    std::unique_ptr<PlatformProductInfo> productInfo,
    folly::MacAddress localMac,
    const std::string& platformMappingStr)
    : SaiBcmPlatform(
          std::move(productInfo),
          platformMappingStr.empty()
              ? std::make_unique<Leh800bclsPlatformMapping>()
              : std::make_unique<Leh800bclsPlatformMapping>(platformMappingStr),
          localMac) {}

void SaiBcmLeh800bclsPlatform::setupAsic(
    std::optional<int64_t> switchId,
    const cfg::SwitchInfo& switchInfo,
    std::optional<HwAsic::FabricNodeRole> fabricNodeRole) {
  CHECK(!fabricNodeRole.has_value());
  asic_ = std::make_unique<Tomahawk6Asic>(switchId, switchInfo);
}

HwAsic* SaiBcmLeh800bclsPlatform::getAsic() const {
  return asic_.get();
}

SaiBcmLeh800bclsPlatform::~SaiBcmLeh800bclsPlatform() = default;

} // namespace facebook::fboss
