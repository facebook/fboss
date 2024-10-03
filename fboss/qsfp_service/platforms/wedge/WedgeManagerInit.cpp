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

#include "fboss/agent/platforms/common/janga800bic/Janga800bicPlatformMapping.h"
#include "fboss/agent/platforms/common/meru400bfu/Meru400bfuPlatformMapping.h"
#include "fboss/agent/platforms/common/meru400bia/Meru400biaPlatformMapping.h"
#include "fboss/agent/platforms/common/meru400biu/Meru400biuPlatformMapping.h"
#include "fboss/agent/platforms/common/meru800bfa/Meru800bfaPlatformMapping.h"
#include "fboss/agent/platforms/common/meru800bia/Meru800biaPlatformMapping.h"
#include "fboss/agent/platforms/common/montblanc/MontblancPlatformMapping.h"
#include "fboss/agent/platforms/common/morgan800cc/Morgan800ccPlatformMapping.h"
#include "fboss/agent/platforms/common/tahan800bc/Tahan800bcPlatformMapping.h"
#include "fboss/lib/bsp/BspGenericSystemContainer.h"
#include "fboss/lib/bsp/janga800bic/Janga800bicBspPlatformMapping.h"
#include "fboss/lib/bsp/meru400bfu/Meru400bfuBspPlatformMapping.h"
#include "fboss/lib/bsp/meru400bia/Meru400biaBspPlatformMapping.h"
#include "fboss/lib/bsp/meru400biu/Meru400biuBspPlatformMapping.h"
#include "fboss/lib/bsp/meru800bfa/Meru800bfaBspPlatformMapping.h"
#include "fboss/lib/bsp/meru800bia/Meru800biaBspPlatformMapping.h"
#include "fboss/lib/bsp/montblanc/MontblancBspPlatformMapping.h"
#include "fboss/lib/bsp/morgan800cc/Morgan800ccBspPlatformMapping.h"
#include "fboss/lib/bsp/tahan800bc/Tahan800bcBspPlatformMapping.h"
#include "fboss/lib/platforms/PlatformProductInfo.h"
#include "fboss/qsfp_service/platforms/wedge/BspWedgeManager.h"
#include "fboss/qsfp_service/platforms/wedge/GalaxyManager.h"
#include "fboss/qsfp_service/platforms/wedge/Wedge100Manager.h"
#include "fboss/qsfp_service/platforms/wedge/Wedge400CManager.h"
#include "fboss/qsfp_service/platforms/wedge/Wedge40Manager.h"

#include "fboss/lib/CommonFileUtils.h"

namespace facebook {
namespace fboss {

std::unique_ptr<WedgeManager> createWedgeManager() {
  auto productInfo =
      std::make_unique<PlatformProductInfo>(FLAGS_fruid_filepath);
  productInfo->initialize();
  auto mode = productInfo->getType();

  std::string platformMappingStr;
  if (!FLAGS_platform_mapping_override_path.empty()) {
    if (!folly::readFile(
            FLAGS_platform_mapping_override_path.data(), platformMappingStr)) {
      throw FbossError("unable to read ", FLAGS_platform_mapping_override_path);
    }
    XLOG(INFO) << "Overriding platform mapping from "
               << FLAGS_platform_mapping_override_path;
  }

  createDir(FLAGS_qsfp_service_volatile_dir);
  if (mode == PlatformType::PLATFORM_WEDGE100) {
    return std::make_unique<Wedge100Manager>(platformMappingStr);
  } else if (
      mode == PlatformType::PLATFORM_GALAXY_LC ||
      mode == PlatformType::PLATFORM_GALAXY_FC) {
    return std::make_unique<GalaxyManager>(mode, platformMappingStr);
  } else if (mode == PlatformType::PLATFORM_YAMP) {
    return createYampWedgeManager(platformMappingStr);
  } else if (mode == PlatformType::PLATFORM_DARWIN) {
    return createDarwinWedgeManager(platformMappingStr);
  } else if (mode == PlatformType::PLATFORM_ELBERT) {
    return createElbertWedgeManager(platformMappingStr);
  } else if (mode == PlatformType::PLATFORM_MERU400BFU) {
    return createMeru400bfuWedgeManager(platformMappingStr);
  } else if (mode == PlatformType::PLATFORM_MERU400BIA) {
    return createMeru400biaWedgeManager(platformMappingStr);
  } else if (mode == PlatformType::PLATFORM_MERU400BIU) {
    return createMeru400biuWedgeManager(platformMappingStr);
  } else if (mode == PlatformType::PLATFORM_MERU800BIA) {
    return createMeru800biaWedgeManager(platformMappingStr);
  } else if (
      mode == PlatformType::PLATFORM_MERU800BFA ||
      mode == PlatformType::PLATFORM_MERU800BFA_P1) {
    return createMeru800bfaWedgeManager(platformMappingStr);
  } else if (mode == PlatformType::PLATFORM_MONTBLANC) {
    return createMontblancWedgeManager(platformMappingStr);
  } else if (mode == PlatformType::PLATFORM_MORGAN800CC) {
    return createMorgan800ccWedgeManager(platformMappingStr);
  } else if (mode == PlatformType::PLATFORM_WEDGE400C) {
    return std::make_unique<Wedge400CManager>(platformMappingStr);
  } else if (mode == PlatformType::PLATFORM_JANGA800BIC) {
    return createJanga800bicWedgeManager(platformMappingStr);
  } else if (mode == PlatformType::PLATFORM_TAHAN800BC) {
    return createTahan800bcWedgeManager(platformMappingStr);
  } else if (
      mode == PlatformType::PLATFORM_FUJI ||
      mode == PlatformType::PLATFORM_MINIPACK ||
      mode == PlatformType::PLATFORM_WEDGE400 ||
      mode == PlatformType::PLATFORM_CLOUDRIPPER) {
    return createFBWedgeManager(std::move(productInfo), platformMappingStr);
  }
  return std::make_unique<Wedge40Manager>(platformMappingStr);
}

std::unique_ptr<WedgeManager> createMeru400bfuWedgeManager(
    const std::string& platformMappingStr) {
  auto systemContainer =
      BspGenericSystemContainer<Meru400bfuBspPlatformMapping>::getInstance()
          .get();
  return std::make_unique<BspWedgeManager>(
      systemContainer,
      std::make_unique<BspTransceiverApi>(systemContainer),
      platformMappingStr.empty()
          ? std::make_unique<Meru400bfuPlatformMapping>()
          : std::make_unique<Meru400bfuPlatformMapping>(platformMappingStr),
      PlatformType::PLATFORM_MERU400BFU);
}

std::unique_ptr<WedgeManager> createMeru400biaWedgeManager(
    const std::string& platformMappingStr) {
  auto systemContainer =
      BspGenericSystemContainer<Meru400biaBspPlatformMapping>::getInstance()
          .get();
  return std::make_unique<BspWedgeManager>(
      systemContainer,
      std::make_unique<BspTransceiverApi>(systemContainer),
      platformMappingStr.empty()
          ? std::make_unique<Meru400biaPlatformMapping>()
          : std::make_unique<Meru400biaPlatformMapping>(platformMappingStr),
      PlatformType::PLATFORM_MERU400BIA);
}

std::unique_ptr<WedgeManager> createMeru400biuWedgeManager(
    const std::string& platformMappingStr) {
  auto systemContainer =
      BspGenericSystemContainer<Meru400biuBspPlatformMapping>::getInstance()
          .get();
  return std::make_unique<BspWedgeManager>(
      systemContainer,
      std::make_unique<BspTransceiverApi>(systemContainer),
      platformMappingStr.empty()
          ? std::make_unique<Meru400biuPlatformMapping>()
          : std::make_unique<Meru400biuPlatformMapping>(platformMappingStr),
      PlatformType::PLATFORM_MERU400BIU);
}

std::unique_ptr<WedgeManager> createMeru800biaWedgeManager(
    const std::string& platformMappingStr) {
  auto systemContainer =
      BspGenericSystemContainer<Meru800biaBspPlatformMapping>::getInstance()
          .get();
  return std::make_unique<BspWedgeManager>(
      systemContainer,
      std::make_unique<BspTransceiverApi>(systemContainer),
      platformMappingStr.empty()
          ? std::make_unique<Meru800biaPlatformMapping>()
          : std::make_unique<Meru800biaPlatformMapping>(platformMappingStr),
      PlatformType::PLATFORM_MERU800BIA);
}

std::unique_ptr<WedgeManager> createMeru800bfaWedgeManager(
    const std::string& platformMappingStr) {
  auto systemContainer =
      BspGenericSystemContainer<Meru800bfaBspPlatformMapping>::getInstance()
          .get();
  return std::make_unique<BspWedgeManager>(
      systemContainer,
      std::make_unique<BspTransceiverApi>(systemContainer),
      platformMappingStr.empty()
          ? std::make_unique<Meru800bfaPlatformMapping>()
          : std::make_unique<Meru800bfaPlatformMapping>(platformMappingStr),
      PlatformType::PLATFORM_MERU800BFA);
}

std::unique_ptr<WedgeManager> createMontblancWedgeManager(
    const std::string& platformMappingStr) {
  auto systemContainer =
      BspGenericSystemContainer<MontblancBspPlatformMapping>::getInstance()
          .get();
  return std::make_unique<BspWedgeManager>(
      systemContainer,
      std::make_unique<BspTransceiverApi>(systemContainer),
      platformMappingStr.empty()
          ? std::make_unique<MontblancPlatformMapping>()
          : std::make_unique<MontblancPlatformMapping>(platformMappingStr),
      PlatformType::PLATFORM_MONTBLANC);
}

std::unique_ptr<WedgeManager> createMorgan800ccWedgeManager(
    const std::string& platformMappingStr) {
  auto systemContainer =
      BspGenericSystemContainer<Morgan800ccBspPlatformMapping>::getInstance()
          .get();
  return std::make_unique<BspWedgeManager>(
      systemContainer,
      std::make_unique<BspTransceiverApi>(systemContainer),
      platformMappingStr.empty()
          ? std::make_unique<Morgan800ccPlatformMapping>()
          : std::make_unique<Morgan800ccPlatformMapping>(platformMappingStr),
      PlatformType::PLATFORM_MORGAN800CC);
}
std::unique_ptr<WedgeManager> createJanga800bicWedgeManager(
    const std::string& platformMappingStr) {
  auto systemContainer =
      BspGenericSystemContainer<Janga800bicBspPlatformMapping>::getInstance()
          .get();
  return std::make_unique<BspWedgeManager>(
      systemContainer,
      std::make_unique<BspTransceiverApi>(systemContainer),
      platformMappingStr.empty()
          ? std::make_unique<Janga800bicPlatformMapping>()
          : std::make_unique<Janga800bicPlatformMapping>(platformMappingStr),
      PlatformType::PLATFORM_JANGA800BIC);
}
std::unique_ptr<WedgeManager> createTahan800bcWedgeManager(
    const std::string& platformMappingStr) {
  auto systemContainer =
      BspGenericSystemContainer<Tahan800bcBspPlatformMapping>::getInstance()
          .get();
  return std::make_unique<BspWedgeManager>(
      systemContainer,
      std::make_unique<BspTransceiverApi>(systemContainer),
      platformMappingStr.empty()
          ? std::make_unique<Tahan800bcPlatformMapping>()
          : std::make_unique<Tahan800bcPlatformMapping>(platformMappingStr),
      PlatformType::PLATFORM_TAHAN800BC);
}
} // namespace fboss
} // namespace facebook
