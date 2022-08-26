/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/sai/SaiMakaluPlatform.h"

#include "fboss/agent/hw/switch_asics/IndusAsic.h"
#include "fboss/agent/platforms/common/makalu/MakaluPlatformMapping.h"

#include <cstdio>
#include <cstring>
namespace facebook::fboss {

SaiMakaluPlatform::SaiMakaluPlatform(
    std::unique_ptr<PlatformProductInfo> productInfo,
    folly::MacAddress localMac)
    : SaiBcmPlatform(
          std::move(productInfo),
          std::make_unique<MakaluPlatformMapping>(),
          localMac) {}

void SaiMakaluPlatform::setupAsic(
    cfg::SwitchType /*switchType*/,
    std::optional<int64_t> /*switchId*/) {
  asic_ = std::make_unique<IndusAsic>();
}

HwAsic* SaiMakaluPlatform::getAsic() const {
  return asic_.get();
}

SaiMakaluPlatform::~SaiMakaluPlatform() {}

} // namespace facebook::fboss
