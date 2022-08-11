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
    : SaiHwPlatform(
          std::move(productInfo),
          std::make_unique<MakaluPlatformMapping>(),
          localMac) {
  asic_ = std::make_unique<IndusAsic>();
}

HwAsic* SaiMakaluPlatform::getAsic() const {
  return asic_.get();
}

std::string SaiMakaluPlatform::getHwConfig() {
  return *config()->thrift.platform()->get_chip().get_asic().config();
}

SaiMakaluPlatform::~SaiMakaluPlatform() {}

} // namespace facebook::fboss
