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
