// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/state/DeltaFunctions.h"
#include "fboss/agent/state/MapDelta.h"

#include "fboss/agent/state/Mirror.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/TestUtils.h"

#include <gtest/gtest.h>

using namespace facebook::fboss;

class MultiSwitchMapDeltaTest : public ::testing::Test {
 public:
  void SetUp() override {
    config_ = testConfigA();
    platform_ = createMockPlatform();
    state_ = testStateA();
    state_->publish();
  }

  void addMirror(
      std::shared_ptr<SwitchState>& inState,
      std::shared_ptr<Mirror> mirror,
      const HwSwitchMatcher& matcher) {
    auto mirrors = inState->getMirrors()->modify(&inState);
    mirrors->addNode(mirror, matcher);
  }

  const HwSwitchMatcher& getMatcher0() {
    static HwSwitchMatcher matcher0("id=0");
    return matcher0;
  }

  const HwSwitchMatcher& getMatcher012() {
    static HwSwitchMatcher matcher012("id=0,1,2");
    return matcher012;
  }

  std::shared_ptr<Mirror> createMirror(std::string name) {
    return std::make_shared<Mirror>(
        name,
        std::optional<PortID>(PortID(1)),
        std::optional<folly::IPAddress>());
  }

  const HwSwitchMatcher& getMatcher023() {
    static HwSwitchMatcher matcher023("id=0,2,3");
    return matcher023;
  }

  void processDelta(const MultiSwitchMapDelta<MultiSwitchMirrorMap>& delta) {
    DeltaFunctions::forEachChanged(
        delta,
        [&](auto, auto newMirror) { changed.insert(newMirror->getID()); },
        [&](auto newMirror) { added.insert(newMirror->getID()); },
        [&](auto oldMirror) { removed.insert(oldMirror->getID()); });
  }

  std::shared_ptr<SwitchState> testState1() {
    auto state = state_->clone();
    addMirror(state, createMirror("mirror0_a"), getMatcher0());
    addMirror(state, createMirror("mirror0_b"), getMatcher0());
    addMirror(state, createMirror("mirror0_c"), getMatcher0());

    addMirror(state, createMirror("mirror012_a"), getMatcher012());
    addMirror(state, createMirror("mirror012_b"), getMatcher012());
    addMirror(state, createMirror("mirror012_c"), getMatcher012());

    addMirror(state, createMirror("mirror023_a"), getMatcher023());
    addMirror(state, createMirror("mirror023_b"), getMatcher023());
    addMirror(state, createMirror("mirror023_c"), getMatcher023());
    state->publish();
    return state;
  }

  std::shared_ptr<SwitchState> state_;
  std::shared_ptr<Platform> platform_;
  cfg::SwitchConfig config_;
  std::set<std::string> added, removed, changed;
};

TEST_F(MultiSwitchMapDeltaTest, AddMirror) {
  auto state1 = testState1();
  EXPECT_EQ(state1->getMirrors()->numNodes(), 9);

  auto mirrors0 = state_->getMirrors();
  auto mirrors1 = state1->getMirrors();
  processDelta(MultiSwitchMapDelta<MultiSwitchMirrorMap>(
      mirrors0.get(), mirrors1.get()));
  EXPECT_EQ(added.size(), 9);
  EXPECT_EQ(removed.size(), 0);
  EXPECT_EQ(changed.size(), 0);
}

TEST_F(MultiSwitchMapDeltaTest, ChangeMirror) {
  auto state1 = testState1();
  auto state2 = state1;
  EXPECT_EQ(state1->getMirrors()->numNodes(), 9);

  auto mirrors = state2->getMirrors()->modify(&state2);
  auto mirrorA = mirrors->getNodeIf("mirror012_a");
  mirrorA = mirrorA->clone();
  mirrorA->setEgressPort(PortID(2));
  mirrors->updateNode(mirrorA, getMatcher012());

  auto mirrorB = mirrors->getNodeIf("mirror023_c");
  mirrorB = mirrorB->clone();
  mirrorB->setEgressPort(PortID(2));
  mirrors->updateNode(mirrorB, getMatcher023());

  auto mirrors0 = state1->getMirrors();
  auto mirrors1 = state2->getMirrors();
  state2->publish();
  processDelta(MultiSwitchMapDelta<MultiSwitchMirrorMap>(
      mirrors0.get(), mirrors1.get()));
  EXPECT_EQ(added.size(), 0);
  EXPECT_EQ(removed.size(), 0);
  EXPECT_EQ(changed.size(), 2);
}

TEST_F(MultiSwitchMapDeltaTest, RemoveMirror) {
  auto state1 = testState1();
  EXPECT_EQ(state1->getMirrors()->numNodes(), 9);

  auto mirrors0 = state1->getMirrors();
  auto state2 = state1->clone();
  auto mirrors = state2->getMirrors()->modify(&state2);
  auto mirrorA = mirrors->getNodeIf("mirror012_a");
  auto mirrorB = mirrors->getNodeIf("mirror023_c");

  mirrors->removeNode(mirrorA);
  mirrors->removeNode(mirrorB);
  state2->publish();

  auto mirrors1 = state2->getMirrors();
  processDelta(MultiSwitchMapDelta<MultiSwitchMirrorMap>(
      mirrors0.get(), mirrors1.get()));

  EXPECT_EQ(added.size(), 0);
  EXPECT_EQ(removed.size(), 2);
  EXPECT_EQ(changed.size(), 0);
}

TEST_F(MultiSwitchMapDeltaTest, NoChange) {
  auto state1 = testState1();
  auto state2 = state1->clone();
  auto mirrors0 = state1->getMirrors();
  auto mirrors1 = state2->getMirrors();
  processDelta(MultiSwitchMapDelta<MultiSwitchMirrorMap>(
      mirrors0.get(), mirrors1.get()));

  EXPECT_EQ(added.size(), 0);
  EXPECT_EQ(removed.size(), 0);
  EXPECT_EQ(changed.size(), 0);
}

TEST_F(MultiSwitchMapDeltaTest, NewMap) {
  auto state1 = testState1();
  auto state2 = state1->clone();
  auto mirrors0 = state1->getMirrors();
  auto mirrors1 = state2->getMirrors()->modify(&state2);
  mirrors1->addNode(createMirror("mirror12_a"), HwSwitchMatcher("id=1,2"));
  processDelta(
      MultiSwitchMapDelta<MultiSwitchMirrorMap>(mirrors0.get(), mirrors1));

  EXPECT_EQ(added.size(), 1);
  EXPECT_EQ(removed.size(), 0);
  EXPECT_EQ(changed.size(), 0);
}

TEST_F(MultiSwitchMapDeltaTest, EmptyMap) {
  auto state1 = state_;
  state1->publish();
  auto state2 = bringAllPortsUp(state1);
  state2->publish();
  auto delta = MultiSwitchMapDelta<MultiSwitchMirrorMap>(
      state1->getMirrors().get(), state2->getMirrors().get());
  EXPECT_EQ(delta.begin(), delta.end());
}

TEST_F(MultiSwitchMapDeltaTest, AddRemoveUpdate) {
  auto state1 = testState1();
  auto state2 = state1->clone();
  auto mirrors = state2->getMirrors()->modify(&state2);
  auto removedMirror = mirrors->getNodeIf("mirror0_a");
  mirrors->removeNode("mirror0_a");
  mirrors->addNode(createMirror("mirror012_d"), getMatcher012());
  auto addedMirror = mirrors->getNodeIf("mirror012_d");
  auto changedOld = mirrors->getNodeIf("mirror023_c");
  auto mirror23c = mirrors->getNodeIf("mirror023_c")->clone();
  mirror23c->setEgressPort(PortID(2));
  mirrors->updateNode(mirror23c, getMatcher023());
  auto changedNew = mirrors->getNodeIf("mirror023_c");
  state2->publish();
  auto delta = MultiSwitchMapDelta<MultiSwitchMirrorMap>(
      state1->getMirrors().get(), state2->getMirrors().get());
  processDelta(delta);
  EXPECT_EQ(added.size(), 1);
  EXPECT_EQ(removed.size(), 1);
  EXPECT_EQ(changed.size(), 1);

  DeltaFunctions::forEachChanged(
      delta,
      [&](auto oldMirror, auto newMirror) {
        EXPECT_EQ(oldMirror, changedOld);
        EXPECT_EQ(newMirror, changedNew);
      },
      [&](auto newMirror) { EXPECT_EQ(newMirror, addedMirror); },
      [&](auto oldMirror) { EXPECT_EQ(oldMirror, removedMirror); });
}
