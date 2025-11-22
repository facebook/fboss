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
#include "fboss/agent/state/FibInfo.h"
#include "fboss/agent/state/FibInfoMap.h"
#include "fboss/agent/state/ForwardingInformationBaseContainer.h"
#include "fboss/agent/state/ForwardingInformationBaseMap.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/TestUtils.h"

#include <gtest/gtest.h>
#include <memory>

namespace facebook::fboss {

namespace {

std::shared_ptr<ForwardingInformationBaseContainer> createFibContainer(
    RouterID vrf) {
  return std::make_shared<ForwardingInformationBaseContainer>(vrf);
}

std::shared_ptr<FibInfo> createFibInfo(RouterID vrf) {
  auto fibInfo = std::make_shared<FibInfo>();
  auto fibsMap = std::make_shared<ForwardingInformationBaseMap>();
  auto fibContainer = createFibContainer(vrf);
  fibsMap->updateForwardingInformationBaseContainer(fibContainer);
  fibInfo->ref<switch_state_tags::fibsMap>() = fibsMap;
  return fibInfo;
}

HwSwitchMatcher createMatcher(int64_t switchId) {
  return HwSwitchMatcher(std::unordered_set<SwitchID>{SwitchID(switchId)});
}

} // namespace

class FibInfoTest : public ::testing::Test {
 protected:
  void SetUp() override {
    state_ = std::make_shared<SwitchState>();
  }

  std::shared_ptr<SwitchState> state_;
};

TEST_F(FibInfoTest, SerializationAndGetfibsMap) {
  auto fibInfo = createFibInfo(RouterID(0));

  // Test serialization
  validateNodeSerialization(*fibInfo);

  // Test getting fibsMap
  auto fibsMap = fibInfo->getfibsMap();
  EXPECT_EQ(fibsMap->size(), 1);
  EXPECT_EQ(fibsMap->getNodeIf(RouterID(0))->getID(), RouterID(0));
}

TEST_F(FibInfoTest, GetFibContainer) {
  // Test getting existing FIB container
  auto fibInfo = createFibInfo(RouterID(5));
  auto fibContainer = fibInfo->getFibContainerIf(RouterID(5));
  EXPECT_NE(fibContainer, nullptr);
  EXPECT_EQ(fibContainer->getID(), RouterID(5));

  // Test getting non-existent FIB container
  auto nonExistentContainer = fibInfo->getFibContainerIf(RouterID(10));
  EXPECT_EQ(nonExistentContainer, nullptr);
}

TEST_F(FibInfoTest, ModifyPublished) {
  auto fibInfo = createFibInfo(RouterID(0));

  auto fibInfoMap = std::make_shared<MultiSwitchFibInfoMap>();
  auto matcher = createMatcher(10);
  fibInfoMap->addNode(matcher.matcherString(), fibInfo);
  state_->resetFibsInfoMap(fibInfoMap);
  state_->publish();

  auto modifiedFibInfo = fibInfo->modify(&state_);
  EXPECT_NE(modifiedFibInfo, fibInfo.get());
  EXPECT_EQ(
      state_->getFibsInfoMap()->getFibInfo(matcher).get(), modifiedFibInfo);
}

TEST_F(FibInfoTest, UpdateFibContainer) {
  // Setup initial state with RouterID(0) container
  auto fibInfo = createFibInfo(RouterID(0));
  auto fibInfoMap = std::make_shared<MultiSwitchFibInfoMap>();
  auto matcher = createMatcher(10);
  fibInfoMap->addNode(matcher.matcherString(), fibInfo);
  state_->resetFibsInfoMap(fibInfoMap);
  state_->publish();

  // Verify initial container exists
  EXPECT_NE(fibInfo->getFibContainerIf(RouterID(0)), nullptr);
  EXPECT_EQ(fibInfo->getFibContainerIf(RouterID(1)), nullptr);

  // Add a new container with RouterID(1)
  auto newFibContainer = createFibContainer(RouterID(1));
  fibInfo->updateFibContainer(newFibContainer, &state_);

  // Verify the FibInfo was cloned
  auto updatedFibInfo =
      state_->getFibsInfoMap()->getNodeIf(matcher.matcherString());
  EXPECT_NE(updatedFibInfo, nullptr);
  EXPECT_NE(updatedFibInfo, fibInfo);

  // Verify both containers exist in the updated FibInfo
  EXPECT_NE(updatedFibInfo->getFibContainerIf(RouterID(0)), nullptr);
  EXPECT_NE(updatedFibInfo->getFibContainerIf(RouterID(1)), nullptr);
}

class MultiSwitchFibInfoMapTest : public ::testing::Test {
 protected:
  void SetUp() override {
    state_ = std::make_shared<SwitchState>();
  }

  std::shared_ptr<SwitchState> state_;
};

TEST_F(MultiSwitchFibInfoMapTest, SerializationAndGetFibInfoTest) {
  auto fibInfoMap = std::make_shared<MultiSwitchFibInfoMap>();
  auto fibInfo1 = createFibInfo(RouterID(0));
  auto fibInfo2 = createFibInfo(RouterID(1));

  auto matcher1 = createMatcher(10);
  auto matcher2 = createMatcher(20);
  fibInfoMap->addNode(matcher1.matcherString(), fibInfo1);
  fibInfoMap->addNode(matcher2.matcherString(), fibInfo2);

  // Test serialization
  validateThriftMapMapSerialization(*fibInfoMap);

  // Test getting existing FibInfo
  auto retrievedFibInfo1 = fibInfoMap->getFibInfo(matcher1);
  EXPECT_NE(retrievedFibInfo1, nullptr);
  EXPECT_EQ(retrievedFibInfo1, fibInfo1);

  auto retrievedFibInfo2 = fibInfoMap->getFibInfo(matcher2);
  EXPECT_EQ(retrievedFibInfo2, fibInfo2);

  // Test getting non-existent FibInfo
  auto matcher3 = createMatcher(30);
  auto nonExistentFibInfo = fibInfoMap->getFibInfo(matcher3);
  EXPECT_EQ(nonExistentFibInfo, nullptr);
}

TEST_F(MultiSwitchFibInfoMapTest, GetNodeByVrfAndHwSwitchMatcherTest) {
  auto fibInfoMap = std::make_shared<MultiSwitchFibInfoMap>();
  auto fibInfo1 = createFibInfo(RouterID(5));
  auto fibInfo2 = createFibInfo(RouterID(10));

  auto matcher1 = createMatcher(10);
  auto matcher2 = createMatcher(20);
  fibInfoMap->addNode(matcher1.matcherString(), fibInfo1);
  fibInfoMap->addNode(matcher2.matcherString(), fibInfo2);

  // Test getNode searches across all switches
  auto fibContainer5 = fibInfoMap->getFibContainerIf(RouterID(5));
  EXPECT_NE(fibContainer5, nullptr);
  EXPECT_EQ(fibContainer5->getID(), RouterID(5));

  auto fibContainer10 = fibInfoMap->getFibContainerIf(RouterID(10));
  EXPECT_NE(fibContainer10, nullptr);
  EXPECT_EQ(fibContainer10->getID(), RouterID(10));

  // Test getting non-existent FIB container by VRF
  auto nonExistentContainer = fibInfoMap->getFibContainerIf(RouterID(15));
  EXPECT_EQ(nonExistentContainer, nullptr);

  // Test getFibContainerIf(matcher, RouterID vrf) gets from specific switch
  auto fibContainer5FromMatcher1 =
      fibInfoMap->getFibContainerIf(matcher1, RouterID(5));
  EXPECT_EQ(fibContainer5FromMatcher1->getID(), RouterID(5));

  auto fibContainer10FromMatcher2 =
      fibInfoMap->getFibContainerIf(matcher2, RouterID(10));
  EXPECT_EQ(fibContainer10FromMatcher2->getID(), RouterID(10));

  // Test getting non-existent VRF from existing switch
  auto nonExistentVrf = fibInfoMap->getFibContainerIf(matcher1, RouterID(99));
  EXPECT_EQ(nonExistentVrf, nullptr);

  // Test getting VRF from wrong switch
  auto wrongSwitch = fibInfoMap->getFibContainerIf(matcher1, RouterID(10));
  EXPECT_EQ(wrongSwitch, nullptr);

  // Test getting from non-existent switch
  auto matcher3 = createMatcher(30);
  auto nonExistentSwitch = fibInfoMap->getFibContainerIf(matcher3, RouterID(5));
  EXPECT_EQ(nonExistentSwitch, nullptr);
}

TEST_F(MultiSwitchFibInfoMapTest, UpdateFibInfoTest) {
  auto fibInfoMap = std::make_shared<MultiSwitchFibInfoMap>();
  auto fibInfo1 = createFibInfo(RouterID(0));
  auto fibInfo2 = createFibInfo(RouterID(1));
  auto matcher = createMatcher(10);

  // Test adding new FibInfo
  fibInfoMap->updateFibInfo(fibInfo1, matcher);
  auto retrievedFibInfo = fibInfoMap->getFibInfo(matcher);
  EXPECT_EQ(retrievedFibInfo, fibInfo1);
  EXPECT_EQ(fibInfoMap->size(), 1);

  // Test updating existing FibInfo
  fibInfoMap->updateFibInfo(fibInfo2, matcher);
  EXPECT_EQ(fibInfoMap->size(), 1);

  retrievedFibInfo = fibInfoMap->getFibInfo(matcher);
  EXPECT_EQ(retrievedFibInfo, fibInfo2);
  EXPECT_NE(retrievedFibInfo, fibInfo1);
}

TEST_F(MultiSwitchFibInfoMapTest, ModifyPublishedStateTest) {
  auto fibInfoMap = std::make_shared<MultiSwitchFibInfoMap>();
  auto fibInfo = createFibInfo(RouterID(0));
  auto matcher = createMatcher(0);

  fibInfoMap->addNode(matcher.matcherString(), fibInfo);
  state_->resetFibsInfoMap(fibInfoMap);
  state_->publish();

  auto modifiedMap = fibInfoMap->modify(&state_);
  EXPECT_NE(modifiedMap, fibInfoMap.get());
  EXPECT_EQ(state_->getFibsInfoMap().get(), modifiedMap);
}

TEST_F(MultiSwitchFibInfoMapTest, MultipleNodesAndEmptyMapTest) {
  auto fibInfoMap = std::make_shared<MultiSwitchFibInfoMap>();

  // Test empty map behavior
  EXPECT_EQ(fibInfoMap->size(), 0);
  EXPECT_EQ(fibInfoMap->getFibContainerIf(RouterID(0)), nullptr);
  EXPECT_EQ(fibInfoMap->getFibInfo(createMatcher(10)), nullptr);

  // Add multiple nodes and test
  auto fibInfo1 = createFibInfo(RouterID(0));
  auto fibInfo2 = createFibInfo(RouterID(1));
  auto fibInfo3 = createFibInfo(RouterID(2));

  auto matcher1 = createMatcher(10);
  auto matcher2 = createMatcher(20);
  auto matcher3 = createMatcher(30);

  fibInfoMap->updateFibInfo(fibInfo1, matcher1);
  fibInfoMap->updateFibInfo(fibInfo2, matcher2);
  fibInfoMap->updateFibInfo(fibInfo3, matcher3);

  // Test multiple nodes behavior
  EXPECT_EQ(fibInfoMap->size(), 3);
  EXPECT_EQ(fibInfoMap->getFibInfo(matcher1), fibInfo1);
  EXPECT_EQ(fibInfoMap->getFibInfo(matcher2), fibInfo2);
  EXPECT_EQ(fibInfoMap->getFibInfo(matcher3), fibInfo3);
}

TEST_F(MultiSwitchFibInfoMapTest, GetRouteCount) {
  auto fibInfoMap = std::make_shared<MultiSwitchFibInfoMap>();

  // Verify initial route count is 0 for empty map
  auto [emptyV4Count, emptyV6Count] = fibInfoMap->getRouteCount();
  EXPECT_EQ(emptyV4Count, 0);
  EXPECT_EQ(emptyV6Count, 0);

  // Create FibInfo with RouterID(0)
  auto fibInfo = std::make_shared<FibInfo>();
  auto fibsMap = std::make_shared<ForwardingInformationBaseMap>();
  auto fibContainer =
      std::make_shared<ForwardingInformationBaseContainer>(RouterID(0));

  // Create and add 2 IPv4 routes
  ForwardingInformationBaseV4 fibV4;
  RouteFields<folly::IPAddressV4> routeV4_1(
      RoutePrefixV4{folly::IPAddressV4("10.0.0.0"), 24});
  RouteFields<folly::IPAddressV4> routeV4_2(
      RoutePrefixV4{folly::IPAddressV4("192.168.1.0"), 24});
  fibV4.addNode(std::make_shared<RouteV4>(routeV4_1.toThrift()));
  fibV4.addNode(std::make_shared<RouteV4>(routeV4_2.toThrift()));
  fibContainer->setFib(fibV4.clone());

  // Create and add 3 IPv6 routes
  ForwardingInformationBaseV6 fibV6;
  RouteFields<folly::IPAddressV6> routeV6_1(
      RoutePrefixV6{folly::IPAddressV6("2001:db8::1"), 64});
  RouteFields<folly::IPAddressV6> routeV6_2(
      RoutePrefixV6{folly::IPAddressV6("2001:db8:1::"), 64});
  RouteFields<folly::IPAddressV6> routeV6_3(
      RoutePrefixV6{folly::IPAddressV6("2001:db8:2::"), 64});
  fibV6.addNode(std::make_shared<RouteV6>(routeV6_1.toThrift()));
  fibV6.addNode(std::make_shared<RouteV6>(routeV6_2.toThrift()));
  fibV6.addNode(std::make_shared<RouteV6>(routeV6_3.toThrift()));
  fibContainer->setFib(fibV6.clone());

  fibsMap->updateForwardingInformationBaseContainer(fibContainer);
  fibInfo->ref<switch_state_tags::fibsMap>() = fibsMap;

  // Add FibInfo to FibInfoMap
  auto matcher = createMatcher(10);
  fibInfoMap->updateFibInfo(fibInfo, matcher);

  // Verify route count after adding routes: 2 IPv4 and 3 IPv6
  auto [v4Count, v6Count] = fibInfoMap->getRouteCount();
  EXPECT_EQ(v4Count, 2);
  EXPECT_EQ(v6Count, 3);
}

TEST_F(MultiSwitchFibInfoMapTest, GetAllFibNodes) {
  auto fibInfoMap = std::make_shared<MultiSwitchFibInfoMap>();

  // Test empty map returns empty merged map
  auto emptyMergedMap = fibInfoMap->getAllFibNodes();
  EXPECT_NE(emptyMergedMap, nullptr);
  EXPECT_EQ(emptyMergedMap->size(), 0);

  // Create FibInfo objects for different switches with different RouterIDs
  auto fibInfo1 = createFibInfo(RouterID(0));
  auto fibInfo2 = createFibInfo(RouterID(1));
  auto fibInfo3 = createFibInfo(RouterID(2));

  // Add FibInfos to the map
  auto matcher1 = createMatcher(10);
  auto matcher2 = createMatcher(20);
  auto matcher3 = createMatcher(30);
  fibInfoMap->updateFibInfo(fibInfo1, matcher1);
  fibInfoMap->updateFibInfo(fibInfo2, matcher2);
  fibInfoMap->updateFibInfo(fibInfo3, matcher3);

  // Get merged map and verify it contains all FibContainers from all switches
  auto mergedMap = fibInfoMap->getAllFibNodes();
  EXPECT_EQ(mergedMap->size(), 3);

  // Verify all FibContainers are present in the merged map
  auto mergedContainer0 = mergedMap->getFibContainerIf(RouterID(0));
  EXPECT_EQ(mergedContainer0->getID(), RouterID(0));

  auto mergedContainer1 = mergedMap->getFibContainerIf(RouterID(1));
  EXPECT_EQ(mergedContainer1->getID(), RouterID(1));

  auto mergedContainer2 = mergedMap->getFibContainerIf(RouterID(2));
  EXPECT_EQ(mergedContainer2->getID(), RouterID(2));
}

} // namespace facebook::fboss
