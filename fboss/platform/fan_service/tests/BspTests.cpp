/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/FbossError.h"
#include "fboss/platform/fan_service/Bsp.h"
#include "fboss/platform/fan_service/if/gen-cpp2/fan_service_config_types.h"

#include <gtest/gtest.h>

using namespace facebook::fboss::platform::fan_service;
using facebook::fboss::FbossError;

class BspTest : public ::testing::Test {};

TEST_F(BspTest, emergencyShutdownUndefinedCmdThrows) {
  auto config = FanServiceConfig{};
  config.shutdownCmd() = "NOT_DEFINED";
  auto bsp = Bsp(config);
  EXPECT_THROW(bsp.emergencyShutdown(true), FbossError);
}

TEST_F(BspTest, emergencyShutdownIdempotent) {
  auto config = FanServiceConfig{};
  config.shutdownCmd() = "NOT_DEFINED";
  auto bsp = Bsp(config);
  // First call with enable=false should be a no-op (no throw)
  EXPECT_NO_THROW(bsp.emergencyShutdown(false));
}
