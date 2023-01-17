/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/sai/SaiBcmDarwinPlatform.h"

#include "fboss/agent/hw/switch_asics/Tomahawk3Asic.h"
#include "fboss/agent/platforms/common/darwin/DarwinPlatformMapping.h"

#include <cstdio>
#include <cstring>
namespace facebook::fboss {

SaiBcmDarwinPlatform::SaiBcmDarwinPlatform(
    std::unique_ptr<PlatformProductInfo> productInfo,
    folly::MacAddress localMac)
    : SaiBcmPlatform(
          std::move(productInfo),
          std::make_unique<DarwinPlatformMapping>(),
          localMac) {}

void SaiBcmDarwinPlatform::setupAsic(
    cfg::SwitchType switchType,
    std::optional<int64_t> switchId,
    std::optional<cfg::Range64> /*systemPortRange*/) {
  asic_ = std::make_unique<Tomahawk3Asic>(switchType, switchId);
}

HwAsic* SaiBcmDarwinPlatform::getAsic() const {
  return asic_.get();
}

SaiBcmDarwinPlatform::~SaiBcmDarwinPlatform() {}

} // namespace facebook::fboss
