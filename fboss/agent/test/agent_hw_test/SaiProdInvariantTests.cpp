#include "fboss/agent/platforms/sai/SaiPlatformInit.h"
#include "fboss/agent/test/agent_hw_test/ProdInvariantTests.h"

int main(int argc, char* argv[]) {
  return facebook::fboss::ProdInvariantTestMain(
      argc, argv, facebook::fboss::initSaiPlatform);
}
