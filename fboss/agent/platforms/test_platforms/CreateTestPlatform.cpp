/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/test_platforms/CreateTestPlatform.h"

#include "fboss/agent/FbossError.h"
#include "fboss/agent/platforms/test_platforms/BcmTestGalaxyPlatform.h"
#include "fboss/agent/platforms/test_platforms/BcmTestMinipackPlatform.h"
#include "fboss/agent/platforms/test_platforms/BcmTestWedge100Platform.h"
#include "fboss/agent/platforms/test_platforms/BcmTestWedge40Platform.h"
#include "fboss/agent/platforms/test_platforms/BcmTestYampPlatform.h"
#include "fboss/agent/platforms/test_platforms/FakeBcmTestPlatform.h"

namespace facebook {
namespace fboss {

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
  if (mode == PlatformMode::WEDGE) {
    return std::make_unique<BcmTestWedge40Platform>();
  } else if (mode == PlatformMode::WEDGE100) {
    return std::make_unique<BcmTestWedge100Platform>();
  } else if (
      mode == PlatformMode::GALAXY_LC || mode == PlatformMode::GALAXY_FC) {
    return std::make_unique<BcmTestGalaxyPlatform>();
  } else if (mode == PlatformMode::MINIPACK) {
    return std::make_unique<BcmTestMinipackPlatform>();
  } else if (mode == PlatformMode::YAMP) {
    return std::make_unique<BcmTestYampPlatform>();
  } else if (mode == PlatformMode::FAKE_WEDGE) {
    return std::make_unique<FakeBcmTestPlatform>();
  } else {
    throw std::runtime_error("invalid mode ");
  }
  return nullptr;
}

} // namespace fboss
} // namespace facebook
