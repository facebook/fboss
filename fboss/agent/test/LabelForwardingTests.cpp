// Copyright 2004-present Facebook. All Rights Reserved.

#include <gtest/gtest.h>

#include "fboss/agent/ThriftHandler.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/HwTestHandle.h"
#include "fboss/agent/test/LabelForwardingUtils.h"
#include "fboss/agent/test/TestUtils.h"

using namespace ::testing;
namespace facebook {
namespace fboss {
class LabelForwardingTest : public ::testing::Test {
 public:
  void SetUp() override {
    // Setup a default state object
    auto state = testStateA();
    handle = createTestHandle(state);
    sw = handle->getSw();
    thriftHandler = std::make_unique<ThriftHandler>(sw);
    sw->initialConfigApplied(std::chrono::steady_clock::now());
    waitForStateUpdates(sw);
  }

  void TearDown() override {
    waitForStateUpdates(sw);
  }
  std::unique_ptr<ThriftHandler> thriftHandler{nullptr};
  SwSwitch* sw{nullptr};
  std::unique_ptr<HwTestHandle> handle{nullptr};
};

TEST_F(LabelForwardingTest, addMplsRoutes) {
  std::array<ClientID, 2> clients{
      StdClientIds2ClientID(StdClientIds::OPENR),
      StdClientIds2ClientID(StdClientIds::BGPD),
  };
  std::array<std::vector<MplsRoute>, 2> routes{
      util::getTestRoutes(0, 4),
      util::getTestRoutes(4, 4),
  };

  for (auto i = 0; i < 2; i++) {
    thriftHandler->addMplsRoutes(
        clients[i], std::make_unique<std::vector<MplsRoute>>(routes[i]));
  }

  waitForStateUpdates(sw);
  auto labelFib = sw->getState()->getLabelForwardingInformationBase();

  for (auto i = 0; i < 2; i++) {
    for (const auto& route : routes[i]) {
      const auto& labelFibEntry =
          labelFib->getLabelForwardingEntry(route.topLabel);
      const auto* labelFibEntryForClient =
          labelFibEntry->getEntryForClient(clients[i]);

      EXPECT_NE(nullptr, labelFibEntryForClient);
      EXPECT_EQ(
          util::toRouteNextHopSet(route.nextHops),
          labelFibEntryForClient->getNextHopSet());
    }
  }
}

} // namespace fboss
} // namespace facebook
