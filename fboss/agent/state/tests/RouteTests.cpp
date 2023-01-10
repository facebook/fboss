/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/AddressUtil.h"
#include "fboss/agent/ApplyThriftConfig.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/mock/MockPlatform.h"
#include "fboss/agent/state/DeltaFunctions.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/InterfaceMap.h"

#include "fboss/agent/state/NodeMapDelta.h"
#include "fboss/agent/state/Route.h"
#include "fboss/agent/state/RouteNextHopEntry.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/StateUtils.h"
#include "fboss/agent/state/SwitchState-defs.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/TestUtils.h"

#include <folly/logging/xlog.h>
#include <gtest/gtest.h>
#include <optional>

using namespace facebook::fboss;
using folly::IPAddress;
using folly::IPAddressV4;
using folly::IPAddressV6;
using std::make_shared;
using std::shared_ptr;
using ::testing::Return;

//
// Helper functions
//

//
// Tests
//
#define CLIENT_A ClientID(1001)
#define CLIENT_B ClientID(1002)
#define CLIENT_C ClientID(1003)
#define CLIENT_D ClientID(1004)
#define CLIENT_E ClientID(1005)

constexpr AdminDistance DISTANCE = AdminDistance::MAX_ADMIN_DISTANCE;

// Utility function for creating a nexthops list of size n,
// starting with the prefix.  For prefix "1.1.1.", first
// IP in the list will be 1.1.1.10
RouteNextHopSet newNextHops(int n, std::string prefix) {
  RouteNextHopSet h;
  for (int i = 0; i < n; i++) {
    auto ipStr = prefix + std::to_string(i + 10);
    h.emplace(UnresolvedNextHop(IPAddress(ipStr), UCMP_DEFAULT_WEIGHT));
  }
  return h;
}

// Test equality of RouteNextHopsMulti.
TEST(Route, equality) {
  // Create two identical RouteNextHopsMulti, and compare
  RouteNextHopsMulti nhm1;
  nhm1.update(CLIENT_A, RouteNextHopEntry(newNextHops(3, "1.1.1."), DISTANCE));
  nhm1.update(CLIENT_B, RouteNextHopEntry(newNextHops(3, "2.2.2."), DISTANCE));

  RouteNextHopsMulti nhm2;
  nhm2.update(CLIENT_A, RouteNextHopEntry(newNextHops(3, "1.1.1."), DISTANCE));
  nhm2.update(CLIENT_B, RouteNextHopEntry(newNextHops(3, "2.2.2."), DISTANCE));

  EXPECT_TRUE(nhm1 == nhm2);

  // Delete data for CLIENT_C.  But there wasn't any.  Two objs still equal
  nhm1.delEntryForClient(CLIENT_C);
  EXPECT_TRUE(nhm1 == nhm2);

  // Delete obj1's CLIENT_B.  Now, objs should be NOT equal
  nhm1.delEntryForClient(CLIENT_B);
  EXPECT_FALSE(nhm1 == nhm2);

  // Now replace obj1's CLIENT_B list with a shorter list.
  // Objs should be NOT equal.
  nhm1.update(CLIENT_B, RouteNextHopEntry(newNextHops(2, "2.2.2."), DISTANCE));
  EXPECT_FALSE(nhm1 == nhm2);

  // Now replace obj1's CLIENT_B list with the original list.
  // But construct the list in opposite order.
  // Objects should still be equal, despite the order of construction.
  RouteNextHopSet nextHopsRev;
  nextHopsRev.emplace(
      UnresolvedNextHop(IPAddress("2.2.2.12"), UCMP_DEFAULT_WEIGHT));
  nextHopsRev.emplace(
      UnresolvedNextHop(IPAddress("2.2.2.11"), UCMP_DEFAULT_WEIGHT));
  nextHopsRev.emplace(
      UnresolvedNextHop(IPAddress("2.2.2.10"), UCMP_DEFAULT_WEIGHT));
  nhm1.update(CLIENT_B, RouteNextHopEntry(nextHopsRev, DISTANCE));
  EXPECT_TRUE(nhm1 == nhm2);
}

// Test that a copy of a RouteNextHopsMulti is a deep copy, and that the
// resulting objects can be modified independently.
TEST(Route, deepCopy) {
  // Create two identical RouteNextHopsMulti, and compare
  RouteNextHopsMulti nhm1;
  auto origHops = newNextHops(3, "1.1.1.");
  nhm1.update(CLIENT_A, RouteNextHopEntry(origHops, DISTANCE));
  nhm1.update(CLIENT_B, RouteNextHopEntry(newNextHops(3, "2.2.2."), DISTANCE));

  // Copy it
  RouteNextHopsMulti nhm2(nhm1.toThrift());

  // The two should be identical
  EXPECT_TRUE(nhm1 == nhm2);

  // Now modify the underlying nexthop list.
  // Should be changed in nhm1, but not nhm2.
  auto newHops = newNextHops(4, "10.10.10.");
  nhm1.update(CLIENT_A, RouteNextHopEntry(newHops, DISTANCE));

  EXPECT_FALSE(nhm1 == nhm2);

  EXPECT_TRUE(nhm1.isSame(CLIENT_A, RouteNextHopEntry(newHops, DISTANCE)));
  EXPECT_TRUE(nhm2.isSame(CLIENT_A, RouteNextHopEntry(origHops, DISTANCE)));
}

// Test serialization of RouteNextHopsMulti.
TEST(Route, serializeRouteNextHopsMulti) {
  // This function tests [de]serialization of:
  // RouteNextHopsMulti
  // RouteNextHopEntry
  // NextHop

  // test for new format to new format
  RouteNextHopsMulti nhm1;
  nhm1.update(CLIENT_A, RouteNextHopEntry(newNextHops(3, "1.1.1."), DISTANCE));
  nhm1.update(CLIENT_B, RouteNextHopEntry(newNextHops(1, "2.2.2."), DISTANCE));
  nhm1.update(CLIENT_C, RouteNextHopEntry(newNextHops(4, "3.3.3."), DISTANCE));
  nhm1.update(CLIENT_D, RouteNextHopEntry(RouteForwardAction::DROP, DISTANCE));
  nhm1.update(
      CLIENT_E, RouteNextHopEntry(RouteForwardAction::TO_CPU, DISTANCE));

  auto serialized = nhm1.toFollyDynamicLegacy();

  auto nhm2 = LegacyRouteNextHopsMulti::fromFollyDynamicLegacy(serialized);

  EXPECT_TRUE(nhm1 == *nhm2);
}

TEST(Route, RouteNextHopsMultiThrift) {
  RouteNextHopsMulti nhm1;
  nhm1.update(CLIENT_A, RouteNextHopEntry(newNextHops(3, "1.1.1."), DISTANCE));
  nhm1.update(CLIENT_B, RouteNextHopEntry(newNextHops(1, "2.2.2."), DISTANCE));
  nhm1.update(CLIENT_C, RouteNextHopEntry(newNextHops(4, "3.3.3."), DISTANCE));
  nhm1.update(CLIENT_D, RouteNextHopEntry(RouteForwardAction::DROP, DISTANCE));
  nhm1.update(
      CLIENT_E, RouteNextHopEntry(RouteForwardAction::TO_CPU, DISTANCE));
  nhm1.update(
      CLIENT_A,
      RouteNextHopEntry(
          newNextHops(4, "4.4.1."),
          DISTANCE,
          std::optional<RouteCounterID>(RouteCounterID("testcounter0")),
          std::optional<cfg::AclLookupClass>(
              cfg::AclLookupClass::DST_CLASS_L3_DPR)));
  nhm1.update(
      CLIENT_A,
      RouteNextHopEntry(
          newNextHops(4, "4.4.2."),
          DISTANCE,
          std::optional<RouteCounterID>(RouteCounterID("testcounter1"))));
  nhm1.update(
      CLIENT_A,
      RouteNextHopEntry(
          newNextHops(4, "4.4.3."),
          DISTANCE,
          std::optional<RouteCounterID>(std::nullopt),
          std::optional<cfg::AclLookupClass>(
              cfg::AclLookupClass::DST_CLASS_L3_DPR)));
  validateThriftStructNodeSerialization<RouteNextHopsMulti>(nhm1);
}

// Test priority ranking of nexthop lists within a RouteNextHopsMulti.
TEST(Route, listRanking) {
  auto list00 = newNextHops(3, "0.0.0.");
  auto list07 = newNextHops(3, "7.7.7.");
  auto list01 = newNextHops(3, "1.1.1.");
  auto list20 = newNextHops(3, "20.20.20.");
  auto list30 = newNextHops(3, "30.30.30.");

  RouteNextHopsMulti nhm;
  nhm.update(ClientID(20), RouteNextHopEntry(list20, AdminDistance::EBGP));
  nhm.update(
      ClientID(1), RouteNextHopEntry(list01, AdminDistance::STATIC_ROUTE));
  nhm.update(
      ClientID(30),
      RouteNextHopEntry(list30, AdminDistance::MAX_ADMIN_DISTANCE));
  EXPECT_TRUE(nhm.getBestEntry().second->getNextHopSet() == list01);

  nhm.update(
      ClientID(10),
      RouteNextHopEntry(list00, AdminDistance::DIRECTLY_CONNECTED));
  EXPECT_TRUE(nhm.getBestEntry().second->getNextHopSet() == list00);

  nhm.delEntryForClient(ClientID(10));
  EXPECT_TRUE(nhm.getBestEntry().second->getNextHopSet() == list01);

  nhm.delEntryForClient(ClientID(30));
  EXPECT_TRUE(nhm.getBestEntry().second->getNextHopSet() == list01);

  nhm.delEntryForClient(ClientID(1));
  EXPECT_TRUE(nhm.getBestEntry().second->getNextHopSet() == list20);

  nhm.delEntryForClient(ClientID(20));
  EXPECT_THROW(nhm.getBestEntry().second->getNextHopSet(), FbossError);
}

// Very basic test for serialization/deseralization of Routes
TEST(Route, serializeRoute) {
  ClientID clientId = ClientID(1);
  auto nxtHops = makeNextHops({"10.10.10.10", "11.11.11.11"});
  Route<IPAddressV4> rt(
      Route<IPAddressV4>::makeThrift(makePrefixV4("1.2.3.4/32")));
  rt.update(clientId, RouteNextHopEntry(nxtHops, DISTANCE));
  validateNodeSerialization(rt);

  // to folly dynamic
  folly::dynamic obj = rt.toFollyDynamic();
  // to string
  folly::json::serialization_opts serOpts;
  serOpts.allow_non_string_keys = true;
  std::string json = folly::json::serialize(obj, serOpts);
  // back to folly dynamic
  folly::dynamic obj2 = folly::parseJson(json, serOpts);
  // back to Route object
  auto rt2 = Route<IPAddressV4>::fromFollyDynamic(obj2);
  ASSERT_TRUE(rt2->has(clientId, RouteNextHopEntry(nxtHops, DISTANCE)));
  validateNodeSerialization(*rt2);
}

TEST(Route, serializeMplsRoute) {
  ClientID clientId = ClientID(1);
  auto nxtHops = makeNextHops(
      {"10.10.10.10", "11.11.11.11"},
      LabelForwardingAction(LabelForwardingAction::LabelForwardingType::PHP));
  Route<LabelID> rt(Route<LabelID>::makeThrift(LabelID(100)));
  rt.update(clientId, RouteNextHopEntry(nxtHops, DISTANCE));
  validateNodeSerialization(rt);

  // to folly dynamic
  folly::dynamic obj = rt.toFollyDynamic();
  // to string
  folly::json::serialization_opts serOpts;
  serOpts.allow_non_string_keys = true;
  std::string json = folly::json::serialize(obj, serOpts);
  // back to folly dynamic
  folly::dynamic obj2 = folly::parseJson(json, serOpts);
  // back to Route object
  auto rt2 = Route<LabelID>::fromFollyDynamic(obj2);
  ASSERT_TRUE(rt2->has(clientId, RouteNextHopEntry(nxtHops, DISTANCE)));
  EXPECT_EQ(int32_t(rt2->getID().label()), 100);
  validateNodeSerialization(*rt2);
}

// Serialization/deseralization of Routes with counterID
TEST(Route, serializeRouteCounterID) {
  ClientID clientId = ClientID(1);
  auto nxtHops = makeNextHops({"10.10.10.10", "11.11.11.11"});
  std::optional<RouteCounterID> counterID("route.counter.0");
  Route<IPAddressV4> rt(Route<IPAddressV4>::makeThrift(
      makePrefixV4("1.2.3.4/32"),
      clientId,
      RouteNextHopEntry(nxtHops, DISTANCE, counterID)));
  rt.setResolved(RouteNextHopEntry(nxtHops, DISTANCE, counterID));
  validateNodeSerialization(rt);

  // to folly dynamic
  folly::dynamic obj = rt.toFollyDynamic();
  // to string
  folly::json::serialization_opts serOpts;
  serOpts.allow_non_string_keys = true;
  std::string json = folly::json::serialize(obj, serOpts);
  // back to folly dynamic
  folly::dynamic obj2 = folly::parseJson(json, serOpts);
  // back to Route object
  auto rt2 = Route<IPAddressV4>::fromFollyDynamic(obj2);
  EXPECT_EQ(*(rt2->getEntryForClient(clientId)->getCounterID()), counterID);
  EXPECT_EQ(rt2->getForwardInfo().getCounterID(), counterID);
  validateNodeSerialization(*rt2);
}

// Serialization/deseralization of Routes with counterID
TEST(Route, serializeRouteClassID) {
  ClientID clientId = ClientID(1);
  auto nxtHops = makeNextHops({"10.10.10.10", "11.11.11.11"});
  std::optional<cfg::AclLookupClass> classID(
      cfg::AclLookupClass::DST_CLASS_L3_DPR);
  Route<IPAddressV4> rt(Route<IPAddressV4>::makeThrift(
      makePrefixV4("1.2.3.4/32"),
      clientId,
      RouteNextHopEntry(
          nxtHops, DISTANCE, std::optional<RouteCounterID>(), classID)));
  rt.setResolved(RouteNextHopEntry(
      nxtHops, DISTANCE, std::optional<RouteCounterID>(), classID));
  validateNodeSerialization(rt);

  // to folly dynamic
  folly::dynamic obj = rt.toFollyDynamic();
  // to string
  folly::json::serialization_opts serOpts;
  serOpts.allow_non_string_keys = true;
  std::string json = folly::json::serialize(obj, serOpts);
  // back to folly dynamic
  folly::dynamic obj2 = folly::parseJson(json, serOpts);
  // back to Route object
  auto rt2 = Route<IPAddressV4>::fromFollyDynamic(obj2);
  EXPECT_EQ(*(rt2->getEntryForClient(clientId)->getClassID()), classID);
  EXPECT_EQ(rt2->getForwardInfo().getClassID(), classID);
  validateNodeSerialization(*rt2);
}

// Test utility functions for converting RouteNextHopSet to thrift and back
TEST(RouteTypes, toFromRouteNextHops) {
  RouteNextHopSet nhs;
  // Non v4 link-local address without interface scoping
  nhs.emplace(
      UnresolvedNextHop(folly::IPAddress("10.0.0.1"), UCMP_DEFAULT_WEIGHT));

  // v4 link-local address without interface scoping
  nhs.emplace(
      UnresolvedNextHop(folly::IPAddress("169.254.0.1"), UCMP_DEFAULT_WEIGHT));

  // Non v6 link-local address without interface scoping
  nhs.emplace(
      UnresolvedNextHop(folly::IPAddress("face:b00c::1"), UCMP_DEFAULT_WEIGHT));

  // v6 link-local address with interface scoping
  nhs.emplace(ResolvedNextHop(
      folly::IPAddress("fe80::1"), InterfaceID(4), UCMP_DEFAULT_WEIGHT));

  // v6 link-local address without interface scoping
  EXPECT_THROW(
      nhs.emplace(UnresolvedNextHop(folly::IPAddress("fe80::1"), 42)),
      FbossError);

  // Convert to thrift object
  auto nhts = util::fromRouteNextHopSet(nhs);
  ASSERT_EQ(4, nhts.size());

  auto verify = [&](const std::string& ipaddr,
                    std::optional<InterfaceID> intf) {
    auto bAddr = facebook::network::toBinaryAddress(folly::IPAddress(ipaddr));
    if (intf.has_value()) {
      bAddr.ifName() = util::createTunIntfName(intf.value());
    }
    bool found = false;
    for (const auto& entry : nhts) {
      if (*entry.address() == bAddr) {
        if (intf.has_value()) {
          EXPECT_TRUE(entry.address()->ifName());
          EXPECT_EQ(*bAddr.ifName(), *entry.address()->ifName());
        }
        found = true;
        break;
      }
    }
    XLOG(DBG2) << "**** " << ipaddr;
    EXPECT_TRUE(found);
  };

  verify("10.0.0.1", std::nullopt);
  verify("169.254.0.1", std::nullopt);
  verify("face:b00c::1", std::nullopt);
  verify("fe80::1", InterfaceID(4));

  // Convert back to RouteNextHopSet
  auto newNhs = util::toRouteNextHopSet(nhts);
  EXPECT_EQ(nhs, newNhs);

  //
  // Some ignore cases
  //

  facebook::network::thrift::BinaryAddress addr;

  addr = facebook::network::toBinaryAddress(folly::IPAddress("10.0.0.1"));
  addr.ifName() = "fboss10";
  NextHopThrift nht;
  *nht.address() = addr;
  {
    NextHop nh = util::fromThrift(nht);
    EXPECT_EQ(folly::IPAddress("10.0.0.1"), nh.addr());
    EXPECT_EQ(std::nullopt, nh.intfID());
  }

  addr = facebook::network::toBinaryAddress(folly::IPAddress("face::1"));
  addr.ifName() = "fboss10";
  *nht.address() = addr;
  {
    NextHop nh = util::fromThrift(nht);
    EXPECT_EQ(folly::IPAddress("face::1"), nh.addr());
    EXPECT_EQ(std::nullopt, nh.intfID());
  }

  addr = facebook::network::toBinaryAddress(folly::IPAddress("fe80::1"));
  addr.ifName() = "fboss10";
  *nht.address() = addr;
  {
    NextHop nh = util::fromThrift(nht);
    EXPECT_EQ(folly::IPAddress("fe80::1"), nh.addr());
    EXPECT_EQ(InterfaceID(10), nh.intfID());
  }
}

TEST(Route, nextHopTest) {
  folly::IPAddress addr("1.1.1.1");
  NextHop unh = UnresolvedNextHop(addr, 0);
  NextHop rnh = ResolvedNextHop(addr, InterfaceID(1), 0);
  EXPECT_FALSE(unh.isResolved());
  EXPECT_TRUE(rnh.isResolved());
  EXPECT_EQ(unh.addr(), addr);
  EXPECT_EQ(rnh.addr(), addr);
  EXPECT_THROW(unh.intf(), std::bad_optional_access);
  EXPECT_EQ(rnh.intf(), InterfaceID(1));
  EXPECT_NE(unh, rnh);
  auto unh2 = unh;
  EXPECT_EQ(unh, unh2);
  auto rnh2 = rnh;
  EXPECT_EQ(rnh, rnh2);
  EXPECT_FALSE(rnh < unh && unh < rnh);
  EXPECT_TRUE(rnh < unh || unh < rnh);
  EXPECT_TRUE(unh < rnh && rnh > unh);
}

TEST(Route, nexthopFromThriftAndDynamic) {
  IPAddressV6 ip{"fe80::1"};
  NextHopThrift nexthop;
  nexthop.address()->addr()->append(
      reinterpret_cast<const char*>(ip.bytes()),
      folly::IPAddressV6::byteCount());
  nexthop.address()->ifName() = "fboss100";
  MplsAction action;
  *action.action() = MplsActionCode::PUSH;
  action.pushLabels() = MplsLabelStack{501, 502, 503};
  EXPECT_EQ(util::fromThrift(nexthop).toThrift(), nexthop);
  EXPECT_EQ(
      util::nextHopFromFollyDynamic(util::fromThrift(nexthop).toFollyDynamic()),
      util::fromThrift(nexthop));
}

TEST(RoutePrefix, Thrift) {
  RouteV4::Prefix prefix10{IPAddressV4("10.10.10.10"), 32};
  RouteV6::Prefix prefix20{IPAddressV6("1::10"), 64};
  RouteV6::Prefix prefix30{IPAddressV6("2001::1"), 128};
  validateNodeSerialization<RouteV4::Prefix, true>(prefix10);
  validateNodeSerialization<RouteV6::Prefix, true>(prefix20);
  validateNodeSerialization<RouteV6::Prefix, true>(prefix30);
}

TEST(RouteNextHopEntry, toUnicastRouteDrop) {
  folly::CIDRNetwork nw{folly::IPAddress{"1::1"}, 64};
  auto unicastRoute = util::toUnicastRoute(
      nw, RouteNextHopEntry(RouteForwardAction::DROP, DISTANCE));

  EXPECT_EQ(
      nw,
      folly::CIDRNetwork(
          facebook::network::toIPAddress(*unicastRoute.dest()->ip()),
          *unicastRoute.dest()->prefixLength()));
  EXPECT_EQ(RouteForwardAction::DROP, unicastRoute.action());
  EXPECT_EQ(0, unicastRoute.nextHops()->size());
}

TEST(RouteNextHopEntry, toUnicastRouteCPU) {
  folly::CIDRNetwork nw{folly::IPAddress{"1.1.1.1"}, 24};
  auto unicastRoute = util::toUnicastRoute(
      nw, RouteNextHopEntry(RouteForwardAction::TO_CPU, DISTANCE));

  EXPECT_EQ(
      nw,
      folly::CIDRNetwork(
          facebook::network::toIPAddress(*unicastRoute.dest()->ip()),
          *unicastRoute.dest()->prefixLength()));
  EXPECT_EQ(RouteForwardAction::TO_CPU, unicastRoute.action());
  EXPECT_EQ(0, unicastRoute.nextHops()->size());
}

TEST(RouteNextHopEntry, toUnicastRouteNhopsNonEcmp) {
  folly::CIDRNetwork nw{folly::IPAddress{"1::1"}, 64};
  RouteNextHopSet nhops = makeNextHops({"1.1.1.10"});
  auto unicastRoute =
      util::toUnicastRoute(nw, RouteNextHopEntry(nhops, DISTANCE));

  EXPECT_EQ(
      nw,
      folly::CIDRNetwork(
          facebook::network::toIPAddress(*unicastRoute.dest()->ip()),
          *unicastRoute.dest()->prefixLength()));
  EXPECT_EQ(RouteForwardAction::NEXTHOPS, unicastRoute.action());
  EXPECT_EQ(1, unicastRoute.nextHops()->size());
}

TEST(RouteNextHopEntry, toUnicastRouteNhopsEcmp) {
  folly::CIDRNetwork nw{folly::IPAddress{"1::1"}, 64};
  RouteNextHopSet nhops = makeNextHops({"1.1.1.10", "2::2"});
  auto unicastRoute =
      util::toUnicastRoute(nw, RouteNextHopEntry(nhops, DISTANCE));

  EXPECT_EQ(
      nw,
      folly::CIDRNetwork(
          facebook::network::toIPAddress(*unicastRoute.dest()->ip()),
          *unicastRoute.dest()->prefixLength()));
  EXPECT_EQ(RouteForwardAction::NEXTHOPS, unicastRoute.action());
  EXPECT_EQ(2, unicastRoute.nextHops()->size());
}
