/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/sai/SaiLassenPlatform.h"
#include "fboss/agent/hw/switch_asics/EbroAsic.h"
#include "fboss/agent/platforms/common/lassen/LassenPlatformMapping.h"

#include <algorithm>

namespace facebook::fboss {

SaiLassenPlatform::SaiLassenPlatform(
    std::unique_ptr<PlatformProductInfo> productInfo,
    folly::MacAddress localMac)
    : SaiTajoPlatform(
          std::move(productInfo),
          std::make_unique<LassenPlatformMapping>(),
          localMac) {}

void SaiLassenPlatform::setupAsic(
    cfg::SwitchType switchType,
    std::optional<int64_t> switchId,
    std::optional<cfg::Range64> systemPortRange) {
  asic_ = std::make_unique<EbroAsic>(switchType, switchId, systemPortRange);
#if defined(TAJO_SDK_VERSION_1_56_0)
  asic_->setDefaultStreamType(cfg::StreamType::UNICAST);
#endif
}

HwAsic* SaiLassenPlatform::getAsic() const {
  return asic_.get();
}

std::string SaiLassenPlatform::getHwConfig() {
  return *config()->thrift.platform()->get_chip().get_asic().config();
}

SaiLassenPlatform::~SaiLassenPlatform() {}

} // namespace facebook::fboss
