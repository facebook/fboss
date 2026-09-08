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
#include "fboss/agent/platforms/common/ladakh800bcls/Ladakh800bclsOsfpTrayPlatformMapping.h"
#include "fboss/agent/platforms/common/ladakh800bcls/Ladakh800bclsProdPlatformMapping.h"
#include "fboss/agent/platforms/common/ladakh800bcls/Ladakh800bclsProdPostEvtPlatformMapping.h"
#include "fboss/agent/platforms/common/ladakh800bcls/Ladakh800bclsTestPlatformMapping.h"
#include "fboss/agent/platforms/common/ladakh800bcls/Ladakh800bclsTestPostEvtPlatformMapping.h"
#include "fboss/lib/platforms/PlatformProductInfo.h"

namespace facebook::fboss {
namespace {
// EVT boards report a fruid Production State of 1. Anything higher is DVT or
// later. Boards whose fruid cannot be read fall back to EVT.
bool isPostEvt(std::optional<int> productionState) {
  if (!productionState) {
    auto productInfo =
        std::make_unique<PlatformProductInfo>(FLAGS_fruid_filepath);
    try {
      productInfo->initialize();
    } catch (const std::exception& ex) {
      // Expected when fruid file is not of a switch (eg: on devservers).
      // Not fatal: initialize() throws from initMode(), which runs after the
      // fruid data has already been parsed, so the value may still be good.
      XLOG(INFO) << "Couldn't fully initialize product info: " << ex.what();
    }
    productionState = productInfo->getProductionState();
  }
  XLOG(INFO) << "Production state: " << *productionState;
  return *productionState > 1;
}

static const std::string getPlatformMappingStr(
    std::optional<int> productionState) {
  if (FLAGS_osfp_tray) {
    XLOG(INFO) << "Using OSFP Tray Platform Mapping";
    return kJsonOsfpTrayPlatformMappingStr;
  }
  const bool postEvt = isPostEvt(productionState);
  if (FLAGS_test_fixture) {
    if (postEvt) {
      XLOG(INFO) << "Using Post-EVT Test Fixture Platform Mapping";
      return kJsonTestPostEvtPlatformMappingStr;
    }
    XLOG(INFO) << "Using Test Fixture Platform Mapping";
    return kJsonTestPlatformMappingStr;
  }
  if (postEvt) {
    XLOG(INFO) << "Using Post-EVT Prod Platform Mapping";
    return kJsonProdPostEvtPlatformMappingStr;
  }
  XLOG(INFO) << "Using Prod Platform Mapping";
  return kJsonProdPlatformMappingStr;
}
} // namespace

Ladakh800bclsPlatformMapping::Ladakh800bclsPlatformMapping()
    : PlatformMapping(getPlatformMappingStr(std::nullopt)) {}

Ladakh800bclsPlatformMapping::Ladakh800bclsPlatformMapping(
    const std::string& platformMappingStr)
    : PlatformMapping(platformMappingStr) {}

Ladakh800bclsPlatformMapping::Ladakh800bclsPlatformMapping(
    std::optional<int> productionState)
    : PlatformMapping(getPlatformMappingStr(productionState)) {}

} // namespace facebook::fboss
