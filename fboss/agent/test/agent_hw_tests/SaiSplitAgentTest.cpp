// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/agent/platforms/sai/SaiPlatformInit.h"
#include "fboss/agent/test/AgentEnsemble.h"

#include <folly/logging/Init.h>
#include <gtest/gtest.h>

FOLLY_INIT_LOGGING_CONFIG("fboss=DBG4; default:async=true");

int main(int argc, char* argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return facebook::fboss::ensembleMain(
      argc, argv, facebook::fboss::initSaiPlatform);
}
