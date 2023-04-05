/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/platforms/sai/SaiPlatformInit.h"
#include "fboss/lib/platforms/PlatformProductInfo.h"

#include <memory>

#include "fboss/agent/AgentConfig.h"
#include "fboss/agent/Platform.h"
#include "fboss/agent/Utils.h"
#include "fboss/agent/platforms/sai/SaiBcmDarwinPlatform.h"
#include "fboss/agent/platforms/sai/SaiBcmElbertPlatform.h"
#include "fboss/agent/platforms/sai/SaiBcmFujiPlatform.h"
#include "fboss/agent/platforms/sai/SaiBcmGalaxyFCPlatform.h"
#include "fboss/agent/platforms/sai/SaiBcmGalaxyLCPlatform.h"
#include "fboss/agent/platforms/sai/SaiBcmMinipackPlatform.h"
#include "fboss/agent/platforms/sai/SaiBcmMontblancPlatform.h"
#include "fboss/agent/platforms/sai/SaiBcmWedge100Platform.h"
#include "fboss/agent/platforms/sai/SaiBcmWedge400Platform.h"
#include "fboss/agent/platforms/sai/SaiBcmWedge40Platform.h"
#include "fboss/agent/platforms/sai/SaiBcmYampPlatform.h"
#include "fboss/agent/platforms/sai/SaiCloudRipperPlatform.h"
#include "fboss/agent/platforms/sai/SaiLassenPlatform.h"
#include "fboss/agent/platforms/sai/SaiMeru400bfuPlatform.h"
#include "fboss/agent/platforms/sai/SaiMeru400biaPlatform.h"
#include "fboss/agent/platforms/sai/SaiMeru400biuPlatform.h"
#include "fboss/agent/platforms/sai/SaiMeru800biaPlatform.h"
#include "fboss/agent/platforms/sai/SaiSandiaPlatform.h"
#include "fboss/agent/platforms/sai/SaiWedge400CPlatform.h"

namespace facebook::fboss {

std::unique_ptr<SaiPlatform> chooseSaiPlatform(
    std::unique_ptr<PlatformProductInfo> productInfo,
    folly::MacAddress localMac,
    const std::string& platformMappingStr) {
  if (productInfo->getType() == PlatformType::PLATFORM_WEDGE100) {
    return std::make_unique<SaiBcmWedge100Platform>(
        std::move(productInfo), localMac, platformMappingStr);
  } else if (productInfo->getType() == PlatformType::PLATFORM_WEDGE) {
    return std::make_unique<SaiBcmWedge40Platform>(
        std::move(productInfo), localMac, platformMappingStr);
  } else if (productInfo->getType() == PlatformType::PLATFORM_GALAXY_FC) {
    return std::make_unique<SaiBcmGalaxyFCPlatform>(
        std::move(productInfo), localMac, platformMappingStr);
  } else if (productInfo->getType() == PlatformType::PLATFORM_GALAXY_LC) {
    return std::make_unique<SaiBcmGalaxyLCPlatform>(
        std::move(productInfo), localMac, platformMappingStr);
  } else if (productInfo->getType() == PlatformType::PLATFORM_WEDGE400) {
    return std::make_unique<SaiBcmWedge400Platform>(
        std::move(productInfo), localMac, platformMappingStr);
  } else if (productInfo->getType() == PlatformType::PLATFORM_DARWIN) {
    return std::make_unique<SaiBcmDarwinPlatform>(
        std::move(productInfo), localMac, platformMappingStr);
  } else if (productInfo->getType() == PlatformType::PLATFORM_MINIPACK) {
    return std::make_unique<SaiBcmMinipackPlatform>(
        std::move(productInfo), localMac, platformMappingStr);
  } else if (productInfo->getType() == PlatformType::PLATFORM_YAMP) {
    return std::make_unique<SaiBcmYampPlatform>(
        std::move(productInfo), localMac, platformMappingStr);
  } else if (productInfo->getType() == PlatformType::PLATFORM_WEDGE400C) {
    return std::make_unique<SaiWedge400CPlatform>(
        std::move(productInfo), localMac, platformMappingStr);
  } else if (productInfo->getType() == PlatformType::PLATFORM_WEDGE400C_VOQ) {
    return std::make_unique<SaiWedge400CVoqPlatform>(
        std::move(productInfo), localMac, platformMappingStr);
  } else if (
      productInfo->getType() == PlatformType::PLATFORM_WEDGE400C_FABRIC) {
    return std::make_unique<SaiWedge400CFabricPlatform>(
        std::move(productInfo), localMac, platformMappingStr);
  } else if (productInfo->getType() == PlatformType::PLATFORM_CLOUDRIPPER) {
    return std::make_unique<SaiCloudRipperPlatform>(
        std::move(productInfo), localMac, platformMappingStr);
  } else if (productInfo->getType() == PlatformType::PLATFORM_CLOUDRIPPER_VOQ) {
    return std::make_unique<SaiCloudRipperVoqPlatform>(
        std::move(productInfo), localMac, platformMappingStr);
  } else if (
      productInfo->getType() == PlatformType::PLATFORM_CLOUDRIPPER_FABRIC) {
    return std::make_unique<SaiCloudRipperFabricPlatform>(
        std::move(productInfo), localMac, platformMappingStr);
  } else if (productInfo->getType() == PlatformType::PLATFORM_FUJI) {
    return std::make_unique<SaiBcmFujiPlatform>(
        std::move(productInfo), localMac, platformMappingStr);
  } else if (productInfo->getType() == PlatformType::PLATFORM_LASSEN) {
    return std::make_unique<SaiLassenPlatform>(
        std::move(productInfo), localMac, platformMappingStr);
  } else if (productInfo->getType() == PlatformType::PLATFORM_SANDIA) {
    return std::make_unique<SaiSandiaPlatform>(
        std::move(productInfo), localMac, platformMappingStr);
  } else if (productInfo->getType() == PlatformType::PLATFORM_ELBERT) {
    return std::make_unique<SaiBcmElbertPlatform>(
        std::move(productInfo), localMac, platformMappingStr);
  } else if (productInfo->getType() == PlatformType::PLATFORM_MERU400BIU) {
    return std::make_unique<SaiMeru400biuPlatform>(
        std::move(productInfo), localMac, platformMappingStr);
  } else if (productInfo->getType() == PlatformType::PLATFORM_MERU800BIA) {
    return std::make_unique<SaiMeru800biaPlatform>(
        std::move(productInfo), localMac, platformMappingStr);
  } else if (productInfo->getType() == PlatformType::PLATFORM_MERU400BIA) {
    return std::make_unique<SaiMeru400biaPlatform>(
        std::move(productInfo), localMac, platformMappingStr);
  } else if (productInfo->getType() == PlatformType::PLATFORM_MERU400BFU) {
    return std::make_unique<SaiMeru400bfuPlatform>(
        std::move(productInfo), localMac, platformMappingStr);
  } else if (productInfo->getType() == PlatformType::PLATFORM_MONTBLANC) {
    return std::make_unique<SaiBcmMontblancPlatform>(
        std::move(productInfo), localMac, platformMappingStr);
  }

  return nullptr;
} // namespace facebook::fboss

std::unique_ptr<Platform> initSaiPlatform(
    std::unique_ptr<AgentConfig> config,
    uint32_t hwFeaturesDesired) {
  auto productInfo =
      std::make_unique<PlatformProductInfo>(FLAGS_fruid_filepath);
  productInfo->initialize();
  auto localMac = getLocalMacAddress();

  std::string platformMappingStr;
  if (!FLAGS_platform_mapping_override_path.empty()) {
    if (!folly::readFile(
            FLAGS_platform_mapping_override_path.data(), platformMappingStr)) {
      throw FbossError("unable to read ", FLAGS_platform_mapping_override_path);
    }
    XLOG(INFO) << "Overriding platform mapping from "
               << FLAGS_platform_mapping_override_path;
  }
  auto platform =
      chooseSaiPlatform(std::move(productInfo), localMac, platformMappingStr);
  platform->init(std::move(config), hwFeaturesDesired);
  return std::move(platform);
}

} // namespace facebook::fboss
