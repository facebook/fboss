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
#include "fboss/agent/platforms/common/minipack3n/Minipack3NPlatformMapping.h"
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
#include "fboss/lib/bsp/minipack3n/Minipack3NBspPlatformMapping.h"
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
  } else if (
      mode == PlatformType::PLATFORM_DARWIN ||
      mode == PlatformType::PLATFORM_DARWIN48V) {
    return createDarwinWedgeManager(platformMappingStr);
  } else if (mode == PlatformType::PLATFORM_ELBERT) {
    return createElbertWedgeManager(platformMappingStr);
  } else if (mode == PlatformType::PLATFORM_MERU400BFU) {
    return createBspWedgeManager<
        Meru400bfuBspPlatformMapping,
        Meru400bfuPlatformMapping,
        PlatformType::PLATFORM_MERU400BFU>(platformMappingStr);
  } else if (mode == PlatformType::PLATFORM_MERU400BIA) {
    return createBspWedgeManager<
        Meru400biaBspPlatformMapping,
        Meru400biaPlatformMapping,
        PlatformType::PLATFORM_MERU400BIA>(platformMappingStr);
  } else if (mode == PlatformType::PLATFORM_MERU400BIU) {
    return createBspWedgeManager<
        Meru400biuBspPlatformMapping,
        Meru400biuPlatformMapping,
        PlatformType::PLATFORM_MERU400BIU>(platformMappingStr);
  } else if (
      mode == PlatformType::PLATFORM_MERU800BIA ||
      mode == PlatformType::PLATFORM_MERU800BIAB) {
    return createBspWedgeManager<
        Meru800biaBspPlatformMapping,
        Meru800biaPlatformMapping,
        PlatformType::PLATFORM_MERU800BIA>(platformMappingStr);
  } else if (
      mode == PlatformType::PLATFORM_MERU800BFA ||
      mode == PlatformType::PLATFORM_MERU800BFA_P1) {
    return createBspWedgeManager<
        Meru800bfaBspPlatformMapping,
        Meru800bfaPlatformMapping,
        PlatformType::PLATFORM_MERU800BFA>(platformMappingStr);
  } else if (mode == PlatformType::PLATFORM_MONTBLANC) {
    return createBspWedgeManager<
        MontblancBspPlatformMapping,
        MontblancPlatformMapping,
        PlatformType::PLATFORM_MONTBLANC>(platformMappingStr);
  } else if (mode == PlatformType::PLATFORM_MINIPACK3N) {
    return createBspWedgeManager<
        Minipack3NBspPlatformMapping,
        Minipack3NPlatformMapping,
        PlatformType::PLATFORM_MINIPACK3N>(platformMappingStr);
  } else if (mode == PlatformType::PLATFORM_MORGAN800CC) {
    return createBspWedgeManager<
        Morgan800ccBspPlatformMapping,
        Morgan800ccPlatformMapping,
        PlatformType::PLATFORM_MORGAN800CC>(platformMappingStr);
  } else if (mode == PlatformType::PLATFORM_WEDGE400C) {
    return std::make_unique<Wedge400CManager>(platformMappingStr);
  } else if (mode == PlatformType::PLATFORM_JANGA800BIC) {
    return createBspWedgeManager<
        Janga800bicBspPlatformMapping,
        Janga800bicPlatformMapping,
        PlatformType::PLATFORM_JANGA800BIC>(platformMappingStr);
  } else if (mode == PlatformType::PLATFORM_TAHAN800BC) {
    return createBspWedgeManager<
        Tahan800bcBspPlatformMapping,
        Tahan800bcPlatformMapping,
        PlatformType::PLATFORM_TAHAN800BC>(platformMappingStr);
  } else if (
      mode == PlatformType::PLATFORM_FUJI ||
      mode == PlatformType::PLATFORM_MINIPACK ||
      mode == PlatformType::PLATFORM_WEDGE400) {
    return createFBWedgeManager(std::move(productInfo), platformMappingStr);
  }
  return std::make_unique<Wedge40Manager>(platformMappingStr);
}

template <
    typename BspPlatformMapping,
    typename PlatformMapping,
    PlatformType platformType>
std::unique_ptr<WedgeManager> createBspWedgeManager(
    const std::string& platformMappingStr) {
  auto systemContainer =
      BspGenericSystemContainer<BspPlatformMapping>::getInstance().get();
  return std::make_unique<BspWedgeManager>(
      systemContainer,
      std::make_unique<BspTransceiverApi>(systemContainer),
      platformMappingStr.empty()
          ? std::make_unique<PlatformMapping>()
          : std::make_unique<PlatformMapping>(platformMappingStr),
      platformType);
}
} // namespace fboss
} // namespace facebook
