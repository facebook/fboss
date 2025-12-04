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
#include "fboss/lib/bsp/ladakh800bcls/Ladakh800bclsBspPlatformMapping.h"
#include "fboss/lib/bsp/meru400bfu/Meru400bfuBspPlatformMapping.h"
#include "fboss/lib/bsp/meru400bia/Meru400biaBspPlatformMapping.h"
#include "fboss/lib/bsp/meru400biu/Meru400biuBspPlatformMapping.h"
#include "fboss/lib/bsp/meru800bfa/Meru800bfaBspPlatformMapping.h"
#include "fboss/lib/bsp/meru800bia/Meru800biaBspPlatformMapping.h"
#include "fboss/lib/bsp/minipack3n/Minipack3NBspPlatformMapping.h"
#include "fboss/lib/bsp/montblanc/MontblancBspPlatformMapping.h"
#include "fboss/lib/bsp/morgan800cc/Morgan800ccBspPlatformMapping.h"
#include "fboss/lib/bsp/tahan800bc/Tahan800bcBspPlatformMapping.h"
#include "fboss/lib/bsp/tahansb800bc/Tahansb800bcBspPlatformMapping.h"
#include "fboss/lib/bsp/wedge800bact/Wedge800BACTBspPlatformMapping.h"
#include "fboss/lib/bsp/wedge800cact/Wedge800CACTBspPlatformMapping.h"
#include "fboss/lib/platforms/PlatformProductInfo.h"
#include "fboss/qsfp_service/PortManager.h"
#include "fboss/qsfp_service/platforms/wedge/BspWedgeManager.h"
#include "fboss/qsfp_service/platforms/wedge/GalaxyManager.h"
#include "fboss/qsfp_service/platforms/wedge/Wedge100Manager.h"
#include "fboss/qsfp_service/platforms/wedge/Wedge400CManager.h"
#include "fboss/qsfp_service/platforms/wedge/Wedge400Manager.h"
#include "fboss/qsfp_service/platforms/wedge/Wedge40Manager.h"

#include "fboss/lib/CommonFileUtils.h"

namespace facebook::fboss {
namespace {

struct ManagerInitComponents {
  std::unique_ptr<PlatformProductInfo> productInfo;
  std::shared_ptr<const PlatformMapping> platformMapping;
  const std::shared_ptr<std::unordered_map<TransceiverID, SlotThreadHelper>>
      threads;
};

ManagerInitComponents initializeManagerComponents() {
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

  return {std::move(productInfo), platformMapping, threads};
}

} // namespace

std::unique_ptr<WedgeManager> createWedgeManager(
    std::unique_ptr<PlatformProductInfo> productInfo,
    const std::shared_ptr<const PlatformMapping> platformMapping,
    const std::shared_ptr<std::unordered_map<TransceiverID, SlotThreadHelper>>
        threads) {
  auto mode = productInfo->getType();

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
      return createFBWedgeManager(
          std::move(productInfo), platformMapping, threads);
    case PlatformType::PLATFORM_WEDGE400:
      return std::make_unique<Wedge400Manager>(platformMapping, threads);
    case PlatformType::PLATFORM_TAHANSB800BC:
      return createBspWedgeManager<
          Tahansb800bcBspPlatformMapping,
          PlatformType::PLATFORM_TAHANSB800BC>(platformMapping, threads);
    case PlatformType::PLATFORM_WEDGE800BACT:
      return createBspWedgeManager<
          Wedge800BACTBspPlatformMapping,
          PlatformType::PLATFORM_WEDGE800BACT>(platformMapping, threads);
    case PlatformType::PLATFORM_WEDGE800CACT:
      return createBspWedgeManager<
          Wedge800CACTBspPlatformMapping,
          PlatformType::PLATFORM_WEDGE800CACT>(platformMapping, threads);
    case PlatformType::PLATFORM_LADAKH800BCLS:
      return createBspWedgeManager<
          Ladakh800bclsBspPlatformMapping,
          PlatformType::PLATFORM_LADAKH800BCLS>(platformMapping, threads);
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

std::pair<std::unique_ptr<WedgeManager>, std::unique_ptr<PortManager>>
createQsfpManagers() {
  auto [productInfo, platformMapping, threads] = initializeManagerComponents();

  const auto platformType = productInfo->getType();
  auto wedgeManager =
      createWedgeManager(std::move(productInfo), platformMapping, threads);
  auto phyManager =
      createPhyManager(platformType, platformMapping.get(), wedgeManager.get());

  std::unique_ptr<PortManager> portManager{nullptr};
  if (FLAGS_port_manager_mode) {
    // When port_manager_mode is enabled, we want Port Manager to own the
    // PhyManager.
    portManager = std::make_unique<PortManager>(
        wedgeManager.get(),
        std::move(phyManager),
        platformMapping,
        threads,
        wedgeManager->getFsdbSyncManager());
  } else {
    if (phyManager) {
      wedgeManager->setPhyManager(std::move(phyManager));
    }
  }

  return std::
      make_pair<std::unique_ptr<WedgeManager>, std::unique_ptr<PortManager>>(
          std::move(wedgeManager), std::move(portManager));
}

} // namespace facebook::fboss
