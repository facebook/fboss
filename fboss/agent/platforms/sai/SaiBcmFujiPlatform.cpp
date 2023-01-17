/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/sai/SaiBcmFujiPlatform.h"
#include "fboss/agent/platforms/common/fuji/FujiPlatformMapping.h"

#include <cstdio>
#include <cstring>
namespace facebook::fboss {

SaiBcmFujiPlatform::SaiBcmFujiPlatform(
    std::unique_ptr<PlatformProductInfo> productInfo,
    folly::MacAddress localMac)
    : SaiBcmPlatform(
          std::move(productInfo),
          std::make_unique<FujiPlatformMapping>(),
          localMac) {}

void SaiBcmFujiPlatform::setupAsic(
    cfg::SwitchType switchType,
    std::optional<int64_t> switchId,
    std::optional<cfg::Range64> systemPortRange) {
  asic_ =
      std::make_unique<Tomahawk4Asic>(switchType, switchId, systemPortRange);
}

HwAsic* SaiBcmFujiPlatform::getAsic() const {
  return asic_.get();
}

SaiBcmFujiPlatform::~SaiBcmFujiPlatform() {}

} // namespace facebook::fboss
