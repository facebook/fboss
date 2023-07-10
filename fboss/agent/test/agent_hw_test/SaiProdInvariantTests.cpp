#include "fboss/agent/hw/sai/switch/gen-cpp2/SaiCtrl.h"
#include "fboss/agent/platforms/sai/SaiPlatformInit.h"
#include "fboss/agent/test/agent_hw_test/ProdInvariantTests.h"

#include "fboss/agent/SetupThrift.h"

namespace facebook::fboss {
void verifyHwSwitchHandler() {
  std::shared_ptr<folly::ScopedEventBaseThread> evbThread;
  auto client = setupClient<apache::thrift::Client<SaiCtrl>>("sai", evbThread);
  auto state = client->sync_getHwSwitchRunState();
  EXPECT_GE(state, SwitchRunState::CONFIGURED);
}
} // namespace facebook::fboss
int main(int argc, char* argv[]) {
  return facebook::fboss::ProdInvariantTestMain(
      argc, argv, facebook::fboss::initSaiPlatform);
}
