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
    ON_CALL(*getMockHw(sw), isValidStateUpdate(_))
        .WillByDefault(testing::Return(true));
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
      ClientID::OPENR,
      ClientID::BGPD,
  };
  std::array<std::vector<MplsRoute>, 2> routes{
      util::getTestRoutes(0, 4),
      util::getTestRoutes(4, 4),
  };

  for (auto i = 0; i < 2; i++) {
    thriftHandler->addMplsRoutes(
        static_cast<int>(clients[i]),
        std::make_unique<std::vector<MplsRoute>>(routes[i]));
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

TEST_F(LabelForwardingTest, deleteMplsRoutes) {
  std::array<ClientID, 2> clients{
      ClientID::OPENR,
      ClientID::BGPD,
  };
  std::array<std::vector<MplsRoute>, 2> routes{
      util::getTestRoutes(0, 4),
      util::getTestRoutes(4, 4),
  };

  for (auto i = 0; i < 2; i++) {
    thriftHandler->addMplsRoutes(
        static_cast<int>(clients[i]),
        std::make_unique<std::vector<MplsRoute>>(routes[i]));
  }

  waitForStateUpdates(sw);

  std::array<std::vector<MplsLabel>, 2> routesToRemove;
  std::array<std::vector<MplsRoute>, 2> routesToRetain;

  for (auto i = 0; i < 2; i++) {
    for (const auto& route : routes[i]) {
      if (route.topLabel % 2) {
        routesToRemove[i].push_back(route.topLabel);
      }
    }
  }
  for (auto i = 0; i < 2; i++) {
    thriftHandler->deleteMplsRoutes(
        static_cast<int>(clients[i]),
        std::make_unique<std::vector<MplsLabel>>(routesToRemove[i]));
  }

  auto labelFib = sw->getState()->getLabelForwardingInformationBase();

  for (auto i = 0; i < 2; i++) {
    for (const auto& label : routesToRemove[i]) {
      const auto labelFibEntry = labelFib->getLabelForwardingEntryIf(label);
      EXPECT_EQ(nullptr, labelFibEntry);
    }
    for (const auto& route : routesToRetain[i]) {
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

TEST_F(LabelForwardingTest, syncMplsFib) {
  std::array<ClientID, 2> clients{
      ClientID::OPENR,
      ClientID::BGPD,
  };
  std::array<std::vector<MplsRoute>, 2> routes{
      util::getTestRoutes(0, 4),
      util::getTestRoutes(4, 4),
  };

  for (auto i = 0; i < 2; i++) {
    thriftHandler->addMplsRoutes(
        static_cast<int>(clients[i]),
        std::make_unique<std::vector<MplsRoute>>(routes[i]));
  }

  waitForStateUpdates(sw);

  auto moreOpenrRoutes = util::getTestRoutes(8, 4);
  thriftHandler->addMplsRoutes(
      static_cast<int>(clients[0]),
      std::make_unique<std::vector<MplsRoute>>(moreOpenrRoutes));

  waitForStateUpdates(sw);

  routes[0].insert(
      std::end(routes[0]),
      std::begin(moreOpenrRoutes),
      std::end(moreOpenrRoutes));

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

  thriftHandler->syncMplsFib(
      static_cast<int>(clients[0]),
      std::make_unique<std::vector<MplsRoute>>(
          std::begin(routes[0]), std::begin(routes[0]) + 4));

  waitForStateUpdates(sw);

  labelFib = sw->getState()->getLabelForwardingInformationBase();

  for (auto i = 0; i < 8; i++) {
    if (i < 4) {
      const auto& labelFibEntry =
          labelFib->getLabelForwardingEntry(routes[0][i].topLabel);
      const auto* labelFibEntryForClient =
          labelFibEntry->getEntryForClient(clients[0]);

      EXPECT_NE(nullptr, labelFibEntryForClient);
      EXPECT_EQ(
          util::toRouteNextHopSet(routes[0][i].nextHops),
          labelFibEntryForClient->getNextHopSet());
    } else {
      auto labelFibEntry =
          labelFib->getLabelForwardingEntryIf(routes[0][i].topLabel);
      EXPECT_EQ(nullptr, labelFibEntry);
    }
  }
}

TEST_F(LabelForwardingTest, getMplsRouteTableByClient) {
  std::array<ClientID, 2> clients{
      ClientID::OPENR,
      ClientID::BGPD,
  };
  std::array<std::vector<MplsRoute>, 2> inRoutes{
      util::getTestRoutes(0, 4),
      util::getTestRoutes(4, 4),
  };

  auto sortByLabel = [](const MplsRoute& route1, const MplsRoute& route2) {
    return route1.topLabel < route2.topLabel;
  };

  for (auto i = 0; i < 2; i++) {
    std::sort(inRoutes[i].begin(), inRoutes[i].end(), sortByLabel);

    thriftHandler->addMplsRoutes(
        static_cast<int>(clients[i]),
        std::make_unique<std::vector<MplsRoute>>(inRoutes[i]));
  }

  std::array<std::vector<MplsRoute>, 2> outRoutes;

  for (auto i = 0; i < 2; i++) {
    thriftHandler->getMplsRouteTableByClient(
        outRoutes[i], static_cast<int>(clients[i]));
    std::sort(outRoutes[i].begin(), outRoutes[i].end(), sortByLabel);
  }

  for (auto i = 0; i < 2; i++) {
    auto in = inRoutes[i].begin();
    auto out = outRoutes[i].begin();
    for (; in != inRoutes[i].end() && out != outRoutes[i].end(); in++, out++) {
      EXPECT_EQ(in->topLabel, out->topLabel);
      EXPECT_EQ(
          util::toRouteNextHopSet(in->nextHops),
          util::toRouteNextHopSet(out->nextHops));
      EXPECT_EQ(
          in->adminDistance_ref().value_unchecked(),
          out->adminDistance_ref().value_unchecked());
    }
  }
}
} // namespace fboss
} // namespace facebook
