// Copyright 2004-present Facebook. All Rights Reserved.
#include <fboss/agent/FbossError.h>
#include <fboss/agent/test/TestUtils.h>
#include "fboss/agent/hw/bcm/tests/BcmTest.h"
#include "fboss/agent/hw/bcm/tests/BcmTestUtils.h"
#include "fboss/agent/test/LabelForwardingUtils.h"

#include "fboss/agent/hw/test/ConfigFactory.h"

namespace facebook::fboss {

class BcmLabelForwardingTests : public BcmTest {
 public:
  void SetUp() override {
    BcmTest::SetUp();
    applyNewConfig(initialConfig());
  }

 protected:
  cfg::SwitchConfig initialConfig() const override {
    return utility::oneL3IntfConfig(getHwSwitch(), masterLogicalPortIds()[0]);
  }
  LabelNextHop getLabelNextHop(
      folly::IPAddress nexthop,
      LabelForwardingAction forwardingAction) {
    return ResolvedNextHop(
        nexthop, InterfaceID(1), NextHopWeight(0), std::move(forwardingAction));
  }
  void programLabel(ClientID client, Label label, LabelNextHopSet nhops) {
    auto updater = getRouteUpdater();
    MplsRoute route;
    route.topLabel() = label.value();
    route.nextHops() = util::fromRouteNextHopSet(nhops);
    updater->addRoute(client, std::move(route));
    updater->program();
  }
};

TEST_F(BcmLabelForwardingTests, ValidLabelFIBDelta) {
  LabelNextHopSet nexthops;
  nexthops.insert(getLabelNextHop(
      folly::IPAddressV4("1.1.1.1"),
      LabelForwardingAction(
          LabelForwardingAction::LabelForwardingType::SWAP, 1001)));
  nexthops.insert(getLabelNextHop(
      folly::IPAddressV4("1.1.1.1"),
      LabelForwardingAction(
          LabelForwardingAction::LabelForwardingType::SWAP, 1002)));
  nexthops.insert(getLabelNextHop(
      folly::IPAddressV4("1.1.1.1"),
      LabelForwardingAction(
          LabelForwardingAction::LabelForwardingType::SWAP, 1003)));
  nexthops.insert(getLabelNextHop(
      folly::IPAddressV4("1.1.1.1"),
      LabelForwardingAction(
          LabelForwardingAction::LabelForwardingType::SWAP, 1004)));

  auto oldState = getProgrammedState();
  programLabel(ClientID::OPENR, 1000, nexthops);
  auto newState = getProgrammedState();
  EXPECT_NE(oldState, newState);
  EXPECT_EQ(
      this->getHwSwitch()->isValidStateUpdate(StateDelta(oldState, newState)),
      true);
}

TEST_F(BcmLabelForwardingTests, InvalidLabelFIBDelta) {
  LabelNextHopSet nexthops;
  nexthops.insert(getLabelNextHop(
      folly::IPAddressV4("1.1.1.1"),
      LabelForwardingAction(
          LabelForwardingAction::LabelForwardingType::SWAP, 1001)));
  nexthops.insert(getLabelNextHop(
      folly::IPAddressV4("1.1.1.1"),
      LabelForwardingAction(
          LabelForwardingAction::LabelForwardingType::SWAP, 1002)));
  nexthops.insert(getLabelNextHop(
      folly::IPAddressV4("1.1.1.1"),
      LabelForwardingAction(
          LabelForwardingAction::LabelForwardingType::SWAP, 1003)));
  nexthops.insert(getLabelNextHop(
      folly::IPAddressV4("1.1.1.1"),
      LabelForwardingAction(
          LabelForwardingAction::LabelForwardingType::PUSH,
          LabelForwardingAction::LabelStack{1001, 1002})));

  auto oldState = getProgrammedState();
  EXPECT_THROW(programLabel(ClientID::OPENR, 1000, nexthops), FbossError);
  auto newState = getProgrammedState();
  EXPECT_EQ(oldState, newState);
}

TEST_F(BcmLabelForwardingTests, ValidPushStack) {
  LabelForwardingAction::LabelStack stack;
  for (auto i = 0; i < getPlatform()->getAsic()->getMaxLabelStackDepth(); i++) {
    stack.push_back(1001 + i);
  }

  LabelNextHopSet nexthops;
  nexthops.emplace(getLabelNextHop(
      folly::IPAddressV4("1.1.1.1"),
      LabelForwardingAction(
          LabelForwardingAction::LabelForwardingType::PUSH, std::move(stack))));

  auto oldState = getProgrammedState();
  programLabel(ClientID::OPENR, 1000, nexthops);
  auto newState = getProgrammedState();
  EXPECT_NE(oldState, newState);
  EXPECT_EQ(
      this->getHwSwitch()->isValidStateUpdate(StateDelta(oldState, newState)),
      true);
}

TEST_F(BcmLabelForwardingTests, InvalidPushStack) {
  LabelForwardingAction::LabelStack stack;
  for (auto i = 0; i < getPlatform()->getAsic()->getMaxLabelStackDepth() + 1;
       i++) {
    stack.push_back(1001 + i);
  }

  LabelNextHopSet nexthops;
  nexthops.emplace(getLabelNextHop(
      folly::IPAddressV4("1.1.1.1"),
      LabelForwardingAction(
          LabelForwardingAction::LabelForwardingType::PUSH, std::move(stack))));

  auto oldState = getProgrammedState();
  EXPECT_THROW(programLabel(ClientID::OPENR, 1000, nexthops), FbossError);
  auto newState = getProgrammedState();
  EXPECT_EQ(oldState, newState);
}

} // namespace facebook::fboss
