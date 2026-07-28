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

} // namespace facebook::fboss
