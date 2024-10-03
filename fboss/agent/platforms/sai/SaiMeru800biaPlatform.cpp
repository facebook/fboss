/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/sai/SaiMeru800biaPlatform.h"

#include "fboss/agent/hw/switch_asics/Jericho3Asic.h"
#include "fboss/agent/platforms/common/meru800bia/Meru800biaPlatformMapping.h"

#include <cstdio>
#include <cstring>
namespace facebook::fboss {

SaiMeru800biaPlatform::SaiMeru800biaPlatform(
    std::unique_ptr<PlatformProductInfo> productInfo,
    folly::MacAddress localMac,
    const std::string& platformMappingStr)
    : SaiBcmPlatform(
          std::move(productInfo),
          platformMappingStr.empty()
              ? std::make_unique<Meru800biaPlatformMapping>()
              : std::make_unique<Meru800biaPlatformMapping>(platformMappingStr),
          localMac) {}

void SaiMeru800biaPlatform::setupAsic(
    cfg::SwitchType switchType,
    std::optional<int64_t> switchId,
    int16_t switchIndex,
    std::optional<cfg::Range64> systemPortRange,
    folly::MacAddress& mac,
    std::optional<HwAsic::FabricNodeRole> fabricNodeRole) {
  CHECK(!fabricNodeRole.has_value());
  asic_ = std::make_unique<Jericho3Asic>(
      switchType, switchId, switchIndex, systemPortRange, mac);
}

HwAsic* SaiMeru800biaPlatform::getAsic() const {
  return asic_.get();
}

std::vector<sai_system_port_config_t>
SaiMeru800biaPlatform::getInternalSystemPortConfig() const {
  CHECK(asic_) << " Asic must be set before getting sys port info";
  CHECK(asic_->getSwitchId()) << " Switch Id must be set before sys port info";
  return {{0, static_cast<uint32_t>(*asic_->getSwitchId()), 0, 0, 10000, 8}};
}
SaiMeru800biaPlatform::~SaiMeru800biaPlatform() = default;

} // namespace facebook::fboss
