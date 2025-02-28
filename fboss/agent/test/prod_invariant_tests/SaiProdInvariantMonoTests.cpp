#include "fboss/agent/hw/sai/switch/gen-cpp2/SaiCtrl.h"
#include "fboss/agent/platforms/sai/SaiPlatformInit.h"
#include "fboss/agent/test/prod_agent_tests/ProdInvariantTests.h"

#include "fboss/agent/SetupThrift.h"

#include <folly/logging/Init.h>

FOLLY_INIT_LOGGING_CONFIG("DBG2; default:async=true");

namespace facebook::fboss {
void verifyHwSwitchHandler() {
  std::shared_ptr<folly::ScopedEventBaseThread> evbThread;
  auto client = setupClient<apache::thrift::Client<SaiCtrl>>("sai", evbThread);
  auto state = client->sync_getHwSwitchRunState();
  if (FLAGS_multi_switch) {
    // Expect uninitialized HwAgent since config testing is only run in mono
    // mode
    EXPECT_GE(state, SwitchRunState::UNINITIALIZED);
  } else {
    EXPECT_GE(state, SwitchRunState::CONFIGURED);
  }
}
} // namespace facebook::fboss
int main(int argc, char* argv[]) {
  return facebook::fboss::ProdInvariantTestMain(
      argc, argv, facebook::fboss::initSaiPlatform);
}
