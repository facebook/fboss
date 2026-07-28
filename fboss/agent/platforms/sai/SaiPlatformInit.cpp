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
#include "fboss/agent/platforms/sai/SaiPlatformInitImpl.h"
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

std::unique_ptr<SaiPlatform> createGenericSaiPlatform(
    const PlatformDescriptor& descriptor,
    std::unique_ptr<PlatformProductInfo>& productInfo,
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
    case HwAsic::AsicVendor::ASIC_VENDOR_CHENAB:
      return createGenericSaiYangraPlatform(
          std::move(productInfo), localMac, platformMappingStr);
    case HwAsic::AsicVendor::ASIC_VENDOR_CREDO:
    case HwAsic::AsicVendor::ASIC_VENDOR_MARVELL:
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
          *descriptor, productInfo, localMac, platformMappingStr);
    }
  }

  if (auto platform =
          chooseBcmSaiPlatform(productInfo, localMac, platformMappingStr)) {
    return platform;
  }
  if (auto platform =
          chooseTajoSaiPlatform(productInfo, localMac, platformMappingStr)) {
    return platform;
  }
  if (auto platform =
          chooseYangraSaiPlatform(productInfo, localMac, platformMappingStr)) {
    return platform;
  }
  if (auto platform =
          chooseFakeSaiPlatform(productInfo, localMac, platformMappingStr)) {
    return platform;
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
