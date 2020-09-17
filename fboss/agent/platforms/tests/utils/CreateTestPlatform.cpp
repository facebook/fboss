/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/tests/utils/CreateTestPlatform.h"

#include "fboss/agent/FbossError.h"
#include "fboss/agent/platforms/tests/utils/BcmTestFujiPlatform.h"
#include "fboss/agent/platforms/tests/utils/BcmTestGalaxyPlatform.h"
#include "fboss/agent/platforms/tests/utils/BcmTestMinipackPlatform.h"
#include "fboss/agent/platforms/tests/utils/BcmTestWedge100Platform.h"
#include "fboss/agent/platforms/tests/utils/BcmTestWedge400Platform.h"
#include "fboss/agent/platforms/tests/utils/BcmTestWedge40Platform.h"
#include "fboss/agent/platforms/tests/utils/BcmTestYampPlatform.h"
#include "fboss/agent/platforms/tests/utils/FakeBcmTestPlatform.h"

namespace facebook::fboss {

PlatformMode getPlatformMode() {
  try {
    facebook::fboss::PlatformProductInfo productInfo(FLAGS_fruid_filepath);
    productInfo.initialize();
    return productInfo.getMode();
  } catch (const facebook::fboss::FbossError& ex) {
    return PlatformMode::FAKE_WEDGE;
  }
}

std::unique_ptr<Platform> createTestPlatform() {
  auto mode = getPlatformMode();
  std::unique_ptr<PlatformProductInfo> productInfo;
  if (mode != PlatformMode::FAKE_WEDGE) {
    productInfo = std::make_unique<PlatformProductInfo>(FLAGS_fruid_filepath);
    productInfo->initialize();
  }
  if (mode == PlatformMode::WEDGE) {
    return std::make_unique<BcmTestWedge40Platform>(std::move(productInfo));
  } else if (mode == PlatformMode::WEDGE100) {
    return std::make_unique<BcmTestWedge100Platform>(std::move(productInfo));
  } else if (mode == PlatformMode::GALAXY_LC) {
    return std::make_unique<BcmTestGalaxyLCPlatform>(std::move(productInfo));
  } else if (mode == PlatformMode::GALAXY_FC) {
    return std::make_unique<BcmTestGalaxyFCPlatform>(std::move(productInfo));
  } else if (mode == PlatformMode::MINIPACK) {
    return std::make_unique<BcmTestMinipackPlatform>(std::move(productInfo));
  } else if (mode == PlatformMode::YAMP) {
    return std::make_unique<BcmTestYampPlatform>(std::move(productInfo));
  } else if (mode == PlatformMode::WEDGE400) {
    return std::make_unique<BcmTestWedge400Platform>(std::move(productInfo));
  } else if (mode == PlatformMode::FUJI) {
    return std::make_unique<BcmTestFujiPlatform>(std::move(productInfo));
  } else if (mode == PlatformMode::FAKE_WEDGE) {
    return std::make_unique<FakeBcmTestPlatform>();
  } else {
    throw std::runtime_error("invalid mode ");
  }
  return nullptr;
}

} // namespace facebook::fboss
