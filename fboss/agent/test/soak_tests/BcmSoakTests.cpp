// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/agent/platforms/wedge/WedgePlatformInit.h"
#include "fboss/agent/test/soak_tests/SoakTest.h"

int main(int argc, char* argv[]) {
  return facebook::fboss::soakTestMain(
      argc, argv, facebook::fboss::initWedgePlatform);
}
