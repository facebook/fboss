// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/agent/platforms/wedge/WedgePlatformInit.h"
#include "fboss/agent/test/AgentHwTest.h"

#include <folly/logging/Init.h>
#include <gtest/gtest.h>

FOLLY_INIT_LOGGING_CONFIG("fboss=DBG4; default:async=true");

int main(int argc, char* argv[]) {
  testing::InitGoogleTest(&argc, argv);
  facebook::fboss::initAgentHwTest(
      argc, argv, facebook::fboss::initWedgePlatform);
  return RUN_ALL_TESTS();
}
