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
          localMac) {
  asic_ = std::make_unique<Tomahawk3Asic>();
}

HwAsic* SaiBcmDarwinPlatform::getAsic() const {
  return asic_.get();
}

void SaiBcmDarwinPlatform::initLEDs() {
  // TODO(sampham): initialize LED through FPGA
}

SaiBcmDarwinPlatform::~SaiBcmDarwinPlatform() {}

} // namespace facebook::fboss
