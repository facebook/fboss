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
#include "fboss/lib/platforms/PlatformDescriptor.h"
#include "fboss/lib/platforms/PlatformProductInfo.h"

#include <memory>

#include "fboss/agent/AgentConfig.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/Platform.h"
#include "fboss/agent/Utils.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/platforms/common/PlatformMapping.h"
#include "fboss/agent/platforms/common/blackwolf800banw/Blackwolf800banwPlatformMapping.h"
#include "fboss/agent/platforms/common/icecube800banw/Icecube800banwPlatformMapping.h"
#include "fboss/agent/platforms/common/icecube800bc/Icecube800bcPlatformMapping.h"
#include "fboss/agent/platforms/common/icetea800bc/Icetea800bcPlatformMapping.h"
#include "fboss/agent/platforms/common/j4sim/J4SimPlatformMapping.h"
#include "fboss/agent/platforms/common/janga800bic/Janga800bicPlatformMapping.h"
#include "fboss/agent/platforms/common/ladakh800bcls/Ladakh800bclsPlatformMapping.h"
#include "fboss/agent/platforms/common/leh800bcls/Leh800bclsPlatformMapping.h"
#include "fboss/agent/platforms/common/m5120csc/M5120CSCPlatformMapping.h"
#include "fboss/agent/platforms/common/meru800bfa/Meru800bfaP1PlatformMapping.h"
#include "fboss/agent/platforms/common/meru800bfa/Meru800bfaPlatformMapping.h"
#include "fboss/agent/platforms/common/meru800bia/Meru800biaPlatformMapping.h"
#include "fboss/agent/platforms/common/minipack3bta/Minipack3BTAPlatformMapping.h"
#include "fboss/agent/platforms/common/montblanc/MontblancPlatformMapping.h"
#include "fboss/agent/platforms/common/morgan800cc/Morgan800ccPlatformMapping.h"
#include "fboss/agent/platforms/common/saintpaul/SaintpaulPlatformMapping.h"
#include "fboss/agent/platforms/common/tahan800bc/Tahan800bcPlatformMapping.h"
#include "fboss/agent/platforms/common/tahansb800bc/Tahansb800bcPlatformMapping.h"
#include "fboss/agent/platforms/common/wedge800bact/Wedge800BACTPlatformMapping.h"
#include "fboss/agent/platforms/common/wedge800cact/Wedge800CACTPlatformMapping.h"
#include "fboss/agent/platforms/sai/GenericSaiBcmPlatform.h"
#include "fboss/agent/platforms/sai/GenericSaiTajoPlatform.h"
#include "fboss/agent/platforms/sai/SaiBcmDarwinPlatform.h"
#include "fboss/agent/platforms/sai/SaiBcmElbertPlatform.h"
#include "fboss/agent/platforms/sai/SaiBcmFujiPlatform.h"
#include "fboss/agent/platforms/sai/SaiBcmMinipackPlatform.h"
#include "fboss/agent/platforms/sai/SaiBcmWedge100Platform.h"
#include "fboss/agent/platforms/sai/SaiBcmWedge400Platform.h"
#include "fboss/agent/platforms/sai/SaiBcmYampPlatform.h"
#include "fboss/agent/platforms/sai/SaiFakePlatform.h"
#include "fboss/agent/platforms/sai/SaiMinipack3NPlatform.h"
#include "fboss/agent/platforms/sai/SaiWedge400CPlatform.h"
#include "fboss/agent/platforms/sai/SaiYangra2Platform.h"
#include "fboss/agent/platforms/sai/SaiYangraPlatform.h"
#include "thrift/lib/cpp/util/EnumUtils.h"

namespace facebook::fboss {
namespace {

std::string getPlatformMappingForInit(PlatformType type) {
  std::string platformMappingStr;
  if (!FLAGS_platform_mapping_override_path.empty()) {
    if (!folly::readFile(
            FLAGS_platform_mapping_override_path.data(), platformMappingStr)) {
      throw FbossError("unable to read ", FLAGS_platform_mapping_override_path);
    }
    XLOG(INFO) << "Overriding platform mapping from "
               << FLAGS_platform_mapping_override_path;
    return platformMappingStr;
  }

  if (!FLAGS_platform_descriptor_config_path.empty()) {
    const auto& registry = PlatformDescriptorRegistry::get();
    if (registry.getDescriptor(type)) {
      auto descriptorPlatformMapping = registry.loadPlatformMapping(type);
      if (descriptorPlatformMapping.has_value()) {
        return *descriptorPlatformMapping;
      }
    }
  }

  return platformMappingStr;
}

cfg::SwitchInfo makeAsicVendorProbeSwitchInfo(cfg::AsicType asicType) {
  cfg::SwitchInfo switchInfo;
  switchInfo.switchType() = cfg::SwitchType::NPU;
  switchInfo.asicType() = asicType;
  switchInfo.switchIndex() = 0;
  switchInfo.switchMac() = "02:00:00:00:00:01";
  switchInfo.systemPortRanges()->systemPortRanges() = {};
  return switchInfo;
}

std::unique_ptr<SaiPlatform> createGenericSaiBcmPlatform(
    std::unique_ptr<PlatformProductInfo> productInfo,
    folly::MacAddress localMac,
    std::unique_ptr<PlatformMapping> platformMapping) {
  return std::make_unique<GenericSaiBcmPlatform>(
      std::move(productInfo), std::move(platformMapping), localMac);
}

std::unique_ptr<PlatformMapping> createGenericSaiPlatformMapping(
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
    case PlatformType::PLATFORM_M5120CSC:
      return std::make_unique<M5120CSCPlatformMapping>();
    case PlatformType::PLATFORM_MINIPACK3BTA:
      return std::make_unique<Minipack3BTAPlatformMapping>();
    case PlatformType::PLATFORM_MONTBLANC:
      return std::make_unique<MontblancPlatformMapping>();
    case PlatformType::PLATFORM_MORGAN800CC:
      return std::make_unique<Morgan800ccPlatformMapping>();
    case PlatformType::PLATFORM_SAINTPAUL:
      return std::make_unique<SaintpaulPlatformMapping>();
    case PlatformType::PLATFORM_TAHAN800BC:
      return std::make_unique<Tahan800bcPlatformMapping>();
    case PlatformType::PLATFORM_TAHANSB800BC:
      return std::make_unique<Tahansb800bcPlatformMapping>();
    case PlatformType::PLATFORM_WEDGE800CACT:
      return std::make_unique<Wedge800CACTPlatformMapping>();
    default:
      throw FbossError(
          "Generic SAI platform is missing platform mapping for platform type ",
          apache::thrift::util::enumNameSafe(type));
  }
}

std::unique_ptr<SaiPlatform> createGenericSaiBcmPlatform(
    std::unique_ptr<PlatformProductInfo> productInfo,
    folly::MacAddress localMac,
    const std::string& platformMappingStr) {
  const auto platformType = productInfo->getType();
  return createGenericSaiBcmPlatform(
      std::move(productInfo),
      localMac,
      createGenericSaiPlatformMapping(platformType, platformMappingStr));
}

std::unique_ptr<SaiPlatform> createGenericSaiTajoPlatform(
    std::unique_ptr<PlatformProductInfo> productInfo,
    folly::MacAddress localMac,
    const std::string& platformMappingStr) {
  const auto platformType = productInfo->getType();
  return std::make_unique<GenericSaiTajoPlatform>(
      std::move(productInfo),
      createGenericSaiPlatformMapping(platformType, platformMappingStr),
      localMac);
}

std::unique_ptr<SaiPlatform> createGenericSaiPlatform(
    const PlatformDescriptor& descriptor,
    std::unique_ptr<PlatformProductInfo> productInfo,
    folly::MacAddress localMac,
    const std::string& platformMappingStr) {
  const auto asicType = descriptor.asicType().value();
  auto probeSwitchInfo = makeAsicVendorProbeSwitchInfo(asicType);
  auto probeAsic =
      HwAsic::makeAsic(0, probeSwitchInfo, std::nullopt, std::nullopt);
  const auto asicVendor = probeAsic->getAsicVendor();
  switch (asicVendor) {
    case HwAsic::AsicVendor::ASIC_VENDOR_BCM:
      return createGenericSaiBcmPlatform(
          std::move(productInfo), localMac, platformMappingStr);
    case HwAsic::AsicVendor::ASIC_VENDOR_TAJO:
      return createGenericSaiTajoPlatform(
          std::move(productInfo), localMac, platformMappingStr);
    case HwAsic::AsicVendor::ASIC_VENDOR_CREDO:
    case HwAsic::AsicVendor::ASIC_VENDOR_MARVELL:
    case HwAsic::AsicVendor::ASIC_VENDOR_CHENAB:
    case HwAsic::AsicVendor::ASIC_VENDOR_MOCK:
    case HwAsic::AsicVendor::ASIC_VENDOR_FAKE:
      break;
  }
  throw FbossError(
      "Unsupported generic SAI ASIC vendor ",
      static_cast<int>(asicVendor),
      " for ASIC type ",
      apache::thrift::util::enumNameSafe(asicType));
}

} // namespace

std::unique_ptr<SaiPlatform> chooseSaiPlatform(
    std::unique_ptr<PlatformProductInfo> productInfo,
    folly::MacAddress localMac,
    const std::string& platformMappingStr) {
  if (!FLAGS_platform_descriptor_config_path.empty()) {
    auto type = productInfo->getType();
    const auto& registry = PlatformDescriptorRegistry::get();
    auto descriptor = registry.getDescriptor(type);
    if (descriptor) {
      return createGenericSaiPlatform(
          *descriptor, std::move(productInfo), localMac, platformMappingStr);
    }
  }

  if (productInfo->getType() == PlatformType::PLATFORM_WEDGE100) {
    return std::make_unique<SaiBcmWedge100Platform>(
        std::move(productInfo), localMac, platformMappingStr);
  } else if (
      productInfo->getType() == PlatformType::PLATFORM_WEDGE400 ||
      productInfo->getType() == PlatformType::PLATFORM_WEDGE400_GRANDTETON) {
    auto type = productInfo->getType();
    return std::make_unique<SaiBcmWedge400Platform>(
        std::move(productInfo), type, localMac, platformMappingStr);
  } else if (
      productInfo->getType() == PlatformType::PLATFORM_DARWIN ||
      productInfo->getType() == PlatformType::PLATFORM_DARWIN48V) {
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
  } else if (productInfo->getType() == PlatformType::PLATFORM_FUJI) {
    return std::make_unique<SaiBcmFujiPlatform>(
        std::move(productInfo), localMac, platformMappingStr);
  } else if (productInfo->getType() == PlatformType::PLATFORM_ELBERT) {
    return std::make_unique<SaiBcmElbertPlatform>(
        std::move(productInfo), localMac, platformMappingStr);
  } else if (productInfo->getType() == PlatformType::PLATFORM_YANGRA) {
    return std::make_unique<SaiYangraPlatform>(
        std::move(productInfo), localMac, platformMappingStr);
  } else if (productInfo->getType() == PlatformType::PLATFORM_MINIPACK3N) {
    return std::make_unique<SaiMinipack3NPlatform>(
        std::move(productInfo), localMac, platformMappingStr);
  } else if (
      productInfo->getType() == PlatformType::PLATFORM_BLACKWOLF800BANW ||
      productInfo->getType() == PlatformType::PLATFORM_J4SIM ||
      productInfo->getType() == PlatformType::PLATFORM_JANGA800BIC ||
      productInfo->getType() == PlatformType::PLATFORM_MERU800BIA ||
      productInfo->getType() == PlatformType::PLATFORM_MERU800BIAB ||
      productInfo->getType() == PlatformType::PLATFORM_MERU800BIAC ||
      productInfo->getType() == PlatformType::PLATFORM_MERU800BFA ||
      productInfo->getType() == PlatformType::PLATFORM_MERU800BFA_P1 ||
      productInfo->getType() == PlatformType::PLATFORM_SAINTPAUL ||
      productInfo->getType() == PlatformType::PLATFORM_WEDGE800BACT ||
      productInfo->getType() == PlatformType::PLATFORM_WEDGE800BNHP ||
      productInfo->getType() == PlatformType::PLATFORM_ICECUBE800BC ||
      productInfo->getType() == PlatformType::PLATFORM_ICECUBE800BANW ||
      productInfo->getType() == PlatformType::PLATFORM_ICETEA800BC ||
      productInfo->getType() == PlatformType::PLATFORM_LADAKH800BCLS ||
      productInfo->getType() == PlatformType::PLATFORM_LEH800BCLS ||
      productInfo->getType() == PlatformType::PLATFORM_MINIPACK3BTA ||
      productInfo->getType() == PlatformType::PLATFORM_MONTBLANC ||
      productInfo->getType() == PlatformType::PLATFORM_TAHAN800BC ||
      productInfo->getType() == PlatformType::PLATFORM_TAHANSB800BC) {
    return createGenericSaiBcmPlatform(
        std::move(productInfo), localMac, platformMappingStr);
  } else if (
      productInfo->getType() == PlatformType::PLATFORM_WEDGE800CACT ||
      productInfo->getType() == PlatformType::PLATFORM_M5120CSC ||
      productInfo->getType() == PlatformType::PLATFORM_MORGAN800CC) {
    return createGenericSaiTajoPlatform(
        std::move(productInfo), localMac, platformMappingStr);
  } else if (productInfo->getType() == PlatformType::PLATFORM_YANGRA2) {
    return std::make_unique<SaiYangra2Platform>(
        std::move(productInfo), localMac, platformMappingStr);
  } else if (productInfo->getType() == PlatformType::PLATFORM_FAKE_SAI) {
    return std::make_unique<SaiFakePlatform>(std::move(productInfo));
  }

  return nullptr;
} // namespace facebook::fboss

std::unique_ptr<Platform> initSaiPlatform(
    std::unique_ptr<AgentConfig> config,
    uint32_t hwFeaturesDesired,
    int16_t switchIndex) {
  auto productInfo =
      std::make_unique<PlatformProductInfo>(FLAGS_fruid_filepath);
  productInfo->initialize();
  auto localMac = getLocalMacAddress();

  auto platformMappingStr = getPlatformMappingForInit(productInfo->getType());
  auto platform =
      chooseSaiPlatform(std::move(productInfo), localMac, platformMappingStr);
  platform->init(std::move(config), hwFeaturesDesired, switchIndex);
  return std::move(platform);
}

} // namespace facebook::fboss
