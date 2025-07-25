/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/common/janga800bic/Janga800bicPlatformMapping.h"
#include <folly/logging/xlog.h>
#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/platforms/common/janga800bic/Janga800bicProdPlatformMapping.h"
#include "fboss/agent/platforms/common/janga800bic/Janga800bicTestPlatformMapping.h"

namespace facebook::fboss {
namespace {
static const std::string getPlatformMappingStr(bool multiNpuPlatformMapping) {
  if (multiNpuPlatformMapping) {
    if (FLAGS_janga_test) {
      XLOG(INFO) << "Using Test MNPU Platform Mapping";
      return kJsonMultiNpuTestPlatformMappingStr;
    }
    XLOG(INFO) << "Using Prod MNPU Platform Mapping";
    return kJsonMultiNpuProdPlatformMappingStr;
  } else {
    if (FLAGS_janga_test) {
      XLOG(INFO) << "Using Test Single NPU Platform Mapping";
      return kJsonSingleNpuTestPlatformMappingStr;
    }
    XLOG(INFO) << "Using Prod Single NPU Platform Mapping";
    return kJsonSingleNpuProdPlatformMappingStr;
  }
}
} // namespace

Janga800bicPlatformMapping::Janga800bicPlatformMapping()
    : PlatformMapping(getPlatformMappingStr(FLAGS_multi_npu_platform_mapping)) {
}

Janga800bicPlatformMapping::Janga800bicPlatformMapping(
    const std::string& platformMappingStr)
    : PlatformMapping(platformMappingStr) {}

Janga800bicPlatformMapping::Janga800bicPlatformMapping(
    bool multiNpuPlatformMapping)
    : PlatformMapping(getPlatformMappingStr(multiNpuPlatformMapping)) {}

} // namespace facebook::fboss
