/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/platforms/sai/SaiPlatformInit.h"
#include "fboss/agent/platforms/common/PlatformProductInfo.h"

#include <memory>

#include "fboss/agent/AgentConfig.h"
#include "fboss/agent/Platform.h"
#include "fboss/agent/platforms/sai/SaiBcmPlatform.h"
#include "fboss/agent/platforms/sai/facebook/SaiWedge400CPlatform.h"

namespace facebook {
namespace fboss {

std::unique_ptr<SaiPlatform> chooseSaiPlatform() {
  auto productInfo =
      std::make_unique<PlatformProductInfo>(FLAGS_fruid_filepath);
  productInfo->initialize();

  auto mode = productInfo->getMode();
  if (mode == PlatformMode::WEDGE400C) {
    return std::make_unique<SaiWedge400CPlatform>(std::move(productInfo));
  } else if (mode == PlatformMode::WEDGE100) {
    return std::make_unique<SaiBcmPlatform>(std::move(productInfo));
  } else {
    return nullptr;
  }
}

std::unique_ptr<Platform> initSaiPlatform(std::unique_ptr<AgentConfig> config) {
  auto platform = chooseSaiPlatform();
  platform->init(std::move(config));
  return std::move(platform);
}
} // namespace fboss
} // namespace facebook
