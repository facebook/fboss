/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/AgentConfig.h"
#include "fboss/agent/Main.h"
#include "fboss/agent/Platform.h"
#include "fboss/agent/platforms/wedge/WedgeProductInfo.h"
#include "fboss/agent/platforms/sai/SaiPlatform.h"
#include "fboss/agent/platforms/sai/SaiBcmPlatform.h"


#include <memory>

using namespace facebook::fboss;

std::unique_ptr<Platform> initSaiPlatform(
    std::unique_ptr<AgentConfig> config) {
  std::unique_ptr<SaiPlatform> platform;
  try {
    auto productInfo = std::make_unique<WedgeProductInfo>(FLAGS_fruid_filepath);
    productInfo->initialize();
    auto mode = productInfo->getMode();
    if (mode == WedgePlatformMode::WEDGE100) {
      platform = std::make_unique<SaiBcmPlatform>();
    }
  } catch (const std::exception& e) {
    XLOG(INFO) << " Unable to parse platform info, falling back to default "
               << " sai platform: " << e.what();
    platform = std::make_unique<SaiPlatform>();
  }

  platform->init(std::move(config));
  return std::move(platform);
}

int main(int argc, char* argv[]) {
  return facebook::fboss::fbossMain(argc, argv, initSaiPlatform);
}
