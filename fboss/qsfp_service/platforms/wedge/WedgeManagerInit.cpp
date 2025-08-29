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

#include "fboss/agent/platforms/common/PlatformMappingUtils.h"
#include "fboss/lib/bsp/BspGenericSystemContainer.h"
#include "fboss/lib/bsp/icecube800bc/Icecube800bcBspPlatformMapping.h"
#include "fboss/lib/bsp/icetea800bc/Icetea800bcBspPlatformMapping.h"
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

namespace facebook::fboss {

std::unique_ptr<WedgeManager> createWedgeManager() {
  auto productInfo =
      std::make_unique<PlatformProductInfo>(FLAGS_fruid_filepath);
  productInfo->initialize();
  auto mode = productInfo->getType();

  // Only used for platform mapping overrides.
  std::string platformMappingStr;
  if (!FLAGS_platform_mapping_override_path.empty()) {
    if (!folly::readFile(
            FLAGS_platform_mapping_override_path.data(), platformMappingStr)) {
      throw FbossError("unable to read ", FLAGS_platform_mapping_override_path);
    }
    XLOG(INFO) << "Overriding platform mapping from "
               << FLAGS_platform_mapping_override_path;
  }

  std::shared_ptr<const PlatformMapping> platformMapping =
      utility::initPlatformMapping(mode);

  const auto threads =
      std::make_shared<std::unordered_map<TransceiverID, SlotThreadHelper>>();
  for (const auto& tcvrID :
       utility::getTransceiverIds(platformMapping->getChips())) {
    threads->emplace(tcvrID, SlotThreadHelper(tcvrID));
  }

  createDir(FLAGS_qsfp_service_volatile_dir);
  switch (mode) {
    case PlatformType::PLATFORM_WEDGE100:
      return std::make_unique<Wedge100Manager>(platformMapping, threads);
    case PlatformType::PLATFORM_GALAXY_LC:
    case PlatformType::PLATFORM_GALAXY_FC:
      return std::make_unique<GalaxyManager>(mode, platformMapping, threads);
    case PlatformType::PLATFORM_YAMP:
      return createYampWedgeManager(platformMapping, threads);
    case PlatformType::PLATFORM_DARWIN:
    case PlatformType::PLATFORM_DARWIN48V:
      return createDarwinWedgeManager(platformMapping, threads);
    case PlatformType::PLATFORM_ELBERT:
      return createElbertWedgeManager(platformMapping, threads);
    case PlatformType::PLATFORM_MERU400BFU:
      return createBspWedgeManager<
          Meru400bfuBspPlatformMapping,
          PlatformType::PLATFORM_MERU400BFU>(platformMapping, threads);
    case PlatformType::PLATFORM_MERU400BIA:
      return createBspWedgeManager<
          Meru400biaBspPlatformMapping,
          PlatformType::PLATFORM_MERU400BIA>(platformMapping, threads);
    case PlatformType::PLATFORM_MERU400BIU:
      return createBspWedgeManager<
          Meru400biuBspPlatformMapping,
          PlatformType::PLATFORM_MERU400BIU>(platformMapping, threads);
    case PlatformType::PLATFORM_MERU800BIA:
    case PlatformType::PLATFORM_MERU800BIAB:
    case PlatformType::PLATFORM_MERU800BIAC:
      return createBspWedgeManager<
          Meru800biaBspPlatformMapping,
          PlatformType::PLATFORM_MERU800BIA>(platformMapping, threads);
    case PlatformType::PLATFORM_MERU800BFA:
    case PlatformType::PLATFORM_MERU800BFA_P1:
      return createBspWedgeManager<
          Meru800bfaBspPlatformMapping,
          PlatformType::PLATFORM_MERU800BFA>(platformMapping, threads);
    case PlatformType::PLATFORM_MONTBLANC:
      return createBspWedgeManager<
          MontblancBspPlatformMapping,
          PlatformType::PLATFORM_MONTBLANC>(platformMapping, threads);
    case PlatformType::PLATFORM_ICECUBE800BC:
      return createBspWedgeManager<
          Icecube800bcBspPlatformMapping,
          PlatformType::PLATFORM_ICECUBE800BC>(platformMapping, threads);
    case PlatformType::PLATFORM_ICETEA800BC:
      return createBspWedgeManager<
          Icetea800bcBspPlatformMapping,
          PlatformType::PLATFORM_ICETEA800BC>(platformMapping, threads);
    case PlatformType::PLATFORM_MINIPACK3N:
      return createBspWedgeManager<
          Minipack3NBspPlatformMapping,
          PlatformType::PLATFORM_MINIPACK3N>(platformMapping, threads);
    case PlatformType::PLATFORM_MORGAN800CC:
      return createBspWedgeManager<
          Morgan800ccBspPlatformMapping,
          PlatformType::PLATFORM_MORGAN800CC>(platformMapping, threads);
    case PlatformType::PLATFORM_WEDGE400C:
      return std::make_unique<Wedge400CManager>(platformMapping, threads);
    case PlatformType::PLATFORM_JANGA800BIC:
      return createBspWedgeManager<
          Janga800bicBspPlatformMapping,
          PlatformType::PLATFORM_JANGA800BIC>(platformMapping, threads);
    case PlatformType::PLATFORM_TAHAN800BC:
      return createBspWedgeManager<
          Tahan800bcBspPlatformMapping,
          PlatformType::PLATFORM_TAHAN800BC>(platformMapping, threads);
    case PlatformType::PLATFORM_FUJI:
    case PlatformType::PLATFORM_MINIPACK:
    case PlatformType::PLATFORM_WEDGE400:
      return createFBWedgeManager(
          std::move(productInfo), platformMapping, threads);
    default:
      return std::make_unique<Wedge40Manager>(platformMapping, threads);
  }
}

template <typename BspPlatformMapping, PlatformType platformType>
std::unique_ptr<WedgeManager> createBspWedgeManager(
    const std::shared_ptr<const PlatformMapping> platformMapping,
    const std::shared_ptr<std::unordered_map<TransceiverID, SlotThreadHelper>>
        threads) {
  auto systemContainer =
      BspGenericSystemContainer<BspPlatformMapping>::getInstance().get();
  return std::make_unique<BspWedgeManager>(
      systemContainer,
      std::make_unique<BspTransceiverApi>(systemContainer),
      platformMapping,
      platformType,
      threads);
}

} // namespace facebook::fboss
