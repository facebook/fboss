/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/common/ladakh800bcls/Ladakh800bclsPlatformMapping.h"
#include <folly/logging/xlog.h>
#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/platforms/common/ladakh800bcls/Ladakh800bclsProdPlatformMapping.h"
#include "fboss/agent/platforms/common/ladakh800bcls/Ladakh800bclsTestPlatformMapping.h"

namespace facebook::fboss {
namespace {
static const std::string getPlatformMappingStr() {
  if (FLAGS_test_fixture) {
    XLOG(INFO) << "Using Test Fixture Platform Mapping";
    return kJsonTestPlatformMappingStr;
  }
  XLOG(INFO) << "Using Prod Platform Mapping";
  return kJsonProdPlatformMappingStr;
}
} // namespace

Ladakh800bclsPlatformMapping::Ladakh800bclsPlatformMapping()
    : PlatformMapping(getPlatformMappingStr()) {}

Ladakh800bclsPlatformMapping::Ladakh800bclsPlatformMapping(
    const std::string& platformMappingStr)
    : PlatformMapping(platformMappingStr) {}

} // namespace facebook::fboss
