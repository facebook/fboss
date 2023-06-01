/*
 *  Copyright (c) 2023-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/sai/SaiMorgan800ccPlatform.h"
#include "fboss/agent/hw/switch_asics/YubaAsic.h"
#include "fboss/agent/platforms/common/morgan/Morgan800ccPlatformMapping.h"

#include <algorithm>

namespace facebook::fboss {

SaiMorgan800ccPlatform::SaiMorgan800ccPlatform(
    std::unique_ptr<PlatformProductInfo> productInfo,
    folly::MacAddress localMac,
    const std::string& platformMappingStr)
    : SaiTajoPlatform(
          std::move(productInfo),
          platformMappingStr.empty()
              ? std::make_unique<Morgan800ccPlatformMapping>()
              : std::make_unique<Morgan800ccPlatformMapping>(platformMappingStr),
          localMac) {}

void SaiMorgan800ccPlatform::setupAsic(
    cfg::SwitchType switchType,
    std::optional<int64_t> switchId,
    std::optional<cfg::Range64> systemPortRange,
    folly::MacAddress& mac) {
  asic_ = std::make_unique<YubaAsic>(switchType, switchId, systemPortRange, mac);
  asic_->setDefaultStreamType(cfg::StreamType::UNICAST);
}

HwAsic* SaiMorgan800ccPlatform::getAsic() const {
  return asic_.get();
}

std::string SaiMorgan800ccPlatform::getHwConfig() {
  return *config()->thrift.platform()->get_chip().get_asic().config();
}

SaiMorgan800ccPlatform::~SaiMorgan800ccPlatform() {}

} // namespace facebook::fboss