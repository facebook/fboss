/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/sai/SaiPlatformInitImpl.h"

#include "fboss/agent/FbossError.h"
#include "fboss/agent/platforms/common/PlatformMapping.h"
#include "fboss/agent/platforms/common/blackwolf800banw/Blackwolf800banwPlatformMapping.h"
#include "fboss/agent/platforms/common/icecube800banw/Icecube800banwPlatformMapping.h"
#include "fboss/agent/platforms/common/icecube800bc/Icecube800bcPlatformMapping.h"
#include "fboss/agent/platforms/common/icetea800bc/Icetea800bcPlatformMapping.h"
#include "fboss/agent/platforms/common/j4sim/J4SimPlatformMapping.h"
#include "fboss/agent/platforms/common/janga800bic/Janga800bicPlatformMapping.h"
#include "fboss/agent/platforms/common/ladakh800bcls/Ladakh800bclsPlatformMapping.h"
#include "fboss/agent/platforms/common/leh800bcls/Leh800bclsPlatformMapping.h"
#include "fboss/agent/platforms/common/meru800bfa/Meru800bfaP1PlatformMapping.h"
#include "fboss/agent/platforms/common/meru800bfa/Meru800bfaPlatformMapping.h"
#include "fboss/agent/platforms/common/meru800bia/Meru800biaPlatformMapping.h"
#include "fboss/agent/platforms/common/minipack3bta/Minipack3BTAPlatformMapping.h"
#include "fboss/agent/platforms/common/montblanc/MontblancPlatformMapping.h"
#include "fboss/agent/platforms/common/saintpaul/SaintpaulPlatformMapping.h"
#include "fboss/agent/platforms/common/tahan800bc/Tahan800bcPlatformMapping.h"
#include "fboss/agent/platforms/common/tahansb800bc/Tahansb800bcPlatformMapping.h"
#include "fboss/agent/platforms/common/wedge800bact/Wedge800BACTPlatformMapping.h"
#include "fboss/agent/platforms/sai/GenericSaiBcmPlatform.h"
#include "fboss/agent/platforms/sai/SaiBcmDarwinPlatform.h"
#include "fboss/agent/platforms/sai/SaiBcmElbertPlatform.h"
#include "fboss/agent/platforms/sai/SaiBcmFujiPlatform.h"
#include "fboss/agent/platforms/sai/SaiBcmMinipackPlatform.h"
#include "fboss/agent/platforms/sai/SaiBcmWedge100Platform.h"
#include "fboss/agent/platforms/sai/SaiBcmWedge400Platform.h"
#include "fboss/agent/platforms/sai/SaiBcmYampPlatform.h"
#include "fboss/lib/platforms/PlatformDescriptor.h"

#include "thrift/lib/cpp/util/EnumUtils.h"

namespace facebook::fboss {
namespace {

bool useGenericSaiBcmPlatform(PlatformType type) {
  return type == PlatformType::PLATFORM_BLACKWOLF800BANW ||
      type == PlatformType::PLATFORM_J4SIM ||
      type == PlatformType::PLATFORM_JANGA800BIC ||
      type == PlatformType::PLATFORM_MERU800BIA ||
      type == PlatformType::PLATFORM_MERU800BIAB ||
      type == PlatformType::PLATFORM_MERU800BIAC ||
      type == PlatformType::PLATFORM_MERU800BFA ||
      type == PlatformType::PLATFORM_MERU800BFA_P1 ||
      type == PlatformType::PLATFORM_SAINTPAUL ||
      type == PlatformType::PLATFORM_WEDGE800BACT ||
      type == PlatformType::PLATFORM_WEDGE800BNHP ||
      type == PlatformType::PLATFORM_ICECUBE800BC ||
      type == PlatformType::PLATFORM_ICECUBE800BANW ||
      type == PlatformType::PLATFORM_ICETEA800BC ||
      type == PlatformType::PLATFORM_LADAKH800BCLS ||
      type == PlatformType::PLATFORM_LEH800BCLS ||
      type == PlatformType::PLATFORM_MINIPACK3BTA ||
      type == PlatformType::PLATFORM_MONTBLANC ||
      type == PlatformType::PLATFORM_TAHAN800BC ||
      type == PlatformType::PLATFORM_TAHANSB800BC;
}

std::unique_ptr<PlatformMapping> createGenericSaiBcmPlatformMapping(
    PlatformType type,
    const std::string& platformMappingStr) {
  if (!platformMappingStr.empty()) {
    return std::make_unique<PlatformMapping>(platformMappingStr);
  }
  if (!FLAGS_platform_descriptor_config_path.empty()) {
    throw FbossError(
        "Generic SAI platform descriptor is missing platform mapping");
  }

  switch (type) {
    case PlatformType::PLATFORM_BLACKWOLF800BANW:
      return std::make_unique<Blackwolf800banwPlatformMapping>();
    case PlatformType::PLATFORM_WEDGE800BACT:
    case PlatformType::PLATFORM_WEDGE800BNHP:
      return std::make_unique<Wedge800BACTPlatformMapping>();
    case PlatformType::PLATFORM_ICECUBE800BC:
      return std::make_unique<Icecube800bcPlatformMapping>();
    case PlatformType::PLATFORM_ICECUBE800BANW:
      return std::make_unique<Icecube800banwPlatformMapping>();
    case PlatformType::PLATFORM_ICETEA800BC:
      return std::make_unique<Icetea800bcPlatformMapping>();
    case PlatformType::PLATFORM_J4SIM:
      return std::make_unique<J4SimPlatformMapping>();
    case PlatformType::PLATFORM_JANGA800BIC:
      return std::make_unique<Janga800bicPlatformMapping>();
    case PlatformType::PLATFORM_LADAKH800BCLS:
      return std::make_unique<Ladakh800bclsPlatformMapping>();
    case PlatformType::PLATFORM_LEH800BCLS:
      return std::make_unique<Leh800bclsPlatformMapping>();
    case PlatformType::PLATFORM_MERU800BIA:
    case PlatformType::PLATFORM_MERU800BIAB:
    case PlatformType::PLATFORM_MERU800BIAC:
      return std::make_unique<Meru800biaPlatformMapping>();
    case PlatformType::PLATFORM_MERU800BFA:
      return std::make_unique<Meru800bfaPlatformMapping>();
    case PlatformType::PLATFORM_MERU800BFA_P1:
      return std::make_unique<Meru800bfaP1PlatformMapping>();
    case PlatformType::PLATFORM_MINIPACK3BTA:
      return std::make_unique<Minipack3BTAPlatformMapping>();
    case PlatformType::PLATFORM_MONTBLANC:
      return std::make_unique<MontblancPlatformMapping>();
    case PlatformType::PLATFORM_SAINTPAUL:
      return std::make_unique<SaintpaulPlatformMapping>();
    case PlatformType::PLATFORM_TAHAN800BC:
      return std::make_unique<Tahan800bcPlatformMapping>();
    case PlatformType::PLATFORM_TAHANSB800BC:
      return std::make_unique<Tahansb800bcPlatformMapping>();
    default:
      throw FbossError(
          "Generic BCM SAI platform is missing platform mapping for platform type ",
          apache::thrift::util::enumNameSafe(type));
  }
}

} // namespace

std::unique_ptr<SaiPlatform> createGenericSaiBcmPlatform(
    std::unique_ptr<PlatformProductInfo> productInfo,
    folly::MacAddress localMac,
    const std::string& platformMappingStr) {
  const auto platformType = productInfo->getType();
  return std::make_unique<GenericSaiBcmPlatform>(
      std::move(productInfo),
      createGenericSaiBcmPlatformMapping(platformType, platformMappingStr),
      localMac);
}

std::unique_ptr<SaiPlatform> chooseBcmSaiPlatform(
    std::unique_ptr<PlatformProductInfo>& productInfo,
    folly::MacAddress localMac,
    const std::string& platformMappingStr) {
  const auto type = productInfo->getType();
  if (type == PlatformType::PLATFORM_WEDGE100) {
    return std::make_unique<SaiBcmWedge100Platform>(
        std::move(productInfo), localMac, platformMappingStr);
  }
  if (type == PlatformType::PLATFORM_WEDGE400 ||
      type == PlatformType::PLATFORM_WEDGE400_GRANDTETON) {
    return std::make_unique<SaiBcmWedge400Platform>(
        std::move(productInfo), type, localMac, platformMappingStr);
  }
  if (type == PlatformType::PLATFORM_DARWIN ||
      type == PlatformType::PLATFORM_DARWIN48V) {
    return std::make_unique<SaiBcmDarwinPlatform>(
        std::move(productInfo), localMac, platformMappingStr);
  }
  if (type == PlatformType::PLATFORM_MINIPACK) {
    return std::make_unique<SaiBcmMinipackPlatform>(
        std::move(productInfo), localMac, platformMappingStr);
  }
  if (type == PlatformType::PLATFORM_YAMP) {
    return std::make_unique<SaiBcmYampPlatform>(
        std::move(productInfo), localMac, platformMappingStr);
  }
  if (type == PlatformType::PLATFORM_FUJI) {
    return std::make_unique<SaiBcmFujiPlatform>(
        std::move(productInfo), localMac, platformMappingStr);
  }
  if (type == PlatformType::PLATFORM_ELBERT) {
    return std::make_unique<SaiBcmElbertPlatform>(
        std::move(productInfo), localMac, platformMappingStr);
  }
  if (useGenericSaiBcmPlatform(type)) {
    return createGenericSaiBcmPlatform(
        std::move(productInfo), localMac, platformMappingStr);
  }
  return nullptr;
}

} // namespace facebook::fboss
