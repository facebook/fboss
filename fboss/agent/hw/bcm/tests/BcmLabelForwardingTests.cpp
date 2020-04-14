// Copyright 2004-present Facebook. All Rights Reserved.
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
};

TEST_F(BcmLabelForwardingTests, ValidLabelFIBDelta) {
  LabelNextHopSet nexthops;
  nexthops.insert(getLabelNextHop(
      folly::IPAddressV4("10.0.0.1"),
      LabelForwardingAction(
          LabelForwardingAction::LabelForwardingType::SWAP, 1001)));
  nexthops.insert(getLabelNextHop(
      folly::IPAddressV4("10.0.0.2"),
      LabelForwardingAction(
          LabelForwardingAction::LabelForwardingType::SWAP, 1002)));
  nexthops.insert(getLabelNextHop(
      folly::IPAddressV4("10.0.0.3"),
      LabelForwardingAction(
          LabelForwardingAction::LabelForwardingType::SWAP, 1003)));
  nexthops.insert(getLabelNextHop(
      folly::IPAddressV4("10.0.0.4"),
      LabelForwardingAction(
          LabelForwardingAction::LabelForwardingType::SWAP, 1004)));

  auto oldState = getProgrammedState();
  auto newState = oldState->clone();
  auto* labelFib =
      newState->getLabelForwardingInformationBase()->modify(&newState);

  labelFib->programLabel(
      &newState,
      1000,
      ClientID::OPENR,
      AdminDistance::DIRECTLY_CONNECTED,
      std::move(nexthops));

  EXPECT_EQ(labelFib->isPublished(), false);
  newState->publish();
  EXPECT_EQ(
      this->getHwSwitch()->isValidStateUpdate(StateDelta(oldState, newState)),
      true);
}

TEST_F(BcmLabelForwardingTests, InvalidLabelFIBDelta) {
  LabelNextHopSet nexthops;
  nexthops.insert(getLabelNextHop(
      folly::IPAddressV4("10.0.0.1"),
      LabelForwardingAction(
          LabelForwardingAction::LabelForwardingType::SWAP, 1001)));
  nexthops.insert(getLabelNextHop(
      folly::IPAddressV4("10.0.0.2"),
      LabelForwardingAction(
          LabelForwardingAction::LabelForwardingType::SWAP, 1002)));
  nexthops.insert(getLabelNextHop(
      folly::IPAddressV4("10.0.0.3"),
      LabelForwardingAction(
          LabelForwardingAction::LabelForwardingType::SWAP, 1003)));
  nexthops.insert(getLabelNextHop(
      folly::IPAddressV4("10.0.0.4"),
      LabelForwardingAction(
          LabelForwardingAction::LabelForwardingType::PUSH,
          LabelForwardingAction::LabelStack{1001, 1002})));

  auto oldState = getProgrammedState();
  auto newState = oldState->clone();

  auto* labelFib =
      newState->getLabelForwardingInformationBase().get()->modify(&newState);

  labelFib->programLabel(
      &newState,
      1000,
      ClientID::OPENR,
      AdminDistance::DIRECTLY_CONNECTED,
      nexthops);

  EXPECT_EQ(labelFib->isPublished(), false);
  newState->publish();

  EXPECT_EQ(
      this->getHwSwitch()->isValidStateUpdate(StateDelta(oldState, newState)),
      false);
}

TEST_F(BcmLabelForwardingTests, ValidPushStack) {
  LabelForwardingAction::LabelStack stack;
  for (auto i = 0; i < getPlatform()->getAsic()->getMaxLabelStackDepth(); i++) {
    stack.push_back(1001 + i);
  }

  auto state = getProgrammedState();
  auto newState = state->clone();
  auto* writeableLabelFib =
      newState->getLabelForwardingInformationBase()->modify(&newState);
  LabelNextHopSet nexthops;
  nexthops.emplace(getLabelNextHop(
      folly::IPAddressV4("10.0.0.1"),
      LabelForwardingAction(
          LabelForwardingAction::LabelForwardingType::PUSH, std::move(stack))));

  writeableLabelFib->programLabel(
      &newState,
      1000,
      ClientID::OPENR,
      AdminDistance::DIRECTLY_CONNECTED,
      std::move(nexthops));

  newState->publish();
  EXPECT_EQ(
      this->getHwSwitch()->isValidStateUpdate(StateDelta(state, newState)),
      true);
}

TEST_F(BcmLabelForwardingTests, InvalidPushStack) {
  LabelForwardingAction::LabelStack stack;
  for (auto i = 0; i < getPlatform()->getAsic()->getMaxLabelStackDepth() + 1;
       i++) {
    stack.push_back(1001 + i);
  }

  auto state = getProgrammedState();
  auto newState = state->clone();
  auto* writeableLabelFib =
      newState->getLabelForwardingInformationBase()->modify(&newState);
  LabelNextHopSet nexthops;
  nexthops.emplace(getLabelNextHop(
      folly::IPAddressV4("10.0.0.1"),
      LabelForwardingAction(
          LabelForwardingAction::LabelForwardingType::PUSH, std::move(stack))));

  writeableLabelFib->programLabel(
      &newState,
      1000,
      ClientID::OPENR,
      AdminDistance::DIRECTLY_CONNECTED,
      std::move(nexthops));

  newState->publish();
  EXPECT_EQ(
      this->getHwSwitch()->isValidStateUpdate(StateDelta(state, newState)),
      false);
}

} // namespace facebook::fboss
