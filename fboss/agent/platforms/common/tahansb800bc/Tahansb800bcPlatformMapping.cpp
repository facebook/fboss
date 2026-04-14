/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/common/tahansb800bc/Tahansb800bcPlatformMapping.h"
#include <folly/logging/xlog.h>
#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/platforms/common/tahansb800bc/Tahansb800bcLinkTrainingPlatformMapping.h"
#include "fboss/agent/platforms/common/tahansb800bc/Tahansb800bcProdPlatformMapping.h"
#include "fboss/agent/platforms/common/tahansb800bc/Tahansb800bcTestPlatformMapping.h"

namespace facebook::fboss {
namespace {
static const std::string getPlatformMappingStr() {
  if (FLAGS_test_fixture) {
    XLOG(INFO) << "Using Test Fixture Platform Mapping";
    return kJsonTestPlatformMappingStr;
  }
  if (FLAGS_tahan800sb_link_training) {
    XLOG(INFO) << "Using Link Training Platform Mapping";
    return kJsonLinkTrainingPlatformMappingStr;
  }
  XLOG(INFO) << "Using Prod Platform Mapping";
  return kJsonProdPlatformMappingStr;
}
} // namespace

Tahansb800bcPlatformMapping::Tahansb800bcPlatformMapping()
    : PlatformMapping(getPlatformMappingStr()) {}

Tahansb800bcPlatformMapping::Tahansb800bcPlatformMapping(
    const std::string& platformMappingStr)
    : PlatformMapping(platformMappingStr) {}

} // namespace facebook::fboss
