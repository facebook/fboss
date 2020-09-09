// Copyright 2004-present Facebook. All Rights Reserved.

#include <gtest/gtest.h>

#include "fboss/agent/ThriftHandler.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/HwTestHandle.h"
#include "fboss/agent/test/LabelForwardingUtils.h"
#include "fboss/agent/test/TestUtils.h"

using namespace ::testing;

namespace facebook::fboss {

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

  std::vector<folly::IPAddress> getDirectlyConnectedNexthops() {
    return {
        folly::IPAddress("10.0.0.101"), // intfID 1
        folly::IPAddress("2401:db00:2110:3001::0101"), // intfID 1
        folly::IPAddress("10.0.55.101"), // intfID 55
        folly::IPAddress("2401:db00:2110:3055::0101"), // intfID 55
    };
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

  sw->fibSynced();

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
          util::toRouteNextHopSet(*route.nextHops_ref()),
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

  sw->fibSynced();

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
          util::toRouteNextHopSet(*route.nextHops_ref()),
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

  sw->fibSynced();

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
          util::toRouteNextHopSet(*route.nextHops_ref()),
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
          util::toRouteNextHopSet(*routes[0][i].nextHops_ref()),
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

  sw->fibSynced();

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
          util::toRouteNextHopSet(*in->nextHops_ref()),
          util::toRouteNextHopSet(*out->nextHops_ref()));
      EXPECT_EQ(
          in->adminDistance_ref().value_or({}),
          out->adminDistance_ref().value_or({}));
    }
  }
}

TEST_F(LabelForwardingTest, unresolvedNextHops) {
  std::vector<MplsRoute> mplsRoutes;
  std::vector<MplsActionCode> actions{
      MplsActionCode::SWAP,
      MplsActionCode::PHP,
      MplsActionCode::PUSH,
  };
  std::vector<MplsLabel> labels{9001, 9002, 9003};
  mplsRoutes.resize(3);
  for (auto i = 0; i < 3; i++) {
    mplsRoutes[i].topLabel = labels[i];
    auto ips = getDirectlyConnectedNexthops();
    for (auto ip : ips) {
      NextHopThrift nexthop;
      if (ip.isV4()) {
        nexthop.address_ref()->addr.append(
            reinterpret_cast<const char*>(ip.bytes()),
            folly::IPAddressV4::byteCount());
      } else {
        nexthop.address_ref()->addr.append(
            reinterpret_cast<const char*>(ip.bytes()),
            folly::IPAddressV6::byteCount());
      }
      nexthop.mplsAction_ref() = MplsAction();
      *nexthop.mplsAction_ref()->action_ref() = actions[i];
      if (actions[i] == MplsActionCode::SWAP) {
        nexthop.mplsAction_ref()->swapLabel_ref() = 9781;
      } else if (actions[i] == MplsActionCode::PUSH) {
        nexthop.mplsAction_ref()->pushLabels_ref() = MplsLabelStack();
        nexthop.mplsAction_ref()->pushLabels_ref()->push_back(9781);
      }
      mplsRoutes[i].nextHops_ref()->emplace_back(nexthop);
    }
  }

  sw->fibSynced();
  thriftHandler->addMplsRoutes(
      static_cast<int>(ClientID::OPENR),
      std::make_unique<std::vector<MplsRoute>>(mplsRoutes));

  waitForStateUpdates(sw);
  auto labelFib = sw->getState()->getLabelForwardingInformationBase();

  for (auto i = 0; i < 3; i++) {
    const auto& labelFibEntry = labelFib->getLabelForwardingEntry(labels[i]);
    const auto* labelFibEntryForClient =
        labelFibEntry->getEntryForClient(ClientID::OPENR);

    EXPECT_NE(nullptr, labelFibEntryForClient);
    auto nexthops = labelFibEntryForClient->getNextHopSet();
    EXPECT_EQ(nexthops.size(), 4);
    for (auto nexthop : nexthops) {
      // no unresolved next hops , all are resolved
      EXPECT_TRUE(nexthop.isResolved());
      EXPECT_TRUE(
          nexthop.intf() == InterfaceID(1) ||
          nexthop.intf() == InterfaceID(55));
      auto addr = nexthop.addr();
      if (nexthop.intf() == InterfaceID(1)) {
        if (addr.isV4()) {
          EXPECT_EQ(addr, folly::IPAddress("10.0.0.101"));
        } else {
          EXPECT_EQ(addr, folly::IPAddress("2401:db00:2110:3001::0101"));
        }
      } else if (nexthop.intf() == InterfaceID(55)) {
        if (addr.isV4()) {
          EXPECT_EQ(addr, folly::IPAddress("10.0.55.101"));
        } else {
          EXPECT_EQ(addr, folly::IPAddress("2401:db00:2110:3055::0101"));
        }
      }
      auto action = nexthop.labelForwardingAction();
      EXPECT_EQ(action->type(), actions[i]);
      if (action->type() == MplsActionCode::SWAP) {
        EXPECT_EQ(action->swapWith().value(), 9781);
      } else if (action->type() == MplsActionCode::PUSH) {
        EXPECT_EQ(action->pushStack().value(), std::vector<MplsLabel>{9781});
      }
    }
  }
}

TEST_F(LabelForwardingTest, invalidUnresolvedNextHops) {
  MplsRoute mplsRoute;
  mplsRoute.topLabel = 10010;
  std::vector<folly::IPAddress> ips{
      folly::IPAddress("10.0.0.101"),
      folly::IPAddress("10.0.55.101"),
      folly::IPAddress("101.101.101.1"), // invalid
  };
  for (auto i = 0; i < 3; i++) {
    NextHopThrift nexthop;

    nexthop.mplsAction_ref() = MplsAction();
    *nexthop.mplsAction_ref()->action_ref() = MplsActionCode::PHP;
    nexthop.address_ref()->addr.append(
        reinterpret_cast<const char*>(ips[i].bytes()),
        folly::IPAddressV4::byteCount());
    mplsRoute.nextHops_ref()->emplace_back(nexthop);
  }

  sw->fibSynced();
  auto routes = std::make_unique<std::vector<MplsRoute>>();
  routes->push_back(mplsRoute);
  EXPECT_THROW(
      thriftHandler->addMplsRoutes(
          static_cast<int>(ClientID::OPENR), std::move(routes)),
      FbossError);
}

TEST_F(LabelForwardingTest, nextHopWithInterfaceAddress) {
  MplsRoute mplsRoute0;
  mplsRoute0.topLabel = 10010;
  std::vector<folly::IPAddress> ips0{
      folly::IPAddress("10.0.0.101"),
      folly::IPAddress("10.0.55.101"),
      folly::IPAddress("10.0.0.1"), // invalid, self address
  };
  for (auto i = 0; i < 3; i++) {
    NextHopThrift nexthop;

    nexthop.mplsAction_ref() = MplsAction();
    *nexthop.mplsAction_ref()->action_ref() = MplsActionCode::PHP;
    nexthop.address_ref()->addr.append(
        reinterpret_cast<const char*>(ips0[i].bytes()),
        folly::IPAddressV4::byteCount());
    mplsRoute0.nextHops_ref()->emplace_back(nexthop);
  }

  sw->fibSynced();
  auto routes0 = std::make_unique<std::vector<MplsRoute>>();
  routes0->push_back(mplsRoute0);
  EXPECT_THROW(
      thriftHandler->addMplsRoutes(
          static_cast<int>(ClientID::OPENR), std::move(routes0)),
      FbossError);

  MplsRoute mplsRoute1;
  mplsRoute1.topLabel = 10010;
  std::vector<folly::IPAddress> ips1{
      folly::IPAddress("10.0.0.101"),
      folly::IPAddress("10.0.55.101"),
      folly::IPAddress("fe80:db00::101"), // invalid, link local address
  };
  for (auto i = 0; i < 3; i++) {
    NextHopThrift nexthop;

    nexthop.mplsAction_ref() = MplsAction();
    *nexthop.mplsAction_ref()->action_ref() = MplsActionCode::PHP;
    nexthop.address_ref()->addr.append(
        reinterpret_cast<const char*>(ips0[i].bytes()),
        folly::IPAddressV4::byteCount());
    mplsRoute1.nextHops_ref()->emplace_back(nexthop);
  }

  sw->fibSynced();
  auto routes1 = std::make_unique<std::vector<MplsRoute>>();
  routes1->push_back(mplsRoute1);
  EXPECT_THROW(
      thriftHandler->addMplsRoutes(
          static_cast<int>(ClientID::OPENR), std::move(routes1)),
      FbossError);
}

TEST_F(LabelForwardingTest, popAndLookUp) {
  MplsRoute mplsRoute0;
  mplsRoute0.topLabel = 10010;
  folly::IPAddress ips0{"::"};
  NextHopThrift nexthop;

  nexthop.mplsAction_ref() = MplsAction();
  *nexthop.mplsAction_ref()->action_ref() = MplsActionCode::POP_AND_LOOKUP;
  nexthop.address_ref()->addr.append(
      reinterpret_cast<const char*>(ips0.bytes()),
      folly::IPAddressV6::byteCount());

  mplsRoute0.nextHops_ref()->emplace_back(nexthop);
  sw->fibSynced();
  sw->fibSynced();
  auto routes0 = std::make_unique<std::vector<MplsRoute>>();
  routes0->push_back(mplsRoute0);
  EXPECT_NO_THROW(thriftHandler->addMplsRoutes(
      static_cast<int>(ClientID::OPENR), std::move(routes0)));
}

TEST_F(LabelForwardingTest, popAndLookUpInvalid) {
  MplsRoute mplsRoute0;
  mplsRoute0.topLabel = 10010;

  folly::IPAddress ips0{"::"};
  NextHopThrift nexthop0;
  nexthop0.mplsAction_ref() = MplsAction();
  *nexthop0.mplsAction_ref()->action_ref() = MplsActionCode::POP_AND_LOOKUP;
  nexthop0.address_ref()->addr.append(
      reinterpret_cast<const char*>(ips0.bytes()),
      folly::IPAddressV6::byteCount());
  mplsRoute0.nextHops_ref()->emplace_back(nexthop0);

  folly::IPAddress ips1{"::1"};
  NextHopThrift nexthop1;
  nexthop1.mplsAction_ref() = MplsAction();
  *nexthop1.mplsAction_ref()->action_ref() = MplsActionCode::POP_AND_LOOKUP;
  nexthop1.address_ref()->addr.append(
      reinterpret_cast<const char*>(ips1.bytes()),
      folly::IPAddressV6::byteCount());
  mplsRoute0.nextHops_ref()->emplace_back(nexthop1);

  sw->fibSynced();
  auto routes0 = std::make_unique<std::vector<MplsRoute>>();
  routes0->push_back(mplsRoute0);
  EXPECT_THROW(
      thriftHandler->addMplsRoutes(
          static_cast<int>(ClientID::OPENR), std::move(routes0)),
      FbossError);
}

} // namespace facebook::fboss
