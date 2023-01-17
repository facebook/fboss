/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/sai/SaiKametPlatform.h"

#include "fboss/agent/hw/switch_asics/BeasAsic.h"
#include "fboss/agent/platforms/common/kamet/KametPlatformMapping.h"

#include <cstdio>
#include <cstring>
namespace facebook::fboss {

SaiKametPlatform::SaiKametPlatform(
    std::unique_ptr<PlatformProductInfo> productInfo,
    folly::MacAddress localMac)
    : SaiBcmPlatform(
          std::move(productInfo),
          std::make_unique<KametPlatformMapping>(),
          localMac) {}

void SaiKametPlatform::setupAsic(
    cfg::SwitchType switchType,
    std::optional<int64_t> switchId,
    std::optional<cfg::Range64> /*systemPortRange*/) {
  asic_ = std::make_unique<BeasAsic>(switchType, switchId);
}

HwAsic* SaiKametPlatform::getAsic() const {
  return asic_.get();
}

SaiKametPlatform::~SaiKametPlatform() {}

} // namespace facebook::fboss
