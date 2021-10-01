/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/sai/SaiTajoPlatform.h"
#include "fboss/agent/hw/switch_asics/TajoAsic.h"

namespace facebook::fboss {
SaiTajoPlatform::SaiTajoPlatform(
    std::unique_ptr<PlatformProductInfo> productInfo,
    std::unique_ptr<PlatformMapping> platformMapping,
    folly::MacAddress localMac)
    : SaiHwPlatform(
          std::move(productInfo),
          std::move(platformMapping),
          localMac) {
  asic_ = std::make_unique<TajoAsic>();
}

HwAsic* SaiTajoPlatform::getAsic() const {
  return asic_.get();
}

std::optional<SaiSwitchTraits::Attributes::AclFieldList>
SaiTajoPlatform::getAclFieldList() const {
  return std::nullopt;
}

SaiTajoPlatform::~SaiTajoPlatform() {}

} // namespace facebook::fboss
