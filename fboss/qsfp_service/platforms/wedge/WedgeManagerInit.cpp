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

#include "fboss/agent/platforms/common/meru400bfu/Meru400bfuPlatformMapping.h"
#include "fboss/agent/platforms/common/meru400biu/Meru400biuPlatformMapping.h"
#include "fboss/agent/platforms/common/montblanc/MontblancPlatformMapping.h"
#include "fboss/lib/bsp/BspGenericSystemContainer.h"
#include "fboss/lib/bsp/meru400bfu/Meru400bfuBspPlatformMapping.h"
#include "fboss/lib/bsp/meru400biu/Meru400biuBspPlatformMapping.h"
#include "fboss/lib/bsp/montblanc/MontblancBspPlatformMapping.h"
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
  auto mode = productInfo->getType();

  createDir(FLAGS_qsfp_service_volatile_dir);
  if (mode == PlatformType::PLATFORM_WEDGE100) {
    return std::make_unique<Wedge100Manager>();
  } else if (
      mode == PlatformType::PLATFORM_GALAXY_LC ||
      mode == PlatformType::PLATFORM_GALAXY_FC) {
    return std::make_unique<GalaxyManager>(mode);
  } else if (mode == PlatformType::PLATFORM_YAMP) {
    return createYampWedgeManager();
  } else if (mode == PlatformType::PLATFORM_DARWIN) {
    return createDarwinWedgeManager();
  } else if (mode == PlatformType::PLATFORM_LASSEN) {
    return createLassenWedgeManager();
  } else if (mode == PlatformType::PLATFORM_ELBERT) {
    return createElbertWedgeManager();
  } else if (mode == PlatformType::PLATFORM_SANDIA) {
    return createSandiaWedgeManager();
  } else if (mode == PlatformType::PLATFORM_MERU400BFU) {
    return createMeru400bfuWedgeManager();
  } else if (mode == PlatformType::PLATFORM_MERU400BIU) {
    return createMeru400biuWedgeManager();
  } else if (mode == PlatformType::PLATFORM_MONTBLANC) {
    return createMontblancWedgeManager();
  } else if (
      mode == PlatformType::PLATFORM_FUJI ||
      mode == PlatformType::PLATFORM_MINIPACK ||
      mode == PlatformType::PLATFORM_WEDGE400 ||
      mode == PlatformType::PLATFORM_WEDGE400C ||
      mode == PlatformType::PLATFORM_CLOUDRIPPER) {
    return createFBWedgeManager(std::move(productInfo));
  }
  return std::make_unique<Wedge40Manager>();
}

std::unique_ptr<WedgeManager> createMeru400bfuWedgeManager() {
  auto systemContainer =
      BspGenericSystemContainer<Meru400bfuBspPlatformMapping>::getInstance()
          .get();
  return std::make_unique<BspWedgeManager>(
      systemContainer,
      std::make_unique<BspTransceiverApi>(systemContainer),
      std::make_unique<Meru400bfuPlatformMapping>(),
      PlatformType::PLATFORM_MERU400BFU);
}

std::unique_ptr<WedgeManager> createMeru400biuWedgeManager() {
  auto systemContainer =
      BspGenericSystemContainer<Meru400biuBspPlatformMapping>::getInstance()
          .get();
  return std::make_unique<BspWedgeManager>(
      systemContainer,
      std::make_unique<BspTransceiverApi>(systemContainer),
      std::make_unique<Meru400biuPlatformMapping>(),
      PlatformType::PLATFORM_MERU400BIU);
}

std::unique_ptr<WedgeManager> createMontblancWedgeManager() {
  auto systemContainer =
      BspGenericSystemContainer<MontblancBspPlatformMapping>::getInstance()
          .get();
  return std::make_unique<BspWedgeManager>(
      systemContainer,
      std::make_unique<BspTransceiverApi>(systemContainer),
      std::make_unique<MontblancPlatformMapping>(),
      PlatformType::PLATFORM_MONTBLANC);
}
} // namespace fboss
} // namespace facebook
