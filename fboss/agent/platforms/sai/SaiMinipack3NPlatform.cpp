/*
 *  Copyright (c) 2023-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/sai/SaiMinipack3NPlatform.h"
#include "fboss/agent/platforms/common/minipack3n/Minipack3NPlatformMapping.h"
#include "fboss/agent/platforms/sai/SaiYangraPlatform.h"

namespace facebook::fboss {

SaiMinipack3NPlatform::SaiMinipack3NPlatform(
    std::unique_ptr<PlatformProductInfo> productInfo,
    folly::MacAddress localMac,
    const std::string& platformMappingStr)
    : SaiYangraPlatform(
          std::move(productInfo),
          localMac,
          platformMappingStr.empty()
              ? std::make_unique<Minipack3NPlatformMapping>()
              : std::make_unique<Minipack3NPlatformMapping>(
                    platformMappingStr)) {}

const std::unordered_map<std::string, std::string>
SaiMinipack3NPlatform::getSaiProfileVendorExtensionValues() const {
  auto kv_map = SaiYangraPlatform::getSaiProfileVendorExtensionValues();
  kv_map.insert(std::make_pair("SAI_KEY_INDEPENDENT_MODULE_MODE", "2"));
  return kv_map;
}

SaiMinipack3NPlatform::~SaiMinipack3NPlatform() = default;
} // namespace facebook::fboss
