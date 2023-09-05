// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/agent/platforms/sai/SaiPlatformInit.h"
#include "fboss/agent/test/AgentEnsemble.h"

int main(int argc, char* argv[]) {
  return facebook::fboss::ensembleMain(
      argc, argv, facebook::fboss::initSaiPlatform);
}
