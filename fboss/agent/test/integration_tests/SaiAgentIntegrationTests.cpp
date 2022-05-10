// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/agent/platforms/sai/SaiPlatformInit.h"
#include "fboss/agent/test/integration_tests/AgentIntegrationTest.h"

int main(int argc, char* argv[]) {
  return facebook::fboss::agentIntegrationTestMain(
      argc, argv, facebook::fboss::initSaiPlatform);
}
