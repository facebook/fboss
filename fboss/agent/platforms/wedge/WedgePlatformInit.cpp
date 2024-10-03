/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/platforms/wedge/WedgePlatformInit.h"

#include <memory>

#include "fboss/agent/AgentConfig.h"
#include "fboss/agent/Platform.h"
#include "fboss/agent/Utils.h"
#include "fboss/agent/platforms/wedge/WedgePlatform.h"
#include "fboss/agent/platforms/wedge/wedge100/Wedge100Platform.h"
#include "fboss/agent/platforms/wedge/wedge40/FakeWedge40Platform.h"
#include "fboss/agent/platforms/wedge/wedge40/Wedge40Platform.h"
#include "fboss/lib/platforms/PlatformProductInfo.h"

namespace facebook::fboss {

std::unique_ptr<WedgePlatform> createWedgePlatform() {
  auto productInfo =
      std::make_unique<PlatformProductInfo>(FLAGS_fruid_filepath);
  productInfo->initialize();
  auto localMac = getLocalMacAddress();

  auto type = productInfo->getType();
  if (type == PlatformType::PLATFORM_WEDGE) {
    return std::make_unique<Wedge40Platform>(std::move(productInfo), localMac);
  } else if (type == PlatformType::PLATFORM_WEDGE100) {
    return std::make_unique<Wedge100Platform>(std::move(productInfo), localMac);
  } else if (type == PlatformType::PLATFORM_FAKE_WEDGE40) {
    return std::make_unique<FakeWedge40Platform>(
        std::move(productInfo), localMac);
  }

  // platform type is neither of the offical platforms above, consider it as a
  // Facebook dev platform
  return createFBWedgePlatform(std::move(productInfo), localMac);
}

std::unique_ptr<Platform> initWedgePlatform(
    std::unique_ptr<AgentConfig> config,
    uint32_t hwFeaturesDesired,
    int16_t switchIndex) {
  auto platform = createWedgePlatform();
  platform->init(std::move(config), hwFeaturesDesired, switchIndex);
  return std::move(platform);
}

} // namespace facebook::fboss
