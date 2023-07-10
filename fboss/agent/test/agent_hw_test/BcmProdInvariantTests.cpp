#include "fboss/agent/hw/bcm/gen-cpp2/BcmCtrl.h"
#include "fboss/agent/platforms/wedge/WedgePlatformInit.h"
#include "fboss/agent/test/agent_hw_test/ProdInvariantTests.h"

#include "fboss/agent/SetupThrift.h"

namespace facebook::fboss {
void verifyHwSwitchHandler() {
  std::shared_ptr<folly::ScopedEventBaseThread> evbThread;
  auto client = setupClient<apache::thrift::Client<BcmCtrl>>("bcm", evbThread);
  auto state = client->sync_getHwSwitchRunState();
  EXPECT_GE(state, SwitchRunState::CONFIGURED);
}
} // namespace facebook::fboss
int main(int argc, char* argv[]) {
  return facebook::fboss::ProdInvariantTestMain(
      argc, argv, facebook::fboss::initWedgePlatform);
}
