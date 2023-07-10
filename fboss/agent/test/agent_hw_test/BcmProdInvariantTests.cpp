#include "fboss/agent/hw/bcm/gen-cpp2/BcmCtrl2.h"
#include "fboss/agent/platforms/wedge/WedgePlatformInit.h"
#include "fboss/agent/test/agent_hw_test/ProdInvariantTests.h"

#include "fboss/agent/SetupThrift.h"

namespace facebook::fboss {
void verifyHwSwitchHandler() {
  std::shared_ptr<folly::ScopedEventBaseThread> evbThread;
  auto client = setupClient<apache::thrift::Client<BcmCtrl2>>("bcm", evbThread);
  auto out = client->sync_echoI32(10);
  EXPECT_EQ(out, 10);
}
} // namespace facebook::fboss
int main(int argc, char* argv[]) {
  return facebook::fboss::ProdInvariantTestMain(
      argc, argv, facebook::fboss::initWedgePlatform);
}
