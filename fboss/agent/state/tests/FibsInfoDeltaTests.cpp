/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/HwSwitchMatcher.h"
#include "fboss/agent/state/DeltaFunctions.h"
#include "fboss/agent/state/FibInfo.h"
#include "fboss/agent/state/FibInfoDelta.h"
#include "fboss/agent/state/FibInfoMap.h"
#include "fboss/agent/state/ForwardingInformationBase.h"
#include "fboss/agent/state/ForwardingInformationBaseContainer.h"
#include "fboss/agent/state/Route.h"
#include "fboss/agent/state/RouteTypes.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/TestUtils.h"

#include <gtest/gtest.h>

using namespace facebook::fboss;
using folly::IPAddressV4;
using folly::IPAddressV6;

namespace {

template <typename AddressT>
std::shared_ptr<Route<AddressT>> createRouteFromPrefix(
    const RoutePrefix<AddressT>& prefix) {
  RouteFields<AddressT> fields(prefix);
  return std::make_shared<Route<AddressT>>(fields.toThrift());
}

std::shared_ptr<ForwardingInformationBaseV4> createTestFibV4() {
  ForwardingInformationBaseV4 fibV4;
  fibV4.addNode(
      createRouteFromPrefix(RoutePrefixV4(folly::IPAddressV4("10.0.0.0"), 8)));
  fibV4.addNode(createRouteFromPrefix(
      RoutePrefixV4(folly::IPAddressV4("192.168.0.0"), 16)));
  return fibV4.clone();
}

std::shared_ptr<ForwardingInformationBaseV6> createTestFibV6() {
  ForwardingInformationBaseV6 fibV6;
  fibV6.addNode(
      createRouteFromPrefix(RoutePrefixV6(folly::IPAddressV6("fc00::"), 7)));
  fibV6.addNode(createRouteFromPrefix(
      RoutePrefixV6(folly::IPAddressV6("2001:db8::"), 32)));
  return fibV6.clone();
}

std::shared_ptr<ForwardingInformationBaseContainer> createTestFibContainer(
    const RouterID& vrf) {
  auto container = std::make_shared<ForwardingInformationBaseContainer>(vrf);
  container->setFib(createTestFibV4());
  container->setFib(createTestFibV6());
  return container;
}

HwSwitchMatcher testMatcher() {
  return HwSwitchMatcher{std::unordered_set<SwitchID>{SwitchID(0)}};
}

} // namespace

class FibInfoDeltaTest : public ::testing::Test {
 public:
  void SetUp() override {
    oldState = std::make_shared<SwitchState>();
    newState = std::make_shared<SwitchState>();
  }

  std::shared_ptr<SwitchState> oldState;
  std::shared_ptr<SwitchState> newState;
};

TEST_F(FibInfoDeltaTest, FibInfoAddedTest) {
  // Old state has no FibInfo
  oldState->resetFibsInfoMap(std::make_shared<MultiSwitchFibInfoMap>());

  // New state has FibInfo with routes
  auto newFibInfo = std::make_shared<FibInfo>();
  auto newFibInfoMap = std::make_shared<MultiSwitchFibInfoMap>();
  newFibInfoMap->addNode(testMatcher().matcherString(), newFibInfo);
  newState->resetFibsInfoMap(newFibInfoMap);

  auto vrf = RouterID(0);
  auto fibContainer = createTestFibContainer(vrf);
  newFibInfo->updateFibContainer(fibContainer, &newState);

  // Create delta
  StateDelta delta(oldState, newState);
  auto fibsInfoDelta = delta.getFibsInfoDelta();

  // Verify
  int addedCount = 0;
  for (const auto& fibInfoDelta : fibsInfoDelta) {
    EXPECT_FALSE(fibInfoDelta.getOld());
    EXPECT_TRUE(fibInfoDelta.getNew());
    addedCount++;
  }
  EXPECT_EQ(addedCount, 1);
}

TEST_F(FibInfoDeltaTest, FibInfoRemovedTest) {
  // Old state has FibInfo with routes
  auto oldFibInfo = std::make_shared<FibInfo>();
  auto oldFibInfoMap = std::make_shared<MultiSwitchFibInfoMap>();
  oldFibInfoMap->addNode(testMatcher().matcherString(), oldFibInfo);
  oldState->resetFibsInfoMap(oldFibInfoMap);

  auto vrf = RouterID(0);
  auto fibContainer = createTestFibContainer(vrf);
  oldFibInfo->updateFibContainer(fibContainer, &oldState);

  // New state has empty FibInfoMap
  newState->resetFibsInfoMap(std::make_shared<MultiSwitchFibInfoMap>());

  // Create delta
  StateDelta delta(oldState, newState);
  auto fibsInfoDelta = delta.getFibsInfoDelta();

  // Verify
  int removedCount = 0;
  for (const auto& fibInfoDelta : fibsInfoDelta) {
    EXPECT_TRUE(fibInfoDelta.getOld());
    EXPECT_FALSE(fibInfoDelta.getNew());
    removedCount++;
  }
  EXPECT_EQ(removedCount, 1);
}

TEST_F(FibInfoDeltaTest, RoutesChangedTest) {
  // Old state with 2 routes
  auto oldFibInfo = std::make_shared<FibInfo>();
  auto oldFibInfoMap = std::make_shared<MultiSwitchFibInfoMap>();
  oldFibInfoMap->addNode(testMatcher().matcherString(), oldFibInfo);
  oldState->resetFibsInfoMap(oldFibInfoMap);

  auto vrf = RouterID(0);
  auto oldFibContainer = createTestFibContainer(vrf);
  oldFibInfo->updateFibContainer(oldFibContainer, &oldState);

  // New state with different routes
  auto newFibInfo = std::make_shared<FibInfo>();
  auto newFibInfoMap = std::make_shared<MultiSwitchFibInfoMap>();
  newFibInfoMap->addNode(testMatcher().matcherString(), newFibInfo);
  newState->resetFibsInfoMap(newFibInfoMap);

  // Create a new FibContainer with only one route
  auto newFibContainer =
      std::make_shared<ForwardingInformationBaseContainer>(vrf);
  ForwardingInformationBaseV4 fibV4;
  fibV4.addNode(
      createRouteFromPrefix(RoutePrefixV4(folly::IPAddressV4("10.0.0.0"), 8)));
  // Remove 192.168.0.0/16
  // Add new route
  fibV4.addNode(createRouteFromPrefix(
      RoutePrefixV4(folly::IPAddressV4("172.16.0.0"), 12)));
  newFibContainer->setFib(fibV4.clone());
  newFibInfo->updateFibContainer(newFibContainer, &newState);

  // Create delta
  StateDelta delta(oldState, newState);
  auto fibsInfoDelta = delta.getFibsInfoDelta();

  // Verify
  int changedCount = 0;
  for (const auto& fibInfoDelta : fibsInfoDelta) {
    EXPECT_TRUE(fibInfoDelta.getOld());
    EXPECT_TRUE(fibInfoDelta.getNew());

    // Verify fibsMapV2 delta
    auto fibsMapDelta = fibInfoDelta.getFibsMapDelta();
    for (const auto& fibContainerDelta : fibsMapDelta) {
      EXPECT_TRUE(fibContainerDelta.getOld());
      EXPECT_TRUE(fibContainerDelta.getNew());

      // Count route changes
      int routesAdded = 0, routesRemoved = 0;
      DeltaFunctions::forEachChanged(
          fibContainerDelta.getFibDelta<IPAddressV4>(),
          [](const auto&, const auto&) {},
          [&](const auto&) { routesAdded++; },
          [&](const auto&) { routesRemoved++; });

      EXPECT_EQ(routesAdded, 1);
      EXPECT_EQ(routesRemoved, 1);
    }
    changedCount++;
  }
  EXPECT_EQ(changedCount, 1);
}

TEST_F(FibInfoDeltaTest, NoChangesTest) {
  // Same FibInfo in both states
  auto fibInfo = std::make_shared<FibInfo>();
  auto fibInfoMap = std::make_shared<MultiSwitchFibInfoMap>();
  fibInfoMap->addNode(testMatcher().matcherString(), fibInfo);

  auto vrf = RouterID(0);
  auto fibContainer = createTestFibContainer(vrf);

  auto state = std::make_shared<SwitchState>();
  state->resetFibsInfoMap(fibInfoMap);
  fibInfo->updateFibContainer(fibContainer, &state);

  oldState = state;
  newState = state;

  // Create delta
  StateDelta delta(oldState, newState);
  auto fibsInfoDelta = delta.getFibsInfoDelta();

  // Should have no changes
  EXPECT_TRUE(DeltaFunctions::isEmpty(fibsInfoDelta));
}

TEST_F(FibInfoDeltaTest, MultiVrfChangesTest) {
  // Old state: VRF 0 and VRF 1
  auto oldFibInfo = std::make_shared<FibInfo>();
  auto oldFibInfoMap = std::make_shared<MultiSwitchFibInfoMap>();
  oldFibInfoMap->addNode(testMatcher().matcherString(), oldFibInfo);
  oldState->resetFibsInfoMap(oldFibInfoMap);

  oldFibInfo->updateFibContainer(
      createTestFibContainer(RouterID(0)), &oldState);
  oldFibInfo->updateFibContainer(
      createTestFibContainer(RouterID(1)), &oldState);

  // New state: VRF 0 removed, VRF 1 kept, VRF 2 added
  auto newFibInfo = std::make_shared<FibInfo>();
  auto newFibInfoMap = std::make_shared<MultiSwitchFibInfoMap>();
  newFibInfoMap->addNode(testMatcher().matcherString(), newFibInfo);
  newState->resetFibsInfoMap(newFibInfoMap);

  newFibInfo->updateFibContainer(
      createTestFibContainer(RouterID(1)), &newState);
  newFibInfo->updateFibContainer(
      createTestFibContainer(RouterID(2)), &newState);

  // Create delta
  StateDelta delta(oldState, newState);
  auto fibsInfoDelta = delta.getFibsInfoDelta();

  for (const auto& fibInfoDelta : fibsInfoDelta) {
    int vrfsAdded = 0, vrfsRemoved = 0, vrfsChanged = 0;

    DeltaFunctions::forEachChanged(
        fibInfoDelta.getFibsMapDelta(),
        [&](const auto&, const auto&) { vrfsChanged++; },
        [&](const auto&) { vrfsAdded++; },
        [&](const auto&) { vrfsRemoved++; });

    EXPECT_EQ(vrfsChanged, 1);
    EXPECT_EQ(vrfsAdded, 1);
    EXPECT_EQ(vrfsRemoved, 1);
  }
}
