// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/agent/platforms/wedge/WedgePlatformInit.h"
#include "fboss/agent/test/link_tests/AgentEnsembleLinkTest.h"

int main(int argc, char* argv[]) {
  return facebook::fboss::agentEnsembleLinkTestMain(
      argc, argv, facebook::fboss::initWedgePlatform);
}
