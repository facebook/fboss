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

#include "fboss/agent/platforms/sai/SaiFakePlatform.h"
#include "fboss/agent/platforms/sai/SaiFakePlatformPort.h"

namespace facebook::fboss {

std::unique_ptr<SaiPlatform> chooseFakeSaiPlatform(
    std::unique_ptr<PlatformProductInfo>& productInfo,
    folly::MacAddress /*localMac*/,
    const std::string& /*platformMappingStr*/) {
  if (productInfo->getType() == PlatformType::PLATFORM_FAKE_SAI) {
    return std::make_unique<SaiFakePlatform>(std::move(productInfo));
  }
  return nullptr;
}

std::unique_ptr<SaiPlatformPort> createFakeSaiPlatformPort(
    const PortID& portId,
    SaiPlatform* platform) {
  if (platform->getType() != PlatformType::PLATFORM_FAKE_SAI &&
      platform->getType() != PlatformType::PLATFORM_FAKE_WEDGE) {
    return nullptr;
  }
  return std::make_unique<SaiFakePlatformPort>(portId, platform);
}

} // namespace facebook::fboss
