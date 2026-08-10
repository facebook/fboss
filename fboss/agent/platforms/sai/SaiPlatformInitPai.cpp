/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/sai/SaiPlatformInitImpl.h"

#include "fboss/agent/platforms/sai/SaiBcmPlatformPort.h"
#include "fboss/agent/platforms/sai/SaiFakePlatform.h"
#include "fboss/agent/platforms/sai/SaiFakePlatformPort.h"

namespace facebook::fboss {

// Broadcom PAI (retimer / XPHY) build. The retimer SaiPhyPlatform is created
// directly by BspSaiPhyManager::createExternalPhy, so chooseSaiPlatformImpl is
// only exercised by fake test paths here.
std::unique_ptr<SaiPlatform> chooseSaiPlatformImpl(
    std::unique_ptr<PlatformProductInfo>& productInfo,
    folly::MacAddress /*localMac*/,
    const std::string& /*platformMappingStr*/) {
  if (productInfo->getType() == PlatformType::PLATFORM_FAKE_SAI) {
    return std::make_unique<SaiFakePlatform>(std::move(productInfo));
  }
  return nullptr;
}

std::unique_ptr<SaiPlatformPort> createSaiPlatformPort(
    const PortID& portId,
    SaiPlatform* platform) {
  const auto type = platform->getType();
  // Agera3 retimer boxes: XPHY ports use the generic SaiBcmPlatformPort, same
  // as the agent bcm build. (These are the only real platforms the PAI build
  // instantiates.)
  if (type == PlatformType::PLATFORM_LADAKH800BCLS ||
      type == PlatformType::PLATFORM_LEH800BCLS) {
    return std::make_unique<SaiBcmPlatformPort>(portId, platform);
  }
  if (type == PlatformType::PLATFORM_FAKE_SAI ||
      type == PlatformType::PLATFORM_FAKE_WEDGE) {
    return std::make_unique<SaiFakePlatformPort>(portId, platform);
  }
  return nullptr;
}

} // namespace facebook::fboss
