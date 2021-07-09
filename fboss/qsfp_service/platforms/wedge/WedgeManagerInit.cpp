/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/qsfp_service/platforms/wedge/WedgeManagerInit.h"

#include "fboss/agent/platforms/common/PlatformProductInfo.h"
#include "fboss/qsfp_service/platforms/wedge/GalaxyManager.h"
#include "fboss/qsfp_service/platforms/wedge/Wedge100Manager.h"
#include "fboss/qsfp_service/platforms/wedge/Wedge40Manager.h"

#include "fboss/lib/CommonFileUtils.h"

DECLARE_string(qsfp_service_volatile_dir);

namespace facebook {
namespace fboss {

std::unique_ptr<WedgeManager> createWedgeManager(bool forceColdBoot) {
  auto productInfo =
      std::make_unique<PlatformProductInfo>(FLAGS_fruid_filepath);
  productInfo->initialize();
  auto mode = productInfo->getMode();

  createDir(FLAGS_qsfp_service_volatile_dir);
  if (forceColdBoot) {
    // Force a cold boot
    createFile(WedgeManager::forceColdBootFileName());
  }
  if (mode == PlatformMode::WEDGE100) {
    return std::make_unique<Wedge100Manager>();
  } else if (
      mode == PlatformMode::GALAXY_LC || mode == PlatformMode::GALAXY_FC) {
    return std::make_unique<GalaxyManager>(mode);
  } else if (mode == PlatformMode::YAMP) {
    return createYampWedgeManager();
  } else if (mode == PlatformMode::ELBERT) {
    return createElbertWedgeManager();
  } else if (
      mode == PlatformMode::FUJI || mode == PlatformMode::MINIPACK ||
      mode == PlatformMode::WEDGE400 || mode == PlatformMode::WEDGE400C) {
    return createFBWedgeManager(std::move(productInfo));
  }
  return std::make_unique<Wedge40Manager>();
}
} // namespace fboss
} // namespace facebook
