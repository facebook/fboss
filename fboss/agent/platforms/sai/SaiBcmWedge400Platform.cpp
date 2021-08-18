/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/sai/SaiBcmWedge400Platform.h"

#include "fboss/agent/hw/switch_asics/Tomahawk3Asic.h"
#include "fboss/agent/platforms/common/wedge400/Wedge400PlatformMapping.h"

#include <cstdio>
#include <cstring>
namespace facebook::fboss {

SaiBcmWedge400Platform::SaiBcmWedge400Platform(
    std::unique_ptr<PlatformProductInfo> productInfo,
    folly::MacAddress localMac)
    : SaiBcmPlatform(
          std::move(productInfo),
          std::make_unique<Wedge400PlatformMapping>(),
          localMac) {
  asic_ = std::make_unique<Tomahawk3Asic>();
}

HwAsic* SaiBcmWedge400Platform::getAsic() const {
  return asic_.get();
}

void SaiBcmWedge400Platform::initLEDs() {
  // TODO skhare
}

SaiBcmWedge400Platform::~SaiBcmWedge400Platform() {}

} // namespace facebook::fboss
