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

} // namespace

std::unique_ptr<SaiPlatform> chooseSaiPlatform(
    std::unique_ptr<PlatformProductInfo> productInfo,
    folly::MacAddress localMac,
    const std::string& platformMappingStr) {
  const auto type = productInfo->getType();
  if (auto platform =
          chooseSaiPlatformImpl(productInfo, localMac, platformMappingStr)) {
    return platform;
  }

  throw FbossError(
      "Unsupported SAI platform type ",
      apache::thrift::util::enumNameSafe(type));
}

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
