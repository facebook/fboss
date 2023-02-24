/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/sai/SaiYangraPlatform.h"

#include "fboss/agent/hw/switch_asics/IndusAsic.h"
#include "fboss/agent/platforms/common/yangra/YangraPlatformMapping.h"

#include <cstdio>
#include <cstring>
namespace facebook::fboss {

SaiYangraPlatform::SaiYangraPlatform(
    std::unique_ptr<PlatformProductInfo> productInfo,
    folly::MacAddress localMac,
    const std::string& platformMappingStr)
    : SaiBcmPlatform(
          std::move(productInfo),
          platformMappingStr.empty()
              ? std::make_unique<YangraPlatformMapping>()
              : std::make_unique<YangraPlatformMapping>(platformMappingStr),
          localMac) {}

void SaiYangraPlatform::setupAsic(
    cfg::SwitchType switchType,
    std::optional<int64_t> switchId,
    std::optional<cfg::Range64> systemPortRange) {
  asic_ = std::make_unique<IndusAsic>(switchType, switchId, systemPortRange);
}

HwAsic* SaiYangraPlatform::getAsic() const {
  return asic_.get();
}

std::vector<sai_system_port_config_t>
SaiYangraPlatform::getInternalSystemPortConfig() const {
  CHECK(asic_) << " Asic must be set before getting sys port info";
  CHECK(asic_->getSwitchId()) << " Switch Id must be set before sys port info";
  return {{0, static_cast<uint32_t>(*asic_->getSwitchId()), 0, 0, 10000, 8}};
}
SaiYangraPlatform::~SaiYangraPlatform() {}

} // namespace facebook::fboss
