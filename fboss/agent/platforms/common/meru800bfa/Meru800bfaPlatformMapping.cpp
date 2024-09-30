/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/common/meru800bfa/Meru800bfaPlatformMapping.h"
#include <folly/logging/xlog.h>
#include "fboss/agent/FbossError.h"
#include "fboss/agent/platforms/common/meru800bfa/Meru800bfaP2PlatformMapping.h"
#include "fboss/agent/platforms/common/meru800bfa/Meru800bfaProdPlatformMapping.h"
#include "fboss/lib/platforms/PlatformProductInfo.h"

namespace facebook::fboss {
namespace {
static const std::string getPlatformMappingStr(bool multiNpuPlatformMapping) {
  auto productInfo =
      std::make_unique<PlatformProductInfo>(FLAGS_fruid_filepath);
  try {
    productInfo->initialize();
  } catch (const std::exception& ex) {
    // Expected when fruid file is not of a switch (eg: on devservers)
    XLOG(INFO) << "Couldn't initialize platform mapping " << ex.what();
  }

  auto productVersion = productInfo->getProductVersion();
  XLOG(INFO) << "Product version: " << productVersion;
  if (productVersion < 4 && multiNpuPlatformMapping) {
    XLOG(INFO) << "Using P2 MNPU Platform Mapping";
    return kJsonMultiNpuP2PlatformMappingStr;
  } else if (productVersion >= 4 && multiNpuPlatformMapping) {
    XLOG(INFO) << "Using Prod MNPU Platform Mapping";
    return kJsonMultiNpuProdPlatformMappingStr;
  } else if (productVersion < 4) {
    XLOG(INFO) << "Using P2 Single NPU Platform Mapping";
    return kJsonSingleNpuP2PlatformMappingStr;
  } else if (productVersion >= 4) {
    XLOG(INFO) << "Using Prod Single NPU Platform Mapping";
    return kJsonSingleNpuProdPlatformMappingStr;
  } else {
    throw FbossError(
        "No platform mapping found for product version ", productVersion);
  }
}
} // namespace

Meru800bfaPlatformMapping::Meru800bfaPlatformMapping()
    : PlatformMapping(getPlatformMappingStr(FLAGS_multi_npu_platform_mapping)) {
}

Meru800bfaPlatformMapping::Meru800bfaPlatformMapping(
    const std::string& platformMappingStr)
    : PlatformMapping(platformMappingStr) {}

Meru800bfaPlatformMapping::Meru800bfaPlatformMapping(
    bool multiNpuPlatformMapping)
    : PlatformMapping(getPlatformMappingStr(multiNpuPlatformMapping)) {}

} // namespace facebook::fboss
