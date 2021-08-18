/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/sai/SaiBcmGalaxyFCPlatform.h"
#include "fboss/agent/hw/switch_asics/TomahawkAsic.h"
#include "fboss/agent/platforms/common/galaxy/GalaxyFCPlatformMapping.h"

namespace facebook::fboss {

SaiBcmGalaxyFCPlatform::SaiBcmGalaxyFCPlatform(
    std::unique_ptr<PlatformProductInfo> productInfo,
    folly::MacAddress localMac)
    : SaiBcmGalaxyPlatform(
          std::move(productInfo),
          std::make_unique<GalaxyFCPlatformMapping>(
              GalaxyFCPlatformMapping::getFabriccardName()),
          localMac) {
  asic_ = std::make_unique<TomahawkAsic>();
}

HwAsic* SaiBcmGalaxyFCPlatform::getAsic() const {
  return asic_.get();
}

SaiBcmGalaxyFCPlatform::~SaiBcmGalaxyFCPlatform() {}
} // namespace facebook::fboss
