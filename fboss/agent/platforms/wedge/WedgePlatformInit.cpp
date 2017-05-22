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

#include "fboss/agent/Platform.h"
#include "fboss/agent/platforms/wedge/GalaxyLCPlatform.h"
#include "fboss/agent/platforms/wedge/GalaxyFCPlatform.h"
#include "fboss/agent/platforms/wedge/WedgePlatform.h"
#include "fboss/agent/platforms/wedge/Wedge40Platform.h"
#include "fboss/agent/platforms/wedge/Wedge100Platform.h"

DEFINE_string(fruid_filepath,
              "/var/facebook/fboss/fruid.json",
              "File for storing the fruid data");

namespace facebook { namespace fboss {

std::unique_ptr<WedgePlatform> chooseWedgePlatform() {
  auto productInfo = std::make_unique<WedgeProductInfo>(FLAGS_fruid_filepath);
  productInfo->initialize();

  auto mode = productInfo->getMode();
  if (mode == WedgePlatformMode::WEDGE100) {
    return std::make_unique<Wedge100Platform>(std::move(productInfo));
  } else if (mode == WedgePlatformMode::GALAXY_LC) {
    return std::make_unique<GalaxyLCPlatform>(std::move(productInfo));
  } else if (mode == WedgePlatformMode::GALAXY_FC) {
    return std::make_unique<GalaxyFCPlatform>(std::move(productInfo));
  }

  return std::make_unique<Wedge40Platform>(std::move(productInfo));
}

std::unique_ptr<Platform> initWedgePlatform() {
  auto platform = chooseWedgePlatform();
  platform->init();
  return std::move(platform);
}

}}
