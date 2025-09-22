/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/state/LabelForwardingAction.h"
#include "fboss/agent/state/RouteNextHop.h"

#include <folly/IPAddress.h>
#include <gtest/gtest.h>
#include <unordered_map>
#include <unordered_set>

using namespace facebook::fboss;

TEST(RouteNextHopTest, ResolvedNextHopEquality) {
  auto addr1 = folly::IPAddress("2401:db00::1");
  auto addr2 = folly::IPAddress("2401:db00::2");
  auto intfID1 = InterfaceID(1);
  auto intfID2 = InterfaceID(2);

  // Test basic equality
  ResolvedNextHop nh1(addr1, intfID1, 10);
  ResolvedNextHop nh2(addr1, intfID1, 10);
  EXPECT_EQ(nh1, nh2);

  // Test inequality for different addresses
  ResolvedNextHop nh3(addr2, intfID1, 10);
  EXPECT_NE(nh1, nh3);

  // Test inequality for different interface IDs
  ResolvedNextHop nh4(addr1, intfID2, 10);
  EXPECT_NE(nh1, nh4);

  // Test inequality for different weights
  ResolvedNextHop nh5(addr1, intfID1, 20);
  EXPECT_NE(nh1, nh5);
}

TEST(RouteNextHopTest, ResolvedNextHopEqualityWithOptionalFields) {
  auto addr = folly::IPAddress("2401:db00::1");
  auto intfID = InterfaceID(1);

  // Test with labelForwardingAction
  LabelForwardingAction action1(MplsActionCode::PUSH, MplsLabelStack{1001});
  LabelForwardingAction action2(MplsActionCode::PUSH, MplsLabelStack{1002});

  ResolvedNextHop nh1(addr, intfID, 10, action1);
  ResolvedNextHop nh2(addr, intfID, 10, action1);
  ResolvedNextHop nh3(addr, intfID, 10, action2);

  EXPECT_EQ(nh1, nh2);
  EXPECT_NE(nh1, nh3);

  // Test with disableTTLDecrement
  ResolvedNextHop nh4(addr, intfID, 10, std::nullopt, true);
  ResolvedNextHop nh5(addr, intfID, 10, std::nullopt, true);
  ResolvedNextHop nh6(addr, intfID, 10, std::nullopt, false);

  EXPECT_EQ(nh4, nh5);
  EXPECT_NE(nh4, nh6);

  // Test with topologyInfo
  NetworkTopologyInformation topo1;
  topo1.rack_id() = 1;
  topo1.plane_id() = 2;

  NetworkTopologyInformation topo2;
  topo2.rack_id() = 1;
  topo2.plane_id() = 2;

  NetworkTopologyInformation topo3;
  topo3.rack_id() = 2;
  topo3.plane_id() = 2;

  ResolvedNextHop nh7(addr, intfID, 10, std::nullopt, std::nullopt, topo1);
  ResolvedNextHop nh8(addr, intfID, 10, std::nullopt, std::nullopt, topo2);
  ResolvedNextHop nh9(addr, intfID, 10, std::nullopt, std::nullopt, topo3);

  EXPECT_EQ(nh7, nh8);
  EXPECT_NE(nh7, nh9);

  // Test with adjustedWeight
  ResolvedNextHop nh10(
      addr, intfID, 10, std::nullopt, std::nullopt, std::nullopt, 5);
  ResolvedNextHop nh11(
      addr, intfID, 10, std::nullopt, std::nullopt, std::nullopt, 5);
  ResolvedNextHop nh12(
      addr, intfID, 10, std::nullopt, std::nullopt, std::nullopt, 6);

  EXPECT_EQ(nh10, nh11);
  EXPECT_NE(nh10, nh12);
}

TEST(RouteNextHopTest, UnresolvedNextHopEquality) {
  auto addr1 = folly::IPAddress("2401:db00::1");
  auto addr2 = folly::IPAddress("2401:db00::2");

  // Test basic equality
  UnresolvedNextHop nh1(addr1, 10);
  UnresolvedNextHop nh2(addr1, 10);
  EXPECT_EQ(nh1, nh2);

  // Test inequality for different addresses
  UnresolvedNextHop nh3(addr2, 10);
  EXPECT_NE(nh1, nh3);

  // Test inequality for different weights
  UnresolvedNextHop nh4(addr1, 20);
  EXPECT_NE(nh1, nh4);
}

TEST(RouteNextHopTest, UnresolvedNextHopEqualityWithOptionalFields) {
  auto addr = folly::IPAddress("2401:db00::1");

  // Test with labelForwardingAction
  LabelForwardingAction action1(MplsActionCode::PUSH, MplsLabelStack{1001});
  LabelForwardingAction action2(MplsActionCode::PUSH, MplsLabelStack{1002});

  UnresolvedNextHop nh1(addr, 10, action1);
  UnresolvedNextHop nh2(addr, 10, action1);
  UnresolvedNextHop nh3(addr, 10, action2);

  EXPECT_EQ(nh1, nh2);
  EXPECT_NE(nh1, nh3);

  // Test with disableTTLDecrement
  UnresolvedNextHop nh4(addr, 10, std::nullopt, true);
  UnresolvedNextHop nh5(addr, 10, std::nullopt, true);
  UnresolvedNextHop nh6(addr, 10, std::nullopt, false);

  EXPECT_EQ(nh4, nh5);
  EXPECT_NE(nh4, nh6);

  // Test with topologyInfo
  NetworkTopologyInformation topo1;
  topo1.rack_id() = 1;
  topo1.plane_id() = 2;

  NetworkTopologyInformation topo2;
  topo2.rack_id() = 1;
  topo2.plane_id() = 2;

  NetworkTopologyInformation topo3;
  topo3.rack_id() = 2;
  topo3.plane_id() = 2;

  UnresolvedNextHop nh7(addr, 10, std::nullopt, std::nullopt, topo1);
  UnresolvedNextHop nh8(addr, 10, std::nullopt, std::nullopt, topo2);
  UnresolvedNextHop nh9(addr, 10, std::nullopt, std::nullopt, topo3);

  EXPECT_EQ(nh7, nh8);
  EXPECT_NE(nh7, nh9);

  // Test with adjustedWeight
  UnresolvedNextHop nh10(addr, 10, std::nullopt, std::nullopt, std::nullopt, 5);
  UnresolvedNextHop nh11(addr, 10, std::nullopt, std::nullopt, std::nullopt, 5);
  UnresolvedNextHop nh12(addr, 10, std::nullopt, std::nullopt, std::nullopt, 6);

  EXPECT_EQ(nh10, nh11);
  EXPECT_NE(nh10, nh12);
}

TEST(RouteNextHopTest, ResolvedVsUnresolvedInequality) {
  auto addr = folly::IPAddress("2401:db00::1");
  auto intfID = InterfaceID(1);

  ResolvedNextHop resolved(addr, intfID, 10);
  UnresolvedNextHop unresolved(addr, 10);

  // These should be different types, but we test via NextHop polymorphic
  // interface
  NextHop resolvedNh = resolved;
  NextHop unresolvedNh = unresolved;
  EXPECT_NE(resolvedNh, unresolvedNh);
}

// Hash function tests for NetworkTopologyInformation
TEST(RouteNextHopTest, NetworkTopologyInformationHashFunction) {
  std::hash<NetworkTopologyInformation> hasher;

  // Test that equal objects have the same hash
  NetworkTopologyInformation topo1;
  topo1.rack_id() = 1;
  topo1.plane_id() = 2;
  topo1.remote_rack_capacity() = 3;
  topo1.spine_capacity() = 4;
  topo1.local_rack_capacity() = 5;

  NetworkTopologyInformation topo2;
  topo2.rack_id() = 1;
  topo2.plane_id() = 2;
  topo2.remote_rack_capacity() = 3;
  topo2.spine_capacity() = 4;
  topo2.local_rack_capacity() = 5;

  EXPECT_EQ(hasher(topo1), hasher(topo2));

  // Test that different objects have different hashes
  NetworkTopologyInformation topo3;
  topo3.rack_id() = 2; // Different rack_id
  topo3.plane_id() = 2;
  topo3.remote_rack_capacity() = 3;
  topo3.spine_capacity() = 4;
  topo3.local_rack_capacity() = 5;

  EXPECT_NE(hasher(topo1), hasher(topo3));
}

TEST(RouteNextHopTest, NetworkTopologyInformationHashWithOptionalFields) {
  std::hash<NetworkTopologyInformation> hasher;

  // Test with only some fields set
  NetworkTopologyInformation topo1;
  topo1.rack_id() = 1;
  topo1.plane_id() = 2;

  NetworkTopologyInformation topo2;
  topo2.rack_id() = 1;
  topo2.plane_id() = 2;

  EXPECT_EQ(hasher(topo1), hasher(topo2));

  // Test with all fields vs subset
  NetworkTopologyInformation topo3;
  topo3.rack_id() = 1;
  topo3.plane_id() = 2;
  topo3.remote_rack_capacity() = 10;

  EXPECT_NE(hasher(topo1), hasher(topo3));

  // Test empty topology info
  NetworkTopologyInformation topo4;
  NetworkTopologyInformation topo5;
  EXPECT_EQ(hasher(topo4), hasher(topo5));
}

TEST(RouteNextHopTest, NetworkTopologyInformationHashUnorderedSet) {
  std::unordered_set<NetworkTopologyInformation> topoSet;

  NetworkTopologyInformation topo1;
  topo1.rack_id() = 1;
  topo1.plane_id() = 2;

  NetworkTopologyInformation topo2;
  topo2.rack_id() = 2;
  topo2.plane_id() = 1;

  NetworkTopologyInformation topo3;
  topo3.rack_id() = 1;
  topo3.plane_id() = 2;
  topo3.remote_rack_capacity() = 100;

  topoSet.insert(topo1);
  topoSet.insert(topo2);
  topoSet.insert(topo3);

  EXPECT_EQ(topoSet.size(), 3);

  // Try to insert duplicate
  topoSet.insert(topo1);
  EXPECT_EQ(topoSet.size(), 3);

  // Test find operations
  EXPECT_NE(topoSet.find(topo1), topoSet.end());

  NetworkTopologyInformation notFound;
  notFound.rack_id() = 999;
  EXPECT_EQ(topoSet.find(notFound), topoSet.end());
}

// Hash function tests for NextHop
TEST(RouteNextHopTest, NextHopHashFunction) {
  std::hash<NextHop> hasher;

  auto addr1 = folly::IPAddress("2401:db00::1");
  auto addr2 = folly::IPAddress("2401:db00::2");
  auto intfID = InterfaceID(1);

  // Test resolved nexthops
  ResolvedNextHop resolved1(addr1, intfID, 10);
  ResolvedNextHop resolved2(addr1, intfID, 10);

  NextHop nh1 = resolved1;
  NextHop nh2 = resolved2;

  EXPECT_EQ(hasher(nh1), hasher(nh2));

  // Test different addresses
  ResolvedNextHop resolved3(addr2, intfID, 10);
  NextHop nh3 = resolved3;

  EXPECT_NE(hasher(nh1), hasher(nh3));

  // Test unresolved nexthops
  UnresolvedNextHop unresolved1(addr1, 10);
  UnresolvedNextHop unresolved2(addr1, 10);

  NextHop nh4 = unresolved1;
  NextHop nh5 = unresolved2;

  EXPECT_EQ(hasher(nh4), hasher(nh5));

  // Test resolved vs unresolved
  EXPECT_NE(hasher(nh1), hasher(nh4));
}

TEST(RouteNextHopTest, NextHopHashWithComplexFields) {
  std::hash<NextHop> hasher;

  auto addr = folly::IPAddress("2401:db00::1");
  auto intfID = InterfaceID(1);

  // Test with MPLS action
  LabelForwardingAction action1(MplsActionCode::PUSH, MplsLabelStack{1001});
  LabelForwardingAction action2(MplsActionCode::PUSH, MplsLabelStack{1002});

  ResolvedNextHop resolved1(addr, intfID, 10, action1);
  ResolvedNextHop resolved2(addr, intfID, 10, action1);
  ResolvedNextHop resolved3(addr, intfID, 10, action2);

  NextHop nh1 = resolved1;
  NextHop nh2 = resolved2;
  NextHop nh3 = resolved3;

  EXPECT_EQ(hasher(nh1), hasher(nh2));
  EXPECT_NE(hasher(nh1), hasher(nh3));

  // Test with topology info
  NetworkTopologyInformation topo1;
  topo1.rack_id() = 1;
  topo1.plane_id() = 2;

  NetworkTopologyInformation topo2;
  topo2.rack_id() = 2;
  topo2.plane_id() = 1;

  ResolvedNextHop resolved4(
      addr, intfID, 10, std::nullopt, std::nullopt, topo1);
  ResolvedNextHop resolved5(
      addr, intfID, 10, std::nullopt, std::nullopt, topo1);
  ResolvedNextHop resolved6(
      addr, intfID, 10, std::nullopt, std::nullopt, topo2);

  NextHop nh4 = resolved4;
  NextHop nh5 = resolved5;
  NextHop nh6 = resolved6;

  EXPECT_EQ(hasher(nh4), hasher(nh5));
  EXPECT_NE(hasher(nh4), hasher(nh6));
}

TEST(RouteNextHopTest, NextHopHashUnorderedSet) {
  std::unordered_set<NextHop> nhSet;

  auto addr1 = folly::IPAddress("2401:db00::1");
  auto addr2 = folly::IPAddress("2401:db00::2");
  auto intfID = InterfaceID(1);

  ResolvedNextHop resolved1(addr1, intfID, 10);
  ResolvedNextHop resolved2(addr2, intfID, 20);
  UnresolvedNextHop unresolved1(addr1, 30);
  UnresolvedNextHop unresolved2(addr2, 40);

  nhSet.insert(NextHop(resolved1));
  nhSet.insert(NextHop(resolved2));
  nhSet.insert(NextHop(unresolved1));
  nhSet.insert(NextHop(unresolved2));

  EXPECT_EQ(nhSet.size(), 4);

  // Try to insert duplicate
  nhSet.insert(NextHop(resolved1));
  EXPECT_EQ(nhSet.size(), 4);

  // Test find operations
  EXPECT_NE(nhSet.find(NextHop(resolved1)), nhSet.end());
  EXPECT_NE(nhSet.find(NextHop(unresolved1)), nhSet.end());
}

// Hash function tests for ResolvedNextHop
TEST(RouteNextHopTest, ResolvedNextHopHashFunction) {
  std::hash<ResolvedNextHop> hasher;

  auto addr1 = folly::IPAddress("2401:db00::1");
  auto addr2 = folly::IPAddress("2401:db00::2");
  auto intfID1 = InterfaceID(1);
  auto intfID2 = InterfaceID(2);

  // Test basic equality gives same hash
  ResolvedNextHop nh1(addr1, intfID1, 10);
  ResolvedNextHop nh2(addr1, intfID1, 10);

  EXPECT_EQ(hasher(nh1), hasher(nh2));

  // Test different fields give different hashes
  ResolvedNextHop nh3(addr2, intfID1, 10); // Different address
  ResolvedNextHop nh4(addr1, intfID2, 10); // Different interface
  ResolvedNextHop nh5(addr1, intfID1, 20); // Different weight

  EXPECT_NE(hasher(nh1), hasher(nh3));
  EXPECT_NE(hasher(nh1), hasher(nh4));
  EXPECT_NE(hasher(nh1), hasher(nh5));
}

TEST(RouteNextHopTest, ResolvedNextHopHashWithOptionalFields) {
  std::hash<ResolvedNextHop> hasher;

  auto addr = folly::IPAddress("2401:db00::1");
  auto intfID = InterfaceID(1);

  // Test with MPLS action
  LabelForwardingAction action1(MplsActionCode::PUSH, MplsLabelStack{1001});
  LabelForwardingAction action2(MplsActionCode::PUSH, MplsLabelStack{1002});

  ResolvedNextHop nh1(addr, intfID, 10, action1);
  ResolvedNextHop nh2(addr, intfID, 10, action1);
  ResolvedNextHop nh3(addr, intfID, 10, action2);

  EXPECT_EQ(hasher(nh1), hasher(nh2));
  EXPECT_NE(hasher(nh1), hasher(nh3));

  // Test with TTL decrement setting
  ResolvedNextHop nh4(addr, intfID, 10, std::nullopt, true);
  ResolvedNextHop nh5(addr, intfID, 10, std::nullopt, true);
  ResolvedNextHop nh6(addr, intfID, 10, std::nullopt, false);

  EXPECT_EQ(hasher(nh4), hasher(nh5));
  EXPECT_NE(hasher(nh4), hasher(nh6));

  // Test with topology info
  NetworkTopologyInformation topo1;
  topo1.rack_id() = 1;

  NetworkTopologyInformation topo2;
  topo2.rack_id() = 2;

  ResolvedNextHop nh7(addr, intfID, 10, std::nullopt, std::nullopt, topo1);
  ResolvedNextHop nh8(addr, intfID, 10, std::nullopt, std::nullopt, topo1);
  ResolvedNextHop nh9(addr, intfID, 10, std::nullopt, std::nullopt, topo2);

  EXPECT_EQ(hasher(nh7), hasher(nh8));
  EXPECT_NE(hasher(nh7), hasher(nh9));

  // Test with adjusted weight
  ResolvedNextHop nh10(
      addr, intfID, 10, std::nullopt, std::nullopt, std::nullopt, 100);
  ResolvedNextHop nh11(
      addr, intfID, 10, std::nullopt, std::nullopt, std::nullopt, 100);
  ResolvedNextHop nh12(
      addr, intfID, 10, std::nullopt, std::nullopt, std::nullopt, 200);

  EXPECT_EQ(hasher(nh10), hasher(nh11));
  EXPECT_NE(hasher(nh10), hasher(nh12));
}

TEST(RouteNextHopTest, ResolvedNextHopHashConsistency) {
  std::hash<ResolvedNextHop> hasher;

  auto addr = folly::IPAddress("2401:db00::1");
  auto intfID = InterfaceID(1);

  ResolvedNextHop nh(addr, intfID, 10);

  // Test hash consistency across multiple calls
  size_t hash1 = hasher(nh);
  size_t hash2 = hasher(nh);
  size_t hash3 = hasher(nh);

  EXPECT_EQ(hash1, hash2);
  EXPECT_EQ(hash2, hash3);
}

TEST(RouteNextHopTest, ResolvedNextHopHashUnorderedMap) {
  std::unordered_map<ResolvedNextHop, std::string> nhMap;

  auto addr1 = folly::IPAddress("2401:db00::1");
  auto addr2 = folly::IPAddress("2401:db00::2");
  auto intfID = InterfaceID(1);

  ResolvedNextHop nh1(addr1, intfID, 10);
  ResolvedNextHop nh2(addr2, intfID, 20);

  nhMap[nh1] = "nexthop1";
  nhMap[nh2] = "nexthop2";

  EXPECT_EQ(nhMap.size(), 2);
  EXPECT_EQ(nhMap[nh1], "nexthop1");
  EXPECT_EQ(nhMap[nh2], "nexthop2");

  // Test with duplicate key
  nhMap[nh1] = "updated_nexthop1";
  EXPECT_EQ(nhMap.size(), 2);
  EXPECT_EQ(nhMap[nh1], "updated_nexthop1");
}

TEST(RouteNextHopTest, HashFunctionCompleteness) {
  // This test ensures that all fields are considered in the hash function
  // by creating two objects that differ only in one field at a time
  std::hash<ResolvedNextHop> hasher;

  auto addr1 = folly::IPAddress("10.0.0.1");
  auto addr2 = folly::IPAddress("10.0.0.2");
  auto intfID1 = InterfaceID(1);
  auto intfID2 = InterfaceID(2);

  ResolvedNextHop base(addr1, intfID1, 10);

  // Different address
  ResolvedNextHop diffAddr(addr2, intfID1, 10);
  EXPECT_NE(hasher(base), hasher(diffAddr));

  // Different interface
  ResolvedNextHop diffIntf(addr1, intfID2, 10);
  EXPECT_NE(hasher(base), hasher(diffIntf));

  // Different weight
  ResolvedNextHop diffWeight(addr1, intfID1, 20);
  EXPECT_NE(hasher(base), hasher(diffWeight));

  // Different MPLS action
  LabelForwardingAction action(MplsActionCode::PUSH, MplsLabelStack{1001});
  ResolvedNextHop diffAction(addr1, intfID1, 10, action);
  EXPECT_NE(hasher(base), hasher(diffAction));

  // Different TTL decrement
  ResolvedNextHop diffTTL(addr1, intfID1, 10, std::nullopt, true);
  EXPECT_NE(hasher(base), hasher(diffTTL));

  // Different topology info
  NetworkTopologyInformation topo;
  topo.rack_id() = 42;
  ResolvedNextHop diffTopo(
      addr1, intfID1, 10, std::nullopt, std::nullopt, topo);
  EXPECT_NE(hasher(base), hasher(diffTopo));

  // Different adjusted weight
  ResolvedNextHop diffAdjWeight(
      addr1, intfID1, 10, std::nullopt, std::nullopt, std::nullopt, 50);
  EXPECT_NE(hasher(base), hasher(diffAdjWeight));
}
