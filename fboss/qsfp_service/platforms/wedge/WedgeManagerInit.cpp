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

#include "fboss/agent/platforms/common/kamet/KametPlatformMapping.h"
#include "fboss/agent/platforms/common/makalu/MakaluPlatformMapping.h"
#include "fboss/lib/bsp/BspGenericSystemContainer.h"
#include "fboss/lib/bsp/kamet/KametBspPlatformMapping.h"
#include "fboss/lib/bsp/makalu/MakaluBspPlatformMapping.h"
#include "fboss/lib/platforms/PlatformProductInfo.h"
#include "fboss/qsfp_service/platforms/wedge/BspWedgeManager.h"
#include "fboss/qsfp_service/platforms/wedge/GalaxyManager.h"
#include "fboss/qsfp_service/platforms/wedge/Wedge100Manager.h"
#include "fboss/qsfp_service/platforms/wedge/Wedge40Manager.h"

#include "fboss/lib/CommonFileUtils.h"

namespace facebook {
namespace fboss {

std::unique_ptr<WedgeManager> createWedgeManager() {
  auto productInfo =
      std::make_unique<PlatformProductInfo>(FLAGS_fruid_filepath);
  productInfo->initialize();
  auto mode = productInfo->getMode();

  createDir(FLAGS_qsfp_service_volatile_dir);
  if (mode == PlatformMode::WEDGE100) {
    return std::make_unique<Wedge100Manager>();
  } else if (
      mode == PlatformMode::GALAXY_LC || mode == PlatformMode::GALAXY_FC) {
    return std::make_unique<GalaxyManager>(mode);
  } else if (mode == PlatformMode::YAMP) {
    return createYampWedgeManager();
  } else if (mode == PlatformMode::DARWIN) {
    return createDarwinWedgeManager();
  } else if (mode == PlatformMode::LASSEN) {
    return createLassenWedgeManager();
  } else if (mode == PlatformMode::ELBERT) {
    return createElbertWedgeManager();
  } else if (mode == PlatformMode::SANDIA) {
    return createSandiaWedgeManager();
  } else if (mode == PlatformMode::KAMET) {
    return createKametWedgeManager();
  } else if (mode == PlatformMode::MAKALU) {
    return createMakaluWedgeManager();
  } else if (
      mode == PlatformMode::FUJI || mode == PlatformMode::MINIPACK ||
      mode == PlatformMode::WEDGE400 || mode == PlatformMode::WEDGE400C ||
      mode == PlatformMode::CLOUDRIPPER) {
    return createFBWedgeManager(std::move(productInfo));
  }
  return std::make_unique<Wedge40Manager>();
}

std::unique_ptr<WedgeManager> createKametWedgeManager() {
  auto systemContainer =
      BspGenericSystemContainer<KametBspPlatformMapping>::getInstance().get();
  return std::make_unique<BspWedgeManager>(
      systemContainer,
      std::make_unique<BspTransceiverApi>(systemContainer),
      std::make_unique<KametPlatformMapping>(),
      PlatformMode::KAMET);
}

std::unique_ptr<WedgeManager> createMakaluWedgeManager() {
  auto systemContainer =
      BspGenericSystemContainer<MakaluBspPlatformMapping>::getInstance().get();
  return std::make_unique<BspWedgeManager>(
      systemContainer,
      std::make_unique<BspTransceiverApi>(systemContainer),
      std::make_unique<MakaluPlatformMapping>(),
      PlatformMode::MAKALU);
}
} // namespace fboss
} // namespace facebook
