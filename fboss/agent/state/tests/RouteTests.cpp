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
#include "fboss/agent/state/NodeMapDelta-defs.h"
#include "fboss/agent/state/NodeMapDelta.h"
#include "fboss/agent/state/Route.h"
#include "fboss/agent/state/RouteDelta.h"
#include "fboss/agent/state/RouteTable.h"
#include "fboss/agent/state/RouteTableMap.h"
#include "fboss/agent/state/RouteTableRib.h"
#include "fboss/agent/state/RouteUpdater.h"
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

namespace {
const auto kDestPrefix =
    RouteV6::Prefix{folly::IPAddressV6("2401:bad:cad:dad::"), 64};
const auto kDestAddress = folly::IPAddressV6("2401:bad:cad:dad::beef");

// bgp next hops
const std::array<folly::IPAddressV6, 4> kBgpNextHopAddrs{
    folly::IPAddressV6("2801::1"),
    folly::IPAddressV6("2802::1"),
    folly::IPAddressV6("2803::1"),
    folly::IPAddressV6("2804::1"),
};

// igp next hops
const std::array<folly::IPAddressV6, 4> kIgpAddrs{
    folly::IPAddressV6("fe80::101"),
    folly::IPAddressV6("fe80::102"),
    folly::IPAddressV6("fe80::103"),
    folly::IPAddressV6("fe80::104"),
};

// label stacks
const std::array<LabelForwardingAction::LabelStack, 4> kLabelStacks{
    LabelForwardingAction::LabelStack({101, 201, 301}),
    LabelForwardingAction::LabelStack({102, 202, 302}),
    LabelForwardingAction::LabelStack({103, 203, 303}),
    LabelForwardingAction::LabelStack({104, 204, 304}),
};
const std::array<InterfaceID, 4> kInterfaces{
    InterfaceID(1),
    InterfaceID(2),
    InterfaceID(3),
    InterfaceID(4),
};
} // namespace
//
// Helper functions
//
template <typename AddrT>
void EXPECT_FWD_INFO(
    std::shared_ptr<Route<AddrT>> rt,
    InterfaceID intf,
    std::string ipStr) {
  const auto& fwds = rt->getForwardInfo().getNextHopSet();
  EXPECT_EQ(1, fwds.size());
  const auto& fwd = *fwds.begin();
  EXPECT_EQ(intf, fwd.intf());
  EXPECT_EQ(IPAddress(ipStr), fwd.addr());
}

template <typename AddrT>
void EXPECT_RESOLVED(std::shared_ptr<Route<AddrT>> rt) {
  ASSERT_NE(nullptr, rt);
  EXPECT_TRUE(rt->isResolved());
  EXPECT_FALSE(rt->isUnresolvable());
  EXPECT_FALSE(rt->needResolve());
}

template <typename AddrT>
void EXPECT_NODEMAP_MATCH(const std::shared_ptr<RouteTableRib<AddrT>>& rib) {
  const auto& radixTree = rib->routesRadixTree();
  EXPECT_EQ(rib->size(), radixTree.size());
  for (const auto& route : *(rib->routes())) {
    auto match =
        radixTree.exactMatch(route->prefix().network, route->prefix().mask);
    ASSERT_NE(match, radixTree.end());
    // should be the same shared_ptr
    EXPECT_EQ(route, match->value());
  }
}

void EXPECT_NODEMAP_MATCH(const std::shared_ptr<RouteTableMap>& routeTables) {
  for (const auto& rt : *routeTables) {
    if (rt->getRibV4()) {
      EXPECT_NODEMAP_MATCH<IPAddressV4>(rt->getRibV4());
    }
    if (rt->getRibV6()) {
      EXPECT_NODEMAP_MATCH<IPAddressV6>(rt->getRibV6());
    }
  }
}

template <typename AddrT>
void EXPECT_ROUTETABLERIB_MATCH(
    const std::shared_ptr<RouteTableRib<AddrT>>& rib1,
    const std::shared_ptr<RouteTableRib<AddrT>>& rib2) {
  EXPECT_EQ(rib1->size(), rib2->size());
  EXPECT_EQ(rib1->routesRadixTree().size(), rib2->routesRadixTree().size());
  for (const auto& route : *(rib1->routes())) {
    auto match = rib2->exactMatch(route->prefix());
    ASSERT_NE(nullptr, match);
    EXPECT_TRUE(match->isSame(route.get()));
  }
}

//
// Tests
//
#define CLIENT_A ClientID(1001)
#define CLIENT_B ClientID(1002)
#define CLIENT_C ClientID(1003)
#define CLIENT_D ClientID(1004)
#define CLIENT_E ClientID(1005)

constexpr AdminDistance DISTANCE = AdminDistance::MAX_ADMIN_DISTANCE;

std::shared_ptr<SwitchState> applyInitConfig() {
  auto platform = createMockPlatform();
  auto stateV0 = make_shared<SwitchState>();
  auto tablesV0 = stateV0->getRouteTables();

  cfg::SwitchConfig config;
  config.vlans_ref()->resize(4);
  *config.vlans_ref()[0].id_ref() = 1;
  *config.vlans_ref()[1].id_ref() = 2;
  *config.vlans_ref()[2].id_ref() = 3;
  *config.vlans_ref()[3].id_ref() = 4;

  config.interfaces_ref()->resize(4);
  *config.interfaces_ref()[0].intfID_ref() = 1;
  *config.interfaces_ref()[0].vlanID_ref() = 1;
  *config.interfaces_ref()[0].routerID_ref() = 0;
  config.interfaces_ref()[0].mac_ref() = "00:00:00:00:00:11";
  config.interfaces_ref()[0].ipAddresses_ref()->resize(2);
  config.interfaces_ref()[0].ipAddresses_ref()[0] = "1.1.1.1/24";
  config.interfaces_ref()[0].ipAddresses_ref()[1] = "1::1/48";

  *config.interfaces_ref()[1].intfID_ref() = 2;
  *config.interfaces_ref()[1].vlanID_ref() = 2;
  *config.interfaces_ref()[1].routerID_ref() = 0;
  config.interfaces_ref()[1].mac_ref() = "00:00:00:00:00:22";
  config.interfaces_ref()[1].ipAddresses_ref()->resize(2);
  config.interfaces_ref()[1].ipAddresses_ref()[0] = "2.2.2.2/24";
  config.interfaces_ref()[1].ipAddresses_ref()[1] = "2::1/48";

  *config.interfaces_ref()[2].intfID_ref() = 3;
  *config.interfaces_ref()[2].vlanID_ref() = 3;
  *config.interfaces_ref()[2].routerID_ref() = 0;
  config.interfaces_ref()[2].mac_ref() = "00:00:00:00:00:33";
  config.interfaces_ref()[2].ipAddresses_ref()->resize(2);
  config.interfaces_ref()[2].ipAddresses_ref()[0] = "3.3.3.3/24";
  config.interfaces_ref()[2].ipAddresses_ref()[1] = "3::1/48";

  *config.interfaces_ref()[3].intfID_ref() = 4;
  *config.interfaces_ref()[3].vlanID_ref() = 4;
  *config.interfaces_ref()[3].routerID_ref() = 0;
  config.interfaces_ref()[3].mac_ref() = "00:00:00:00:00:44";
  config.interfaces_ref()[3].ipAddresses_ref()->resize(2);
  config.interfaces_ref()[3].ipAddresses_ref()[0] = "4.4.4.4/24";
  config.interfaces_ref()[3].ipAddresses_ref()[1] = "4::1/48";

  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  stateV1->publish();
  return stateV1;
}

namespace TEMP {
struct Route {
  uint32_t vrf;
  IPAddress prefix;
  uint8_t len;
  Route(uint32_t vrf, IPAddress prefix, uint8_t len)
      : vrf(vrf), prefix(prefix), len(len) {}
  bool operator<(const Route& rt) const {
    if (vrf < rt.vrf) {
      return true;
    } else if (vrf > rt.vrf) {
      return false;
    }
    if (len < rt.len) {
      return true;
    } else if (len > rt.len) {
      return false;
    }
    return prefix < rt.prefix;
  }
  bool operator==(const Route& rt) const {
    return vrf == rt.vrf && len == rt.len && prefix == rt.prefix;
  }
};
} // namespace TEMP

void checkChangedRoute(
    const shared_ptr<RouteTableMap>& oldTables,
    const shared_ptr<RouteTableMap>& newTables,
    const std::set<TEMP::Route> changedIDs,
    const std::set<TEMP::Route> addedIDs,
    const std::set<TEMP::Route> removedIDs) {
  auto oldState = make_shared<SwitchState>();
  oldState->resetRouteTables(oldTables);
  auto newState = make_shared<SwitchState>();
  newState->resetRouteTables(newTables);

  std::set<TEMP::Route> foundChanged;
  std::set<TEMP::Route> foundAdded;
  std::set<TEMP::Route> foundRemoved;
  StateDelta delta(oldState, newState);

  for (auto const& rtDelta : delta.getRouteTablesDelta()) {
    RouterID id;
    if (!rtDelta.getOld()) {
      id = rtDelta.getNew()->getID();
    } else {
      id = rtDelta.getOld()->getID();
    }
    DeltaFunctions::forEachChanged(
        rtDelta.getRoutesV4Delta(),
        [&](const shared_ptr<RouteV4>& oldRt,
            const shared_ptr<RouteV4>& newRt) {
          EXPECT_EQ(oldRt->prefix(), newRt->prefix());
          EXPECT_NE(oldRt, newRt);
          const auto prefix = newRt->prefix();
          auto ret = foundChanged.insert(
              TEMP::Route(id, IPAddress(prefix.network), prefix.mask));
          EXPECT_TRUE(ret.second);
        },
        [&](const shared_ptr<RouteV4>& rt) {
          const auto prefix = rt->prefix();
          auto ret = foundAdded.insert(
              TEMP::Route(id, IPAddress(prefix.network), prefix.mask));
          EXPECT_TRUE(ret.second);
        },
        [&](const shared_ptr<RouteV4>& rt) {
          const auto prefix = rt->prefix();
          auto ret = foundRemoved.insert(
              TEMP::Route(id, IPAddress(prefix.network), prefix.mask));
          EXPECT_TRUE(ret.second);
        });
    DeltaFunctions::forEachChanged(
        rtDelta.getRoutesV6Delta(),
        [&](const shared_ptr<RouteV6>& oldRt,
            const shared_ptr<RouteV6>& newRt) {
          EXPECT_EQ(oldRt->prefix(), newRt->prefix());
          EXPECT_NE(oldRt, newRt);
          const auto prefix = newRt->prefix();
          auto ret = foundChanged.insert(
              TEMP::Route(id, IPAddress(prefix.network), prefix.mask));
          EXPECT_TRUE(ret.second);
        },
        [&](const shared_ptr<RouteV6>& rt) {
          const auto prefix = rt->prefix();
          auto ret = foundAdded.insert(
              TEMP::Route(id, IPAddress(prefix.network), prefix.mask));
          EXPECT_TRUE(ret.second);
        },
        [&](const shared_ptr<RouteV6>& rt) {
          const auto prefix = rt->prefix();
          auto ret = foundRemoved.insert(
              TEMP::Route(id, IPAddress(prefix.network), prefix.mask));
          EXPECT_TRUE(ret.second);
        });
  }

  EXPECT_EQ(changedIDs, foundChanged);
  EXPECT_EQ(addedIDs, foundAdded);
  EXPECT_EQ(removedIDs, foundRemoved);
}

void checkChangedRouteTable(
    const shared_ptr<RouteTableMap>& oldTables,
    const shared_ptr<RouteTableMap>& newTables,
    const std::set<uint32_t> changedIDs,
    const std::set<uint32_t> addedIDs,
    const std::set<uint32_t> removedIDs) {
  auto oldState = make_shared<SwitchState>();
  oldState->resetRouteTables(oldTables);
  auto newState = make_shared<SwitchState>();
  newState->resetRouteTables(newTables);

  std::set<uint32_t> foundChanged;
  std::set<uint32_t> foundAdded;
  std::set<uint32_t> foundRemoved;
  StateDelta delta(oldState, newState);
  DeltaFunctions::forEachChanged(
      delta.getRouteTablesDelta(),
      [&](const shared_ptr<RouteTable>& oldTable,
          const shared_ptr<RouteTable>& newTable) {
        EXPECT_EQ(oldTable->getID(), newTable->getID());
        EXPECT_NE(oldTable, newTable);
        auto ret = foundChanged.insert(oldTable->getID());
        EXPECT_TRUE(ret.second);
      },
      [&](const shared_ptr<RouteTable>& table) {
        auto ret = foundAdded.insert(table->getID());
        EXPECT_TRUE(ret.second);
      },
      [&](const shared_ptr<RouteTable>& table) {
        auto ret = foundRemoved.insert(table->getID());
        EXPECT_TRUE(ret.second);
      });

  EXPECT_EQ(changedIDs, foundChanged);
  EXPECT_EQ(addedIDs, foundAdded);
  EXPECT_EQ(removedIDs, foundRemoved);
}

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
  RouteNextHopsMulti nhm2 = nhm1;

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

  auto serialized = nhm1.toFollyDynamic();

  auto nhm2 = RouteNextHopsMulti::fromFollyDynamic(serialized);

  EXPECT_TRUE(nhm1 == nhm2);
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

bool stringStartsWith(std::string s1, std::string prefix) {
  return s1.compare(0, prefix.size(), prefix) == 0;
}

void assertClientsNotPresent(
    std::shared_ptr<RouteTableMap>& tables,
    RouterID rid,
    RouteV4::Prefix prefix,
    std::vector<int16_t> clientIds) {
  for (int16_t clientId : clientIds) {
    const auto& route =
        tables->getRouteTable(rid)->getRibV4()->exactMatch(prefix);
    auto entry = route->getEntryForClient(ClientID(clientId));
    EXPECT_EQ(nullptr, entry);
  }
}

void assertClientsPresent(
    std::shared_ptr<RouteTableMap>& tables,
    RouterID rid,
    RouteV4::Prefix prefix,
    std::vector<int16_t> clientIds) {
  for (int16_t clientId : clientIds) {
    const auto& route =
        tables->getRouteTable(rid)->getRibV4()->exactMatch(prefix);
    auto entry = route->getEntryForClient(ClientID(clientId));
    EXPECT_NE(nullptr, entry);
  }
}

TEST(Route, dropRoutes) {
  auto stateV1 = make_shared<SwitchState>();
  stateV1->publish();
  auto tables1 = stateV1->getRouteTables();
  auto rid = RouterID(0);
  RouteUpdater u1(tables1);
  u1.addRoute(
      rid,
      IPAddress("10.10.10.10"),
      32,
      CLIENT_A,
      RouteNextHopEntry(RouteForwardAction::DROP, DISTANCE));
  u1.addRoute(
      rid,
      IPAddress("2001::0"),
      128,
      CLIENT_A,
      RouteNextHopEntry(RouteForwardAction::DROP, DISTANCE));
  // Check recursive resolution for drop routes
  RouteNextHopSet v4nexthops = makeNextHops({"10.10.10.10"});
  u1.addRoute(
      rid,
      IPAddress("20.20.20.0"),
      24,
      CLIENT_A,
      RouteNextHopEntry(v4nexthops, DISTANCE));
  RouteNextHopSet v6nexthops = makeNextHops({"2001::0"});
  u1.addRoute(
      rid,
      IPAddress("2001:1::"),
      64,
      CLIENT_A,
      RouteNextHopEntry(v6nexthops, DISTANCE));

  auto tables2 = u1.updateDone();
  ASSERT_NE(nullptr, tables2);
  EXPECT_NODEMAP_MATCH(tables2);

  // Check routes
  auto r1 = GET_ROUTE_V4(tables2, rid, "10.10.10.10/32");
  EXPECT_RESOLVED(r1);
  EXPECT_FALSE(r1->isConnected());
  EXPECT_TRUE(
      r1->has(CLIENT_A, RouteNextHopEntry(RouteForwardAction::DROP, DISTANCE)));
  EXPECT_EQ(
      r1->getForwardInfo(),
      RouteNextHopEntry(RouteForwardAction::DROP, DISTANCE));

  auto r2 = GET_ROUTE_V4(tables2, rid, "20.20.20.0/24");
  EXPECT_RESOLVED(r2);
  EXPECT_FALSE(r2->isConnected());
  EXPECT_FALSE(
      r2->has(CLIENT_A, RouteNextHopEntry(RouteForwardAction::DROP, DISTANCE)));
  EXPECT_EQ(
      r2->getForwardInfo(),
      RouteNextHopEntry(RouteForwardAction::DROP, DISTANCE));

  auto r3 = GET_ROUTE_V6(tables2, rid, "2001::0/128");
  EXPECT_RESOLVED(r3);
  EXPECT_FALSE(r3->isConnected());
  EXPECT_TRUE(
      r3->has(CLIENT_A, RouteNextHopEntry(RouteForwardAction::DROP, DISTANCE)));
  EXPECT_EQ(
      r3->getForwardInfo(),
      RouteNextHopEntry(RouteForwardAction::DROP, DISTANCE));

  auto r4 = GET_ROUTE_V6(tables2, rid, "2001:1::/64");
  EXPECT_RESOLVED(r4);
  EXPECT_FALSE(r4->isConnected());
  EXPECT_EQ(
      r4->getForwardInfo(),
      RouteNextHopEntry(RouteForwardAction::DROP, DISTANCE));
}

TEST(Route, toCPURoutes) {
  auto stateV1 = make_shared<SwitchState>();
  stateV1->publish();
  auto tables1 = stateV1->getRouteTables();
  auto rid = RouterID(0);
  RouteUpdater u1(tables1);
  u1.addRoute(
      rid,
      IPAddress("10.10.10.10"),
      32,
      CLIENT_A,
      RouteNextHopEntry(RouteForwardAction::TO_CPU, DISTANCE));
  u1.addRoute(
      rid,
      IPAddress("2001::0"),
      128,
      CLIENT_A,
      RouteNextHopEntry(RouteForwardAction::TO_CPU, DISTANCE));
  // Check recursive resolution for to_cpu routes
  RouteNextHopSet v4nexthops = makeNextHops({"10.10.10.10"});
  u1.addRoute(
      rid,
      IPAddress("20.20.20.0"),
      24,
      CLIENT_A,
      RouteNextHopEntry(v4nexthops, DISTANCE));
  RouteNextHopSet v6nexthops = makeNextHops({"2001::0"});
  u1.addRoute(
      rid,
      IPAddress("2001:1::"),
      64,
      CLIENT_A,
      RouteNextHopEntry(v6nexthops, DISTANCE));

  auto tables2 = u1.updateDone();
  ASSERT_NE(nullptr, tables2);
  EXPECT_NODEMAP_MATCH(tables2);

  // Check routes
  auto r1 = GET_ROUTE_V4(tables2, rid, "10.10.10.10/32");
  EXPECT_RESOLVED(r1);
  EXPECT_FALSE(r1->isConnected());
  EXPECT_TRUE(r1->has(
      CLIENT_A, RouteNextHopEntry(RouteForwardAction::TO_CPU, DISTANCE)));
  EXPECT_EQ(
      r1->getForwardInfo(),
      RouteNextHopEntry(RouteForwardAction::TO_CPU, DISTANCE));

  auto r2 = GET_ROUTE_V4(tables2, rid, "20.20.20.0/24");
  EXPECT_RESOLVED(r2);
  EXPECT_FALSE(r2->isConnected());
  EXPECT_EQ(
      r2->getForwardInfo(),
      RouteNextHopEntry(RouteForwardAction::TO_CPU, DISTANCE));

  auto r3 = GET_ROUTE_V6(tables2, rid, "2001::0/128");
  EXPECT_RESOLVED(r3);
  EXPECT_FALSE(r3->isConnected());
  EXPECT_TRUE(r3->has(
      CLIENT_A, RouteNextHopEntry(RouteForwardAction::TO_CPU, DISTANCE)));
  EXPECT_EQ(
      r3->getForwardInfo(),
      RouteNextHopEntry(RouteForwardAction::TO_CPU, DISTANCE));

  auto r5 = GET_ROUTE_V6(tables2, rid, "2001:1::/64");
  EXPECT_RESOLVED(r5);
  EXPECT_FALSE(r5->isConnected());
  EXPECT_EQ(
      r5->getForwardInfo(),
      RouteNextHopEntry(RouteForwardAction::TO_CPU, DISTANCE));
}

// Very basic test for serialization/deseralization of Routes
TEST(Route, serializeRoute) {
  ClientID clientId = ClientID(1);
  auto nxtHops = makeNextHops({"10.10.10.10", "11.11.11.11"});
  Route<IPAddressV4> rt(makePrefixV4("1.2.3.4/32"));
  rt.update(clientId, RouteNextHopEntry(nxtHops, DISTANCE));

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
}

TEST(Route, serializeRouteTable) {
  auto stateV1 = make_shared<SwitchState>();
  stateV1->publish();

  auto rid = RouterID(0);
  // 2 different nexthops
  RouteNextHopSet nhop1 = makeNextHops({"1.1.1.10"}); // resolved by intf 1
  RouteNextHopSet nhop2 = makeNextHops({"2.2.2.10"}); // resolved by intf 2
  // 4 prefixes
  RouteV4::Prefix r1{IPAddressV4("10.1.1.0"), 24};
  RouteV4::Prefix r2{IPAddressV4("20.1.1.0"), 24};
  RouteV6::Prefix r3{IPAddressV6("1001::0"), 48};
  RouteV6::Prefix r4{IPAddressV6("2001::0"), 48};

  RouteUpdater u2(stateV1->getRouteTables());
  u2.addRoute(
      rid, r1.network, r1.mask, CLIENT_A, RouteNextHopEntry(nhop1, DISTANCE));
  u2.addRoute(
      rid, r2.network, r2.mask, CLIENT_A, RouteNextHopEntry(nhop2, DISTANCE));
  u2.addRoute(
      rid, r3.network, r3.mask, CLIENT_A, RouteNextHopEntry(nhop1, DISTANCE));
  u2.addRoute(
      rid, r4.network, r4.mask, CLIENT_A, RouteNextHopEntry(nhop2, DISTANCE));
  auto tables2 = u2.updateDone();
  ASSERT_NE(nullptr, tables2);
  EXPECT_NODEMAP_MATCH(tables2);

  // to folly dynamic
  folly::dynamic obj = tables2->toFollyDynamic();
  // to string
  folly::json::serialization_opts serOpts;
  serOpts.allow_non_string_keys = true;
  std::string json = folly::json::serialize(obj, serOpts);
  // back to folly dynamic
  folly::dynamic obj2 = folly::parseJson(json, serOpts);
  // back to Route object
  auto tables = RouteTableMap::fromFollyDynamic(obj2);
  EXPECT_NODEMAP_MATCH(tables);
  auto origRt = tables2->getRouteTable(rid);
  auto desRt = tables->getRouteTable(rid);
  EXPECT_ROUTETABLERIB_MATCH(origRt->getRibV4(), desRt->getRibV4());
  EXPECT_ROUTETABLERIB_MATCH(origRt->getRibV6(), desRt->getRibV6());
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
      bAddr.ifName_ref() = util::createTunIntfName(intf.value());
    }
    bool found = false;
    for (const auto& entry : nhts) {
      if (*entry.address_ref() == bAddr) {
        if (intf.has_value()) {
          EXPECT_TRUE(entry.address_ref()->ifName_ref());
          EXPECT_EQ(*bAddr.ifName_ref(), *entry.address_ref()->ifName_ref());
        }
        found = true;
        break;
      }
    }
    XLOG(INFO) << "**** " << ipaddr;
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
  addr.ifName_ref() = "fboss10";
  NextHopThrift nht;
  *nht.address_ref() = addr;
  {
    NextHop nh = util::fromThrift(nht);
    EXPECT_EQ(folly::IPAddress("10.0.0.1"), nh.addr());
    EXPECT_EQ(std::nullopt, nh.intfID());
  }

  addr = facebook::network::toBinaryAddress(folly::IPAddress("face::1"));
  addr.ifName_ref() = "fboss10";
  *nht.address_ref() = addr;
  {
    NextHop nh = util::fromThrift(nht);
    EXPECT_EQ(folly::IPAddress("face::1"), nh.addr());
    EXPECT_EQ(std::nullopt, nh.intfID());
  }

  addr = facebook::network::toBinaryAddress(folly::IPAddress("fe80::1"));
  addr.ifName_ref() = "fboss10";
  *nht.address_ref() = addr;
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

TEST(Route, nodeMapMatchesRadixTree) {
  auto stateV1 = applyInitConfig();
  ASSERT_NE(nullptr, stateV1);
  auto tables1 = stateV1->getRouteTables();

  auto rid = RouterID(0);
  RouteNextHopSet nhop1 = makeNextHops({"1.1.1.10"}); // resolved by intf 1
  RouteNextHopSet nhop2 = makeNextHops({"2.2.2.10"}); // resolved by intf 2
  RouteNextHopSet nonResolvedHops = makeNextHops({"1.1.3.10"}); // Non-resolved

  RouteV4::Prefix r1{IPAddressV4("10.1.1.0"), 24};
  RouteV4::Prefix r2{IPAddressV4("20.1.1.0"), 24};
  RouteV6::Prefix r3{IPAddressV6("1001::0"), 48};
  RouteV6::Prefix r4{IPAddressV6("2001::0"), 48};
  RouteV4::Prefix r5{IPAddressV4("8.8.8.0"), 24};
  RouteV4::Prefix r6{IPAddressV4("1.1.3.0"), 24};

  // add route case
  RouteUpdater u1(tables1);
  u1.addRoute(
      rid, r1.network, r1.mask, CLIENT_A, RouteNextHopEntry(nhop1, DISTANCE));
  u1.addRoute(
      rid, r2.network, r2.mask, CLIENT_A, RouteNextHopEntry(nhop2, DISTANCE));
  u1.addRoute(
      rid, r3.network, r3.mask, CLIENT_A, RouteNextHopEntry(nhop1, DISTANCE));
  u1.addRoute(
      rid, r4.network, r4.mask, CLIENT_A, RouteNextHopEntry(nhop2, DISTANCE));
  u1.addRoute(
      rid,
      r5.network,
      r5.mask,
      CLIENT_A,
      RouteNextHopEntry(nonResolvedHops, DISTANCE));
  auto tables2 = u1.updateDone();
  ASSERT_NE(nullptr, tables2);
  // check every node in nodeMap_ also matches node in rib_, and every node
  // should be published after the routeTable is published
  EXPECT_NODEMAP_MATCH(tables2);
  // make sure new change won't affect the initial state
  EXPECT_NO_ROUTE(tables1, rid, r5.str());

  // del route case
  RouteUpdater u2(tables2);
  u2.delRoute(rid, r2.network, r2.mask, CLIENT_A);
  auto tables3 = u2.updateDone();
  ASSERT_NE(nullptr, tables3);
  EXPECT_NODEMAP_MATCH(tables3);

  // update route case, resolve previous nonResolved route with prefix r5
  RouteUpdater u3(tables3);
  u3.addRoute(
      rid, r6.network, r6.mask, CLIENT_A, RouteNextHopEntry(nhop1, DISTANCE));
  auto tables4 = u3.updateDone();
  EXPECT_NODEMAP_MATCH(tables4);
  ASSERT_NE(nullptr, tables4);
  // make sure new change won't affect the initial state
  EXPECT_NO_ROUTE(tables1, rid, r6.str());
  // both r5 and r6 should be unpublished now
  ASSERT_FALSE(GET_ROUTE_V4(tables4, rid, r5)->isPublished());
  ASSERT_FALSE(GET_ROUTE_V4(tables4, rid, r6)->isPublished());
}

TEST(Route, withLabelForwardingAction) {
  auto rid = RouterID(0);

  std::array<folly::IPAddressV4, 4> nextHopAddrs{
      folly::IPAddressV4("1.1.1.0"),
      folly::IPAddressV4("1.1.2.0"),
      folly::IPAddressV4("1.1.3.0"),
      folly::IPAddressV4("1.1.4.0"),
  };

  std::array<LabelForwardingAction::LabelStack, 4> nextHopStacks{
      LabelForwardingAction::LabelStack({101, 201, 301}),
      LabelForwardingAction::LabelStack({102, 202, 302}),
      LabelForwardingAction::LabelStack({103, 203, 303}),
      LabelForwardingAction::LabelStack({104, 204, 304}),
  };

  std::map<folly::IPAddressV4, LabelForwardingAction::LabelStack>
      labeledNextHops;
  RouteNextHopSet nexthops;

  for (auto i = 0; i < 4; i++) {
    labeledNextHops.emplace(std::make_pair(nextHopAddrs[i], nextHopStacks[i]));
    nexthops.emplace(UnresolvedNextHop(
        nextHopAddrs[i],
        1,
        std::make_optional<LabelForwardingAction>(
            LabelForwardingAction::LabelForwardingType::PUSH,
            nextHopStacks[i])));
  }

  auto state = applyInitConfig();
  ASSERT_NE(nullptr, state);
  auto tables = state->getRouteTables();

  RouteUpdater updater(tables);
  updater.addRoute(
      rid,
      folly::IPAddressV4("1.1.0.0"),
      16,
      CLIENT_A,
      RouteNextHopEntry(nexthops, DISTANCE));

  tables = updater.updateDone();
  EXPECT_NODEMAP_MATCH(tables);
  const auto& route = tables->getRouteTableIf(rid)->getRibV4()->longestMatch(
      folly::IPAddressV4("1.1.2.2"));

  EXPECT_EQ(route->has(CLIENT_A, RouteNextHopEntry(nexthops, DISTANCE)), true);
  auto entry = route->getBestEntry();
  for (const auto& nh : entry.second->getNextHopSet()) {
    EXPECT_EQ(nh.labelForwardingAction().has_value(), true);
    EXPECT_EQ(
        nh.labelForwardingAction()->type(),
        LabelForwardingAction::LabelForwardingType::PUSH);
    EXPECT_EQ(nh.labelForwardingAction()->pushStack().has_value(), true);
    EXPECT_EQ(nh.labelForwardingAction()->pushStack()->size(), 3);
    EXPECT_EQ(
        labeledNextHops[nh.addr().asV4()],
        nh.labelForwardingAction()->pushStack().value());
  }
}

TEST(Route, unresolvedWithRouteLabels) {
  auto rid = RouterID(0);
  RouteNextHopSet bgpNextHops;

  for (auto i = 0; i < 4; i++) {
    // bgp next hops with some labels, for some prefix
    bgpNextHops.emplace(UnresolvedNextHop(
        kBgpNextHopAddrs[i],
        1,
        std::make_optional<LabelForwardingAction>(
            LabelForwardingAction::LabelForwardingType::PUSH,
            LabelForwardingAction::LabelStack{
                kLabelStacks[i].begin(), kLabelStacks[i].end()})));
  }

  auto state = applyInitConfig();
  ASSERT_NE(nullptr, state);
  auto tables = state->getRouteTables();

  RouteUpdater updater(tables);
  // routes to remote prefix to bgp next hops
  updater.addRoute(
      rid,
      kDestPrefix.network,
      kDestPrefix.mask,
      ClientID::BGPD,
      RouteNextHopEntry(bgpNextHops, DISTANCE));

  tables = updater.updateDone();
  EXPECT_NODEMAP_MATCH(tables);

  // route to remote destination under kDestPrefix advertised by bgp
  const auto& route =
      tables->getRouteTableIf(rid)->getRibV6()->longestMatch(kDestAddress);

  EXPECT_EQ(
      route->has(ClientID::BGPD, RouteNextHopEntry(bgpNextHops, DISTANCE)),
      true);

  EXPECT_FALSE(route->isResolved());
}

TEST(Route, withTunnelAndRouteLabels) {
  auto rid = RouterID(0);
  RouteNextHopSet bgpNextHops;

  for (auto i = 0; i < 4; i++) {
    // bgp next hops with some labels, for some prefix
    bgpNextHops.emplace(UnresolvedNextHop(
        kBgpNextHopAddrs[i],
        1,
        std::make_optional<LabelForwardingAction>(
            LabelForwardingAction::LabelForwardingType::PUSH,
            LabelForwardingAction::LabelStack{
                kLabelStacks[i].begin(), kLabelStacks[i].begin() + 2})));
  }

  auto state = applyInitConfig();
  ASSERT_NE(nullptr, state);
  auto tables = state->getRouteTables();

  RouteUpdater updater(tables);
  // routes to remote prefix to bgp next hops
  updater.addRoute(
      rid,
      kDestPrefix.network,
      kDestPrefix.mask,
      ClientID::BGPD,
      RouteNextHopEntry(bgpNextHops, DISTANCE));

  std::vector<ResolvedNextHop> igpNextHops;
  for (auto i = 0; i < 4; i++) {
    igpNextHops.push_back(ResolvedNextHop(
        kIgpAddrs[i],
        kInterfaces[i],
        ECMP_WEIGHT,
        LabelForwardingAction(
            LabelForwardingAction::LabelForwardingType::PUSH,
            LabelForwardingAction::LabelStack{
                kLabelStacks[i].begin() + 2, kLabelStacks[i].begin() + 3})));
  }

  // igp routes to bgp nexthops,
  for (auto i = 0; i < 4; i++) {
    updater.addRoute(
        rid,
        kBgpNextHopAddrs[i],
        64,
        ClientID::OPENR,
        RouteNextHopEntry(igpNextHops[i], AdminDistance::DIRECTLY_CONNECTED));
  }

  tables = updater.updateDone();
  EXPECT_NODEMAP_MATCH(tables);

  // route to remote destination under kDestPrefix advertised by bgp
  const auto& route =
      tables->getRouteTableIf(rid)->getRibV6()->longestMatch(kDestAddress);

  EXPECT_EQ(
      route->has(ClientID::BGPD, RouteNextHopEntry(bgpNextHops, DISTANCE)),
      true);

  EXPECT_TRUE(route->isResolved());

  for (const auto& nhop : route->getForwardInfo().getNextHopSet()) {
    EXPECT_TRUE(nhop.isResolved());
    EXPECT_TRUE(nhop.labelForwardingAction().has_value());
    EXPECT_EQ(
        nhop.labelForwardingAction()->type(),
        LabelForwardingAction::LabelForwardingType::PUSH);

    ASSERT_TRUE(
        nhop.intf() == kInterfaces[0] || nhop.intf() == kInterfaces[1] ||
        nhop.intf() == kInterfaces[2] || nhop.intf() == kInterfaces[3]);

    if (nhop.intf() == kInterfaces[0]) {
      EXPECT_EQ(
          nhop.labelForwardingAction()->pushStack().value(), kLabelStacks[0]);
    } else if (nhop.intf() == kInterfaces[1]) {
      EXPECT_EQ(
          nhop.labelForwardingAction()->pushStack().value(), kLabelStacks[1]);
    } else if (nhop.intf() == kInterfaces[2]) {
      EXPECT_EQ(
          nhop.labelForwardingAction()->pushStack().value(), kLabelStacks[2]);
    } else if (nhop.intf() == kInterfaces[3]) {
      EXPECT_EQ(
          nhop.labelForwardingAction()->pushStack().value(), kLabelStacks[3]);
    }
  }
}

TEST(Route, withOnlyTunnelLabels) {
  auto rid = RouterID(0);
  RouteNextHopSet bgpNextHops;

  for (auto i = 0; i < 4; i++) {
    // bgp next hops with some labels, for some prefix
    bgpNextHops.emplace(UnresolvedNextHop(kBgpNextHopAddrs[i], 1));
  }

  auto state = applyInitConfig();
  ASSERT_NE(nullptr, state);
  auto tables = state->getRouteTables();

  RouteUpdater updater(tables);
  // routes to remote prefix to bgp next hops
  updater.addRoute(
      rid,
      kDestPrefix.network,
      kDestPrefix.mask,
      ClientID::BGPD,
      RouteNextHopEntry(bgpNextHops, DISTANCE));

  std::vector<ResolvedNextHop> igpNextHops;
  for (auto i = 0; i < 4; i++) {
    igpNextHops.push_back(ResolvedNextHop(
        kIgpAddrs[i],
        kInterfaces[i],
        ECMP_WEIGHT,
        LabelForwardingAction(
            LabelForwardingAction::LabelForwardingType::PUSH,
            LabelForwardingAction::LabelStack{
                kLabelStacks[i].begin(), kLabelStacks[i].end()})));
  }

  // igp routes to bgp nexthops,
  for (auto i = 0; i < 4; i++) {
    updater.addRoute(
        rid,
        kBgpNextHopAddrs[i],
        64,
        ClientID::OPENR,
        RouteNextHopEntry(igpNextHops[i], AdminDistance::DIRECTLY_CONNECTED));
  }

  tables = updater.updateDone();
  EXPECT_NODEMAP_MATCH(tables);

  // route to remote destination under kDestPrefix advertised by bgp
  const auto& route =
      tables->getRouteTableIf(rid)->getRibV6()->longestMatch(kDestAddress);

  EXPECT_EQ(
      route->has(ClientID::BGPD, RouteNextHopEntry(bgpNextHops, DISTANCE)),
      true);

  EXPECT_TRUE(route->isResolved());

  for (const auto& nhop : route->getForwardInfo().getNextHopSet()) {
    EXPECT_TRUE(nhop.isResolved());
    EXPECT_TRUE(nhop.labelForwardingAction().has_value());
    EXPECT_EQ(
        nhop.labelForwardingAction()->type(),
        LabelForwardingAction::LabelForwardingType::PUSH);

    ASSERT_TRUE(
        nhop.intf() == kInterfaces[0] || nhop.intf() == kInterfaces[1] ||
        nhop.intf() == kInterfaces[2] || nhop.intf() == kInterfaces[3]);

    if (nhop.intf() == kInterfaces[0]) {
      EXPECT_EQ(
          nhop.labelForwardingAction()->pushStack().value(), kLabelStacks[0]);
    } else if (nhop.intf() == kInterfaces[1]) {
      EXPECT_EQ(
          nhop.labelForwardingAction()->pushStack().value(), kLabelStacks[1]);
    } else if (nhop.intf() == kInterfaces[2]) {
      EXPECT_EQ(
          nhop.labelForwardingAction()->pushStack().value(), kLabelStacks[2]);
    } else if (nhop.intf() == kInterfaces[3]) {
      EXPECT_EQ(
          nhop.labelForwardingAction()->pushStack().value(), kLabelStacks[3]);
    }
  }
}

TEST(Route, updateTunnelLabels) {
  auto rid = RouterID(0);
  RouteNextHopSet bgpNextHops;
  bgpNextHops.emplace(UnresolvedNextHop(
      kBgpNextHopAddrs[0],
      1,
      std::make_optional<LabelForwardingAction>(
          LabelForwardingAction::LabelForwardingType::PUSH,
          LabelForwardingAction::LabelStack{
              kLabelStacks[0].begin(), kLabelStacks[0].begin() + 2})));

  auto state = applyInitConfig();
  ASSERT_NE(nullptr, state);
  auto tables = state->getRouteTables();

  RouteUpdater updater(tables);
  // routes to remote prefix to bgp next hops
  updater.addRoute(
      rid,
      kDestPrefix.network,
      kDestPrefix.mask,
      ClientID::BGPD,
      RouteNextHopEntry(bgpNextHops, DISTANCE));

  ResolvedNextHop igpNextHop{
      kIgpAddrs[0],
      kInterfaces[0],
      ECMP_WEIGHT,
      LabelForwardingAction(
          LabelForwardingAction::LabelForwardingType::PUSH,
          LabelForwardingAction::LabelStack{
              kLabelStacks[0].begin() + 2, kLabelStacks[0].begin() + 3})};
  // igp routes to bgp nexthops,
  updater.addRoute(
      rid,
      kBgpNextHopAddrs[0],
      64,
      ClientID::OPENR,
      RouteNextHopEntry(igpNextHop, AdminDistance::DIRECTLY_CONNECTED));

  tables = updater.updateDone();
  EXPECT_NODEMAP_MATCH(tables);

  // igp next hop is updated
  ResolvedNextHop updatedIgpNextHop{
      kIgpAddrs[0],
      kInterfaces[0],
      ECMP_WEIGHT,
      LabelForwardingAction(
          LabelForwardingAction::LabelForwardingType::PUSH,
          LabelForwardingAction::LabelStack{
              kLabelStacks[1].begin() + 2, kLabelStacks[1].begin() + 3})};

  RouteUpdater anotherUpdater(tables);
  anotherUpdater.delRoute(rid, kBgpNextHopAddrs[0], 64, ClientID::OPENR);
  anotherUpdater.addRoute(
      rid,
      kBgpNextHopAddrs[0],
      64,
      ClientID::OPENR,
      RouteNextHopEntry(updatedIgpNextHop, AdminDistance::DIRECTLY_CONNECTED));

  tables = anotherUpdater.updateDone();
  EXPECT_NODEMAP_MATCH(tables);

  LabelForwardingAction::LabelStack updatedStack;
  updatedStack.push_back(*kLabelStacks[0].begin());
  updatedStack.push_back(*(kLabelStacks[0].begin() + 1));
  updatedStack.push_back(*(kLabelStacks[1].begin() + 2));

  // route to remote destination under kDestPrefix advertised by bgp
  const auto& route =
      tables->getRouteTableIf(rid)->getRibV6()->longestMatch(kDestAddress);

  EXPECT_EQ(
      route->has(ClientID::BGPD, RouteNextHopEntry(bgpNextHops, DISTANCE)),
      true);

  EXPECT_TRUE(route->isResolved());

  for (const auto& nhop : route->getForwardInfo().getNextHopSet()) {
    EXPECT_TRUE(nhop.isResolved());
    EXPECT_TRUE(nhop.labelForwardingAction().has_value());
    EXPECT_EQ(
        nhop.labelForwardingAction()->type(),
        LabelForwardingAction::LabelForwardingType::PUSH);

    ASSERT_TRUE(nhop.intf() == kInterfaces[0]);

    EXPECT_EQ(nhop.labelForwardingAction()->pushStack().value(), updatedStack);
  }
}

TEST(Route, updateRouteLabels) {
  auto rid = RouterID(0);
  RouteNextHopSet bgpNextHops;
  bgpNextHops.emplace(UnresolvedNextHop(
      kBgpNextHopAddrs[0],
      1,
      std::make_optional<LabelForwardingAction>(
          LabelForwardingAction::LabelForwardingType::PUSH,
          LabelForwardingAction::LabelStack{
              kLabelStacks[0].begin(), kLabelStacks[0].begin() + 2})));

  auto state = applyInitConfig();
  ASSERT_NE(nullptr, state);
  auto tables = state->getRouteTables();

  RouteUpdater updater(tables);
  // routes to remote prefix to bgp next hops
  updater.addRoute(
      rid,
      kDestPrefix.network,
      kDestPrefix.mask,
      ClientID::BGPD,
      RouteNextHopEntry(bgpNextHops, DISTANCE));

  ResolvedNextHop igpNextHop{
      kIgpAddrs[0],
      kInterfaces[0],
      ECMP_WEIGHT,
      LabelForwardingAction(
          LabelForwardingAction::LabelForwardingType::PUSH,
          LabelForwardingAction::LabelStack{
              kLabelStacks[0].begin() + 2, kLabelStacks[0].begin() + 3})};
  // igp routes to bgp nexthops,
  updater.addRoute(
      rid,
      kBgpNextHopAddrs[0],
      64,
      ClientID::OPENR,
      RouteNextHopEntry(igpNextHop, AdminDistance::DIRECTLY_CONNECTED));

  tables = updater.updateDone();
  EXPECT_NODEMAP_MATCH(tables);

  // igp next hop is updated
  UnresolvedNextHop updatedBgpNextHop{
      kBgpNextHopAddrs[0],
      1,
      std::make_optional<LabelForwardingAction>(
          LabelForwardingAction::LabelForwardingType::PUSH,
          LabelForwardingAction::LabelStack{
              kLabelStacks[1].begin(), kLabelStacks[1].begin() + 2})};

  RouteUpdater anotherUpdater(tables);
  anotherUpdater.delRoute(
      rid, kDestPrefix.network, kDestPrefix.mask, ClientID::BGPD);

  anotherUpdater.addRoute(
      rid,
      kDestPrefix.network,
      kDestPrefix.mask,
      ClientID::BGPD,
      RouteNextHopEntry(updatedBgpNextHop, DISTANCE));

  tables = anotherUpdater.updateDone();
  EXPECT_NODEMAP_MATCH(tables);

  LabelForwardingAction::LabelStack updatedStack;
  updatedStack.push_back(*kLabelStacks[1].begin());
  updatedStack.push_back(*(kLabelStacks[1].begin() + 1));
  updatedStack.push_back(*(kLabelStacks[0].begin() + 2));

  // route to remote destination under kDestPrefix advertised by bgp
  const auto& route =
      tables->getRouteTableIf(rid)->getRibV6()->longestMatch(kDestAddress);

  EXPECT_EQ(
      route->has(
          ClientID::BGPD, RouteNextHopEntry(updatedBgpNextHop, DISTANCE)),
      true);

  EXPECT_TRUE(route->isResolved());

  for (const auto& nhop : route->getForwardInfo().getNextHopSet()) {
    EXPECT_TRUE(nhop.isResolved());
    EXPECT_TRUE(nhop.labelForwardingAction().has_value());
    EXPECT_EQ(
        nhop.labelForwardingAction()->type(),
        LabelForwardingAction::LabelForwardingType::PUSH);

    ASSERT_TRUE(nhop.intf() == kInterfaces[0]);

    EXPECT_EQ(nhop.labelForwardingAction()->pushStack().value(), updatedStack);
  }
}

TEST(Route, withNoLabelForwardingAction) {
  auto rid = RouterID(0);

  auto state = applyInitConfig();
  ASSERT_NE(nullptr, state);
  auto tables = state->getRouteTables();

  auto routeNextHopEntry = RouteNextHopEntry(
      makeNextHops({"1.1.1.1", "1.1.2.1", "1.1.3.1", "1.1.4.1"}), DISTANCE);
  RouteUpdater updater(tables);
  updater.addRoute(
      rid, folly::IPAddressV4("1.1.0.0"), 16, CLIENT_A, routeNextHopEntry);

  tables = updater.updateDone();
  EXPECT_NODEMAP_MATCH(tables);
  const auto& route = tables->getRouteTableIf(rid)->getRibV4()->longestMatch(
      folly::IPAddressV4("1.1.2.2"));

  EXPECT_EQ(route->has(CLIENT_A, routeNextHopEntry), true);
  auto entry = route->getBestEntry();
  for (const auto& nh : entry.second->getNextHopSet()) {
    EXPECT_EQ(nh.labelForwardingAction().has_value(), false);
  }
  EXPECT_EQ(*entry.second, routeNextHopEntry);
}

TEST(Route, withInvalidLabelForwardingAction) {
  auto rid = RouterID(0);

  std::array<folly::IPAddressV4, 5> nextHopAddrs{
      folly::IPAddressV4("1.1.1.0"),
      folly::IPAddressV4("1.1.2.0"),
      folly::IPAddressV4("1.1.3.0"),
      folly::IPAddressV4("1.1.4.0"),
      folly::IPAddressV4("1.1.5.0"),
  };

  std::array<LabelForwardingAction, 5> nextHopLabelActions{
      LabelForwardingAction(
          LabelForwardingAction::LabelForwardingType::PUSH,
          LabelForwardingAction::LabelStack({101, 201, 301})),
      LabelForwardingAction(
          LabelForwardingAction::LabelForwardingType::SWAP,
          LabelForwardingAction::Label{202}),
      LabelForwardingAction(LabelForwardingAction::LabelForwardingType::NOOP),
      LabelForwardingAction(
          LabelForwardingAction::LabelForwardingType::POP_AND_LOOKUP),
      LabelForwardingAction(LabelForwardingAction::LabelForwardingType::PHP),
  };

  std::map<folly::IPAddressV4, LabelForwardingAction::LabelStack>
      labeledNextHops;
  RouteNextHopSet nexthops;

  for (auto i = 0; i < 5; i++) {
    nexthops.emplace(
        UnresolvedNextHop(nextHopAddrs[i], 1, nextHopLabelActions[i]));
  }

  auto state = applyInitConfig();
  ASSERT_NE(nullptr, state);
  auto tables = state->getRouteTables();

  RouteUpdater updater(tables);
  EXPECT_THROW(
      {
        updater.addRoute(
            rid,
            folly::IPAddressV4("1.1.0.0"),
            16,
            CLIENT_A,
            RouteNextHopEntry(nexthops, DISTANCE));
      },
      FbossError);
}

TEST(Route, nexthopFromThriftAndDynamic) {
  IPAddressV6 ip{"fe80::1"};
  NextHopThrift nexthop;
  nexthop.address_ref()->addr.append(
      reinterpret_cast<const char*>(ip.bytes()),
      folly::IPAddressV6::byteCount());
  nexthop.address_ref()->ifName_ref() = "fboss100";
  MplsAction action;
  *action.action_ref() = MplsActionCode::PUSH;
  action.pushLabels_ref() = MplsLabelStack{501, 502, 503};
  EXPECT_EQ(util::fromThrift(nexthop).toThrift(), nexthop);
  EXPECT_EQ(
      util::nextHopFromFollyDynamic(util::fromThrift(nexthop).toFollyDynamic()),
      util::fromThrift(nexthop));
}

/*
 * Class that makes it easy to run tests with the following
 * configurable entities:
 * Four interfaces: I1, I2, I3, I4
 * Three routes which require resolution: R1, R2, R3
 */
class UcmpTest : public ::testing::Test {
 public:
  void SetUp() override {
    stateV1_ = applyInitConfig();
    ASSERT_NE(nullptr, stateV1_);
    rid_ = RouterID(0);
  }
  void resolveRoutes(std::vector<std::pair<folly::IPAddress, RouteNextHopSet>>
                         networkAndNextHops) {
    RouteUpdater ru(stateV1_->getRouteTables());
    for (const auto& nnhs : networkAndNextHops) {
      folly::IPAddress net = nnhs.first;
      RouteNextHopSet nhs = nnhs.second;
      ru.addRoute(rid_, net, mask, CLIENT_A, RouteNextHopEntry(nhs, DISTANCE));
    }
    auto tables = ru.updateDone();
    ASSERT_NE(nullptr, tables);
    EXPECT_NODEMAP_MATCH(tables);
    tables->publish();
    for (const auto& nnhs : networkAndNextHops) {
      folly::IPAddress net = nnhs.first;
      auto pfx = folly::sformat("{}/{}", net.str(), mask);
      auto r = GET_ROUTE_V4(tables, rid_, pfx);
      EXPECT_RESOLVED(r);
      EXPECT_FALSE(r->isConnected());
      resolvedRoutes.push_back(r);
    }
  }

  void runRecursiveTest(
      const std::vector<RouteNextHopSet>& routeUnresolvedNextHops,
      const std::vector<NextHopWeight>& resolvedWeights) {
    std::vector<std::pair<folly::IPAddress, RouteNextHopSet>>
        networkAndNextHops;
    auto netsIter = nets.begin();
    for (const auto& nhs : routeUnresolvedNextHops) {
      networkAndNextHops.push_back({*netsIter, nhs});
      netsIter++;
    }
    resolveRoutes(networkAndNextHops);

    RouteNextHopSet expFwd1;
    uint8_t i = 0;
    for (const auto& w : resolvedWeights) {
      expFwd1.emplace(ResolvedNextHop(intfIps[i], InterfaceID(i + 1), w));
      ++i;
    }
    EXPECT_EQ(expFwd1, resolvedRoutes[0]->getForwardInfo().getNextHopSet());
  }

  void runTwoDeepRecursiveTest(
      const std::vector<std::vector<NextHopWeight>>& unresolvedWeights,
      const std::vector<NextHopWeight>& resolvedWeights) {
    std::vector<RouteNextHopSet> routeUnresolvedNextHops;
    auto rsIter = rnhs.begin();
    for (const auto& ws : unresolvedWeights) {
      auto nhIter = rsIter->begin();
      RouteNextHopSet nexthops;
      for (const auto& w : ws) {
        nexthops.insert(UnresolvedNextHop(*nhIter, w));
        nhIter++;
      }
      routeUnresolvedNextHops.push_back(nexthops);
      rsIter++;
    }
    runRecursiveTest(routeUnresolvedNextHops, resolvedWeights);
  }

  void runVaryFromHundredTest(
      NextHopWeight w,
      const std::vector<NextHopWeight>& resolvedWeights) {
    runRecursiveTest(
        {{UnresolvedNextHop(intfIp1, 100),
          UnresolvedNextHop(intfIp2, 100),
          UnresolvedNextHop(intfIp3, 100),
          UnresolvedNextHop(intfIp4, w)}},
        resolvedWeights);
  }

  std::vector<std::shared_ptr<Route<folly::IPAddressV4>>> resolvedRoutes;
  const folly::IPAddress intfIp1{"1.1.1.10"};
  const folly::IPAddress intfIp2{"2.2.2.20"};
  const folly::IPAddress intfIp3{"3.3.3.30"};
  const folly::IPAddress intfIp4{"4.4.4.40"};
  const std::array<folly::IPAddress, 4> intfIps{
      {intfIp1, intfIp2, intfIp3, intfIp4}};
  const folly::IPAddress r2Nh{"42.42.42.42"};
  const folly::IPAddress r3Nh{"43.43.43.43"};
  std::array<folly::IPAddress, 2> r1Nhs{{r2Nh, r3Nh}};
  std::array<folly::IPAddress, 2> r2Nhs{{intfIp1, intfIp2}};
  std::array<folly::IPAddress, 2> r3Nhs{{intfIp3, intfIp4}};
  const std::array<std::array<folly::IPAddress, 2>, 3> rnhs{
      {r1Nhs, r2Nhs, r3Nhs}};
  const folly::IPAddress r1Net{"41.41.41.0"};
  const folly::IPAddress r2Net{"42.42.42.0"};
  const folly::IPAddress r3Net{"43.43.43.0"};
  const std::array<folly::IPAddress, 3> nets{{r1Net, r2Net, r3Net}};
  const uint8_t mask{24};

 private:
  RouterID rid_;
  std::shared_ptr<SwitchState> stateV1_;
};

/*
 * Four interfaces: I1, I2, I3, I4
 * Three routes which require resolution: R1, R2, R3
 * R1 has R2 and R3 as next hops with weights 3 and 2
 * R2 has I1 and I2 as next hops with weights 5 and 4
 * R3 has I3 and I4 as next hops with weights 3 and 2
 * expect R1 to resolve to I1:25, I2:20, I3:18, I4:12
 */
TEST_F(UcmpTest, recursiveUcmp) {
  runTwoDeepRecursiveTest({{3, 2}, {5, 4}, {3, 2}}, {25, 20, 18, 12});
}

/*
 * Two interfaces: I1, I2
 * Three routes which require resolution: R1, R2, R3
 * R1 has R2 and R3 as next hops with weights 2 and 1
 * R2 has I1 and I2 as next hops with weights 1 and 1
 * R3 has I1 as next hop with weight 1
 * expect R1 to resolve to I1:2, I2:1
 */
TEST_F(UcmpTest, recursiveUcmpDuplicateIntf) {
  runRecursiveTest(
      {{UnresolvedNextHop(r2Nh, 2), UnresolvedNextHop(r3Nh, 1)},
       {UnresolvedNextHop(intfIp1, 1), UnresolvedNextHop(intfIp2, 1)},
       {UnresolvedNextHop(intfIp1, 1)}},
      {2, 1});
}

/*
 * Two interfaces: I1, I2
 * Three routes which require resolution: R1, R2, R3
 * R1 has R2 and R3 as next hops with ECMP
 * R2 has I1 and I2 as next hops with ECMP
 * R3 has I1 as next hop with weight 1
 * expect R1 to resolve to ECMP
 */
TEST_F(UcmpTest, recursiveEcmpDuplicateIntf) {
  runRecursiveTest(
      {{UnresolvedNextHop(r2Nh, ECMP_WEIGHT),
        UnresolvedNextHop(r3Nh, ECMP_WEIGHT)},
       {UnresolvedNextHop(intfIp1, ECMP_WEIGHT),
        UnresolvedNextHop(intfIp2, ECMP_WEIGHT)},
       {UnresolvedNextHop(intfIp1, 1)}},
      {ECMP_WEIGHT, ECMP_WEIGHT});
}

/*
 * Two interfaces: I1, I2
 * One route which requires resolution: R1
 * R1 has I1 and I2 as next hops with weights 0 (ECMP) and 1
 * expect R1 to resolve to ECMP between I1, I2
 */
TEST_F(UcmpTest, mixedUcmpVsEcmp_EcmpWins) {
  runRecursiveTest(
      {{UnresolvedNextHop(intfIp1, ECMP_WEIGHT),
        UnresolvedNextHop(intfIp2, 1)}},
      {ECMP_WEIGHT, ECMP_WEIGHT});
}

/*
 * Four interfaces: I1, I2, I3, I4
 * Three routes which require resolution: R1, R2, R3
 * R1 has R2 and R3 as next hops with weights 3 and 2
 * R2 has I1 and I2 as next hops with weights 5 and 4
 * R3 has I3 and I4 as next hops with ECMP
 * expect R1 to resolve to ECMP between I1, I2, I3, I4
 */
TEST_F(UcmpTest, recursiveEcmpPropagatesUp) {
  runTwoDeepRecursiveTest({{3, 2}, {5, 4}, {0, 0}}, {0, 0, 0, 0});
}

/*
 * Four interfaces: I1, I2, I3, I4
 * Three routes which require resolution: R1, R2, R3
 * R1 has R2 and R3 as next hops with weights 3 and 2
 * R2 has I1 and I2 as next hops with weights 5 and 4
 * R3 has I3 and I4 as next hops with weights 0 (ECMP) and 1
 * expect R1 to resolve to ECMP between I1, I2, I3, I4
 */
TEST_F(UcmpTest, recursiveMixedEcmpPropagatesUp) {
  runTwoDeepRecursiveTest({{3, 2}, {5, 4}, {0, 1}}, {0, 0, 0, 0});
}

/*
 * Four interfaces: I1, I2, I3, I4
 * Three routes which require resolution: R1, R2, R3
 * R1 has R2 and R3 as next hops with ECMP
 * R2 has I1 and I2 as next hops with weights 5 and 4
 * R3 has I3 and I4 as next hops with weights 3 and 2
 * expect R1 to resolve to ECMP between I1, I2, I3, I4
 */
TEST_F(UcmpTest, recursiveEcmpPropagatesDown) {
  runTwoDeepRecursiveTest({{0, 0}, {5, 4}, {3, 2}}, {0, 0, 0, 0});
}

/*
 * Two interfaces: I1, I2
 * Two routes which require resolution: R1, R2
 * R1 has I1 and I2 as next hops with ECMP
 * R2 has I1 and I2 as next hops with weights 2 and 1
 * expect R1 to resolve to ECMP between I1, I2
 * expect R2 to resolve to I1:2, I2: 1
 */
TEST_F(UcmpTest, separateEcmpUcmp) {
  runRecursiveTest(
      {{UnresolvedNextHop(intfIp1, ECMP_WEIGHT),
        UnresolvedNextHop(intfIp2, ECMP_WEIGHT)},
       {UnresolvedNextHop(intfIp1, 2), UnresolvedNextHop(intfIp2, 1)}},
      {ECMP_WEIGHT, ECMP_WEIGHT});
  RouteNextHopSet route2ExpFwd;
  route2ExpFwd.emplace(
      ResolvedNextHop(IPAddress("1.1.1.10"), InterfaceID(1), 2));
  route2ExpFwd.emplace(
      ResolvedNextHop(IPAddress("2.2.2.20"), InterfaceID(2), 1));
  EXPECT_EQ(route2ExpFwd, resolvedRoutes[1]->getForwardInfo().getNextHopSet());
}

// The following set of tests will start with 4 next hops all weight 100
// then vary one next hop by 10 weight increments to 90, 80, ... , 10

TEST_F(UcmpTest, Hundred) {
  runVaryFromHundredTest(100, {1, 1, 1, 1});
}

TEST_F(UcmpTest, Ninety) {
  runVaryFromHundredTest(90, {10, 10, 10, 9});
}

TEST_F(UcmpTest, Eighty) {
  runVaryFromHundredTest(80, {5, 5, 5, 4});
}

TEST_F(UcmpTest, Seventy) {
  runVaryFromHundredTest(70, {10, 10, 10, 7});
}

TEST_F(UcmpTest, Sixty) {
  runVaryFromHundredTest(60, {5, 5, 5, 3});
}

TEST_F(UcmpTest, Fifty) {
  runVaryFromHundredTest(50, {2, 2, 2, 1});
}

TEST_F(UcmpTest, Forty) {
  runVaryFromHundredTest(40, {5, 5, 5, 2});
}

TEST_F(UcmpTest, Thirty) {
  runVaryFromHundredTest(30, {10, 10, 10, 3});
}

TEST_F(UcmpTest, Twenty) {
  runVaryFromHundredTest(20, {5, 5, 5, 1});
}

TEST_F(UcmpTest, Ten) {
  runVaryFromHundredTest(10, {10, 10, 10, 1});
}

TEST(RouteNextHopEntry, toUnicastRouteDrop) {
  folly::CIDRNetwork nw{folly::IPAddress{"1::1"}, 64};
  auto unicastRoute = util::toUnicastRoute(
      nw, RouteNextHopEntry(RouteForwardAction::DROP, DISTANCE));

  EXPECT_EQ(
      nw,
      folly::CIDRNetwork(
          facebook::network::toIPAddress(*unicastRoute.dest_ref()->ip_ref()),
          *unicastRoute.dest_ref()->prefixLength_ref()));
  EXPECT_EQ(RouteForwardAction::DROP, unicastRoute.action_ref());
  EXPECT_EQ(0, unicastRoute.nextHops_ref()->size());
}

TEST(RouteNextHopEntry, toUnicastRouteCPU) {
  folly::CIDRNetwork nw{folly::IPAddress{"1.1.1.1"}, 24};
  auto unicastRoute = util::toUnicastRoute(
      nw, RouteNextHopEntry(RouteForwardAction::TO_CPU, DISTANCE));

  EXPECT_EQ(
      nw,
      folly::CIDRNetwork(
          facebook::network::toIPAddress(*unicastRoute.dest_ref()->ip_ref()),
          *unicastRoute.dest_ref()->prefixLength_ref()));
  EXPECT_EQ(RouteForwardAction::TO_CPU, unicastRoute.action_ref());
  EXPECT_EQ(0, unicastRoute.nextHops_ref()->size());
}

TEST(RouteNextHopEntry, toUnicastRouteNhopsNonEcmp) {
  folly::CIDRNetwork nw{folly::IPAddress{"1::1"}, 64};
  RouteNextHopSet nhops = makeNextHops({"1.1.1.10"});
  auto unicastRoute =
      util::toUnicastRoute(nw, RouteNextHopEntry(nhops, DISTANCE));

  EXPECT_EQ(
      nw,
      folly::CIDRNetwork(
          facebook::network::toIPAddress(*unicastRoute.dest_ref()->ip_ref()),
          *unicastRoute.dest_ref()->prefixLength_ref()));
  EXPECT_EQ(RouteForwardAction::NEXTHOPS, unicastRoute.action_ref());
  EXPECT_EQ(1, unicastRoute.nextHops_ref()->size());
}

TEST(RouteNextHopEntry, toUnicastRouteNhopsEcmp) {
  folly::CIDRNetwork nw{folly::IPAddress{"1::1"}, 64};
  RouteNextHopSet nhops = makeNextHops({"1.1.1.10", "2::2"});
  auto unicastRoute =
      util::toUnicastRoute(nw, RouteNextHopEntry(nhops, DISTANCE));

  EXPECT_EQ(
      nw,
      folly::CIDRNetwork(
          facebook::network::toIPAddress(*unicastRoute.dest_ref()->ip_ref()),
          *unicastRoute.dest_ref()->prefixLength_ref()));
  EXPECT_EQ(RouteForwardAction::NEXTHOPS, unicastRoute.action_ref());
  EXPECT_EQ(2, unicastRoute.nextHops_ref()->size());
}
