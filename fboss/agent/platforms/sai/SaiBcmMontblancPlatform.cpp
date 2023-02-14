/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/sai/SaiBcmMontblancPlatform.h"

#include "fboss/agent/hw/switch_asics/Tomahawk5Asic.h"
#include "fboss/agent/platforms/common/montblanc/MontblancPlatformMapping.h"

#include <cstdio>
#include <cstring>
namespace facebook::fboss {

SaiBcmMontblancPlatform::SaiBcmMontblancPlatform(
    std::unique_ptr<PlatformProductInfo> productInfo,
    folly::MacAddress localMac)
    : SaiBcmPlatform(
          std::move(productInfo),
          std::make_unique<MontblancPlatformMapping>(),
          localMac) {}

void SaiBcmMontblancPlatform::setupAsic(
    cfg::SwitchType switchType,
    std::optional<int64_t> switchId,
    std::optional<cfg::Range64> systemPortRange) {
  asic_ =
      std::make_unique<Tomahawk5Asic>(switchType, switchId, systemPortRange);
}

HwAsic* SaiBcmMontblancPlatform::getAsic() const {
  return asic_.get();
}

SaiBcmMontblancPlatform::~SaiBcmMontblancPlatform() {}

} // namespace facebook::fboss
