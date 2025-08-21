/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/sai/SaiBcmWedge800baPlatform.h"

#include "fboss/agent/hw/switch_asics/Tomahawk5Asic.h"
#include "fboss/agent/platforms/common/wedge800ba/Wedge800baPlatformMapping.h"

#include <cstring>
namespace facebook::fboss {

SaiBcmWedge800baPlatform::SaiBcmWedge800baPlatform(
    std::unique_ptr<PlatformProductInfo> productInfo,
    folly::MacAddress localMac,
    const std::string& platformMappingStr)
    : SaiBcmPlatform(
          std::move(productInfo),
          platformMappingStr.empty()
              ? std::make_unique<Wedge800baPlatformMapping>()
              : std::make_unique<Wedge800baPlatformMapping>(platformMappingStr),
          localMac) {}

void SaiBcmWedge800baPlatform::setupAsic(
    std::optional<int64_t> switchId,
    const cfg::SwitchInfo& switchInfo,
    std::optional<HwAsic::FabricNodeRole> fabricNodeRole) {
  CHECK(!fabricNodeRole.has_value());
  asic_ = std::make_unique<Tomahawk5Asic>(switchId, switchInfo);
}

HwAsic* SaiBcmWedge800baPlatform::getAsic() const {
  return asic_.get();
}

SaiBcmWedge800baPlatform::~SaiBcmWedge800baPlatform() = default;

} // namespace facebook::fboss
