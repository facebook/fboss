/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <folly/FileUtil.h>
#include <folly/logging/xlog.h>

#include "fboss/agent/FbossError.h"
#include "fboss/agent/platforms/common/PlatformMapping.h"
#include "fboss/agent/platforms/common/PlatformMappingUtils.h"
#include "fboss/agent/platforms/common/cloud_ripper/CloudRipperPlatformMapping.h"
#include "fboss/agent/platforms/common/darwin/DarwinPlatformMapping.h"
#include "fboss/agent/platforms/common/elbert/ElbertPlatformMapping.h"
#include "fboss/agent/platforms/common/fake_test/FakeTestPlatformMapping.h"
#include "fboss/agent/platforms/common/fuji/FujiPlatformMapping.h"
#include "fboss/agent/platforms/common/galaxy/GalaxyFCPlatformMapping.h"
#include "fboss/agent/platforms/common/galaxy/GalaxyLCPlatformMapping.h"
#include "fboss/agent/platforms/common/janga800bic/Janga800bicPlatformMapping.h"
#include "fboss/agent/platforms/common/meru400bfu/Meru400bfuPlatformMapping.h"
#include "fboss/agent/platforms/common/meru400bia/Meru400biaPlatformMapping.h"
#include "fboss/agent/platforms/common/meru400biu/Meru400biuPlatformMapping.h"
#include "fboss/agent/platforms/common/meru800bfa/Meru800bfaP1PlatformMapping.h"
#include "fboss/agent/platforms/common/meru800bfa/Meru800bfaPlatformMapping.h"
#include "fboss/agent/platforms/common/meru800bia/Meru800biaPlatformMapping.h"
#include "fboss/agent/platforms/common/minipack/MinipackPlatformMapping.h"
#include "fboss/agent/platforms/common/montblanc/MontblancPlatformMapping.h"
#include "fboss/agent/platforms/common/morgan800cc/Morgan800ccPlatformMapping.h"
#include "fboss/agent/platforms/common/tahan800bc/Tahan800bcPlatformMapping.h"
#include "fboss/agent/platforms/common/wedge100/Wedge100PlatformMapping.h"
#include "fboss/agent/platforms/common/wedge40/Wedge40PlatformMapping.h"
#include "fboss/agent/platforms/common/wedge400/Wedge400GrandTetonPlatformMapping.h"
#include "fboss/agent/platforms/common/wedge400/Wedge400PlatformMapping.h"
#include "fboss/agent/platforms/common/wedge400/Wedge400PlatformUtil.h"
#include "fboss/agent/platforms/common/wedge400c/Wedge400CGrandTetonPlatformMapping.h"
#include "fboss/agent/platforms/common/wedge400c/Wedge400CPlatformMapping.h"
#include "fboss/agent/platforms/common/wedge400c/Wedge400CPlatformUtil.h"
#include "fboss/agent/platforms/common/yamp/YampPlatformMapping.h"

namespace {
std::vector<int> getFakeSaiControllingPortIDs() {
  std::vector<int> controllingPorts;
  for (int i = 0; i < 128; i += 4) {
    controllingPorts.push_back(i);
  }
  return controllingPorts;
}
} // namespace

namespace facebook::fboss::utility {

std::unique_ptr<PlatformMapping> initPlatformMapping(PlatformType type) {
  std::string platformMappingStr;
  if (!FLAGS_platform_mapping_override_path.empty()) {
    if (!folly::readFile(
            FLAGS_platform_mapping_override_path.data(), platformMappingStr)) {
      throw FbossError("unable to read ", FLAGS_platform_mapping_override_path);
    }
    XLOG(INFO) << "Overriding platform mapping from "
               << FLAGS_platform_mapping_override_path;
  }
  switch (type) {
    case PlatformType::PLATFORM_WEDGE:
      return platformMappingStr.empty()
          ? std::make_unique<Wedge40PlatformMapping>()
          : std::make_unique<Wedge40PlatformMapping>(platformMappingStr);
    case PlatformType::PLATFORM_WEDGE100:
      return platformMappingStr.empty()
          ? std::make_unique<Wedge100PlatformMapping>()
          : std::make_unique<Wedge100PlatformMapping>(platformMappingStr);
    case PlatformType::PLATFORM_GALAXY_LC:
      return std::make_unique<GalaxyLCPlatformMapping>(
          GalaxyLCPlatformMapping::getLinecardName());
    case PlatformType::PLATFORM_GALAXY_FC:
      return std::make_unique<GalaxyFCPlatformMapping>(
          GalaxyFCPlatformMapping::getFabriccardName());
    case PlatformType::PLATFORM_MINIPACK:
      return std::make_unique<MinipackPlatformMapping>(
          ExternalPhyVersion::MILN5_2, platformMappingStr);
    case PlatformType::PLATFORM_YAMP:
      return std::make_unique<YampPlatformMapping>(platformMappingStr);
    case PlatformType::PLATFORM_FUJI:
      return std::make_unique<FujiPlatformMapping>(platformMappingStr);
    case PlatformType::PLATFORM_ELBERT:
      return std::make_unique<ElbertPlatformMapping>(platformMappingStr);
    case PlatformType::PLATFORM_WEDGE400:
    case PlatformType::PLATFORM_WEDGE400_GRANDTETON:
      if (type == PlatformType::PLATFORM_WEDGE400_GRANDTETON ||
          utility::isWedge400PlatformRackTypeInference()) {
        return platformMappingStr.empty()
            ? std::make_unique<Wedge400GrandTetonPlatformMapping>()
            : std::make_unique<Wedge400GrandTetonPlatformMapping>(
                  platformMappingStr);
      } else {
        return platformMappingStr.empty()
            ? std::make_unique<Wedge400PlatformMapping>()
            : std::make_unique<Wedge400PlatformMapping>(platformMappingStr);
      }
    case PlatformType::PLATFORM_WEDGE400C:
    case PlatformType::PLATFORM_WEDGE400C_GRANDTETON:
      if (type == PlatformType::PLATFORM_WEDGE400C_GRANDTETON ||
          utility::isWedge400CPlatformRackTypeInference()) {
        return platformMappingStr.empty()
            ? std::make_unique<Wedge400CGrandTetonPlatformMapping>()
            : std::make_unique<Wedge400CGrandTetonPlatformMapping>(
                  platformMappingStr);
      } else {
        return platformMappingStr.empty()
            ? std::make_unique<Wedge400CPlatformMapping>()
            : std::make_unique<Wedge400CPlatformMapping>(platformMappingStr);
      }
    case PlatformType::PLATFORM_CLOUDRIPPER:
      return platformMappingStr.empty()
          ? std::make_unique<CloudRipperPlatformMapping>()
          : std::make_unique<CloudRipperPlatformMapping>(platformMappingStr);
    case PlatformType::PLATFORM_DARWIN:
      return platformMappingStr.empty()
          ? std::make_unique<DarwinPlatformMapping>()
          : std::make_unique<DarwinPlatformMapping>(platformMappingStr);
    case PlatformType::PLATFORM_MONTBLANC:
      return platformMappingStr.empty()
          ? std::make_unique<MontblancPlatformMapping>()
          : std::make_unique<MontblancPlatformMapping>(platformMappingStr);
    case PlatformType::PLATFORM_JANGA800BIC:
      return platformMappingStr.empty()
          ? std::make_unique<Janga800bicPlatformMapping>()
          : std::make_unique<Janga800bicPlatformMapping>(platformMappingStr);
    case PlatformType::PLATFORM_TAHAN800BC:
      return platformMappingStr.empty()
          ? std::make_unique<Tahan800bcPlatformMapping>()
          : std::make_unique<Tahan800bcPlatformMapping>(platformMappingStr);
    case PlatformType::PLATFORM_FAKE_WEDGE:
    case PlatformType::PLATFORM_FAKE_WEDGE40:
      return platformMappingStr.empty()
          ? std::make_unique<Wedge40PlatformMapping>()
          : std::make_unique<Wedge40PlatformMapping>(platformMappingStr);
    case PlatformType::PLATFORM_WEDGE400C_SIM:
      return platformMappingStr.empty()
          ? std::make_unique<Wedge400CPlatformMapping>()
          : std::make_unique<Wedge400CPlatformMapping>(platformMappingStr);
    case PlatformType::PLATFORM_MERU400BIU:
      return platformMappingStr.empty()
          ? std::make_unique<Meru400biuPlatformMapping>()
          : std::make_unique<Meru400biuPlatformMapping>(platformMappingStr);
    case PlatformType::PLATFORM_MERU800BIA:
      return platformMappingStr.empty()
          ? std::make_unique<Meru800biaPlatformMapping>()
          : std::make_unique<Meru800biaPlatformMapping>(platformMappingStr);
    case PlatformType::PLATFORM_MERU800BFA:
      return platformMappingStr.empty()
          ? std::make_unique<Meru800bfaPlatformMapping>()
          : std::make_unique<Meru800bfaPlatformMapping>(platformMappingStr);
    case PlatformType::PLATFORM_MERU800BFA_P1:
      return platformMappingStr.empty()
          ? std::make_unique<Meru800bfaP1PlatformMapping>()
          : std::make_unique<Meru800bfaP1PlatformMapping>(platformMappingStr);
    case PlatformType::PLATFORM_MERU400BFU:
      return platformMappingStr.empty()
          ? std::make_unique<Meru400bfuPlatformMapping>()
          : std::make_unique<Meru400bfuPlatformMapping>(platformMappingStr);
    case PlatformType::PLATFORM_MERU400BIA:
      return platformMappingStr.empty()
          ? std::make_unique<Meru400biaPlatformMapping>()
          : std::make_unique<Meru400biaPlatformMapping>(platformMappingStr);
    case PlatformType::PLATFORM_MORGAN800CC:
      return platformMappingStr.empty()
          ? std::make_unique<Morgan800ccPlatformMapping>()
          : std::make_unique<Morgan800ccPlatformMapping>(platformMappingStr);
    case PlatformType::PLATFORM_FAKE_SAI: {
      std::vector<int> controllingPorts = getFakeSaiControllingPortIDs();
      return std::make_unique<FakeTestPlatformMapping>(controllingPorts);
    }
    case PlatformType::PLATFORM_LASSEN_DEPRECATED:
    case PlatformType::PLATFORM_CLOUDRIPPER_FABRIC:
    case PlatformType::PLATFORM_CLOUDRIPPER_VOQ:
    case PlatformType::PLATFORM_WEDGE400C_FABRIC:
    case PlatformType::PLATFORM_WEDGE400C_VOQ:
    case PlatformType::PLATFORM_SANDIA:
      throw FbossError("Unsupported platform type");
  }
  return nullptr;
}
} // namespace facebook::fboss::utility
