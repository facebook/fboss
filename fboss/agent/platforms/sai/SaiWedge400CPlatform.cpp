/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/sai/SaiWedge400CPlatform.h"
#include "fboss/agent/hw/switch_asics/EbroAsic.h"
#include "fboss/agent/platforms/common/ebb_lab/Wedge400CEbbLabPlatformMapping.h"
#include "fboss/agent/platforms/common/wedge400c/Wedge400CPlatformMapping.h"
#include "fboss/agent/platforms/sai/SaiWedge400CPlatformPort.h"

#include <algorithm>

namespace facebook::fboss {

SaiWedge400CPlatform::SaiWedge400CPlatform(
    std::unique_ptr<PlatformProductInfo> productInfo,
    folly::MacAddress localMac)
    : SaiTajoPlatform(
          std::move(productInfo),
          std::make_unique<Wedge400CPlatformMapping>(),
          localMac) {
  asic_ = std::make_unique<EbroAsic>();
}

SaiWedge400CPlatform::SaiWedge400CPlatform(
    std::unique_ptr<PlatformProductInfo> productInfo,
    std::unique_ptr<Wedge400CEbbLabPlatformMapping> mapping,
    folly::MacAddress localMac)
    : SaiTajoPlatform(std::move(productInfo), std::move(mapping), localMac) {
  asic_ = std::make_unique<EbroAsic>();
}

HwAsic* SaiWedge400CPlatform::getAsic() const {
  return asic_.get();
}

std::string SaiWedge400CPlatform::getHwConfig() {
  return *config()->thrift.platform()->get_chip().get_asic().config();
}

SaiWedge400CPlatform::~SaiWedge400CPlatform() {}

SaiWedge400CEbbLabPlatform::SaiWedge400CEbbLabPlatform(
    std::unique_ptr<PlatformProductInfo> productInfo,
    folly::MacAddress localMac)
    : SaiWedge400CPlatform(
          std::move(productInfo),
          std::make_unique<Wedge400CEbbLabPlatformMapping>(),
          localMac) {}

} // namespace facebook::fboss
