// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/SwitchIdScopeResolver.h"
#include "fboss/agent/rib/NetworkToRouteMap.h"
#include "fboss/agent/rib/RouteUpdater.h"
#include "fboss/agent/rib/RoutingInformationBase.h"
#include "fboss/agent/state/ForwardingInformationBase.h"
#include "fboss/agent/state/MySid.h"
#include "fboss/agent/state/MySidMap.h"
#include "fboss/agent/state/Route.h"
#include "fboss/agent/state/RouteTypes.h"

#include <gtest/gtest.h>

using namespace facebook::fboss;

namespace {

HwSwitchMatcher scope() {
  return HwSwitchMatcher{std::unordered_set<SwitchID>{SwitchID(0)}};
}

folly::CIDRNetworkV6 makeSidPrefix(
    const std::string& addr = "fc00:100::1",
    uint8_t len = 48) {
  return std::make_pair(folly::IPAddressV6(addr), len);
}

std::shared_ptr<MySid> makeMySid(
    const folly::CIDRNetworkV6& prefix = makeSidPrefix(),
    MySidType type = MySidType::DECAPSULATE_AND_LOOKUP) {
  state::MySidFields fields;
  fields.type() = type;
  facebook::network::thrift::IPPrefix thriftPrefix;
  thriftPrefix.prefixAddress() =
      facebook::network::toBinaryAddress(prefix.first);
  thriftPrefix.prefixLength() = prefix.second;
  fields.mySid() = thriftPrefix;
  auto mySid = std::make_shared<MySid>(fields);
  mySid->setUnresolvedNextHop(std::nullopt);
  mySid->setResolvedNextHop(std::nullopt);
  return mySid;
}

template <typename AddrT>
std::shared_ptr<Route<AddrT>> makeResolvedRoute(
    const RoutePrefix<AddrT>& prefix) {
  auto thrift = Route<AddrT>::makeThrift(prefix);
  auto route = std::make_shared<Route<AddrT>>(thrift);
  route->setResolved(
      RouteNextHopEntry(RouteForwardAction::DROP, AdminDistance::STATIC_ROUTE));
  route->publish();
  return route;
}

template <typename AddrT>
std::shared_ptr<Route<AddrT>> makeUnresolvedRoute(
    const RoutePrefix<AddrT>& prefix) {
  auto thrift = Route<AddrT>::makeThrift(prefix);
  auto route = std::make_shared<Route<AddrT>>(thrift);
  // Route starts unresolved by default (no setResolved call)
  route->publish();
  return route;
}

RoutePrefixV4 makeV4Prefix(const std::string& addr, uint8_t mask) {
  return RoutePrefixV4{folly::IPAddressV4(addr), mask};
}

RoutePrefixV6 makeV6Prefix(const std::string& addr, uint8_t mask) {
  return RoutePrefixV6{folly::IPAddressV6(addr), mask};
}

} // namespace

TEST(ReconstructRibFromFib, EmptyFibEmptyRib) {
  auto fib = std::make_shared<ForwardingInformationBaseV4>();
  IPv4NetworkToRouteMap rib;

  reconstructRibFromFib<folly::IPAddressV4>(fib, &rib);

  EXPECT_EQ(rib.size(), 0);
}

TEST(ReconstructRibFromFib, FibPopulatesEmptyRibV4) {
  auto fib = std::make_shared<ForwardingInformationBaseV4>();

  auto prefix1 = makeV4Prefix("10.0.0.0", 24);
  auto prefix2 = makeV4Prefix("192.168.1.0", 16);
  auto route1 = makeResolvedRoute(prefix1);
  auto route2 = makeResolvedRoute(prefix2);
  fib->addNode(route1);
  fib->addNode(route2);

  IPv4NetworkToRouteMap rib;
  reconstructRibFromFib<folly::IPAddressV4>(fib, &rib);

  EXPECT_EQ(rib.size(), 2);
}

TEST(ReconstructRibFromFib, FibPopulatesEmptyRibV6) {
  auto fib = std::make_shared<ForwardingInformationBaseV6>();

  auto prefix1 = makeV6Prefix("2001:db8::", 32);
  auto prefix2 = makeV6Prefix("fc00:100::", 48);
  auto route1 = makeResolvedRoute(prefix1);
  auto route2 = makeResolvedRoute(prefix2);
  fib->addNode(route1);
  fib->addNode(route2);

  IPv6NetworkToRouteMap rib;
  reconstructRibFromFib<folly::IPAddressV6>(fib, &rib);

  EXPECT_EQ(rib.size(), 2);
}

TEST(ReconstructRibFromFib, UnresolvedRoutesPreserved) {
  // RIB has an unresolved route; FIB has a resolved route.
  // After reconstruction, both should be present.
  auto fib = std::make_shared<ForwardingInformationBaseV4>();

  auto resolvedPrefix = makeV4Prefix("10.0.0.0", 24);
  auto resolvedRoute = makeResolvedRoute(resolvedPrefix);
  fib->addNode(resolvedRoute);

  IPv4NetworkToRouteMap rib;
  auto unresolvedPrefix = makeV4Prefix("172.16.0.0", 12);
  auto unresolvedRoute = makeUnresolvedRoute(unresolvedPrefix);
  rib.insert(unresolvedPrefix, unresolvedRoute);

  reconstructRibFromFib<folly::IPAddressV4>(fib, &rib);

  // Both FIB resolved route and RIB unresolved route should be present
  EXPECT_EQ(rib.size(), 2);
}

TEST(ReconstructRibFromFib, ResolvedRoutesReplacedByFib) {
  // RIB has resolved routes that differ from FIB.
  // After reconstruction, FIB routes should replace RIB resolved routes.
  auto fib = std::make_shared<ForwardingInformationBaseV4>();

  auto prefix = makeV4Prefix("10.0.0.0", 24);
  auto fibRoute = makeResolvedRoute(prefix);
  fib->addNode(fibRoute);

  IPv4NetworkToRouteMap rib;
  // Add a different resolved route for the same prefix to RIB
  auto ribRoute = makeResolvedRoute(prefix);
  rib.insert(prefix, ribRoute);
  // Also add a route not in FIB (resolved)
  auto extraPrefix = makeV4Prefix("192.168.0.0", 16);
  auto extraRoute = makeResolvedRoute(extraPrefix);
  rib.insert(extraPrefix, extraRoute);

  reconstructRibFromFib<folly::IPAddressV4>(fib, &rib);

  // Only the FIB route should remain (extra resolved route was cleared)
  EXPECT_EQ(rib.size(), 1);
}

TEST(ReconstructRibFromFib, OnlyUnresolvedRoutesWithEmptyFib) {
  // RIB has only unresolved routes, FIB is empty.
  // After reconstruction, unresolved routes should be preserved.
  auto fib = std::make_shared<ForwardingInformationBaseV4>();

  IPv4NetworkToRouteMap rib;
  auto prefix1 = makeV4Prefix("10.0.0.0", 24);
  auto prefix2 = makeV4Prefix("172.16.0.0", 12);
  rib.insert(prefix1, makeUnresolvedRoute(prefix1));
  rib.insert(prefix2, makeUnresolvedRoute(prefix2));

  reconstructRibFromFib<folly::IPAddressV4>(fib, &rib);

  // Unresolved routes should survive
  EXPECT_EQ(rib.size(), 2);
}

TEST(ReconstructRibFromFib, MixedRibWithFibOverwrite) {
  // RIB has both resolved and unresolved routes.
  // FIB has different resolved routes.
  // After reconstruction:
  //   - Old resolved routes from RIB are gone
  //   - FIB resolved routes are imported
  //   - Unresolved routes from RIB are preserved
  auto fib = std::make_shared<ForwardingInformationBaseV6>();

  auto fibPrefix1 = makeV6Prefix("2001:db8::", 32);
  auto fibPrefix2 = makeV6Prefix("2001:db9::", 32);
  fib->addNode(makeResolvedRoute(fibPrefix1));
  fib->addNode(makeResolvedRoute(fibPrefix2));

  IPv6NetworkToRouteMap rib;
  // Resolved route in RIB that is NOT in FIB — should be removed
  auto oldResolvedPrefix = makeV6Prefix("fc00::", 16);
  rib.insert(oldResolvedPrefix, makeResolvedRoute(oldResolvedPrefix));
  // Unresolved route in RIB — should be preserved
  auto unresolvedPrefix = makeV6Prefix("fe80::", 10);
  rib.insert(unresolvedPrefix, makeUnresolvedRoute(unresolvedPrefix));

  reconstructRibFromFib<folly::IPAddressV6>(fib, &rib);

  // 2 FIB routes + 1 unresolved route = 3
  EXPECT_EQ(rib.size(), 3);
}

TEST(ReconstructRibFromFib, UnresolvedRouteOverwritesFibRouteForSamePrefix) {
  // If an unresolved route has the same prefix as a FIB route,
  // the unresolved route is re-inserted after FIB import,
  // so it overwrites the FIB route in the RIB.
  auto fib = std::make_shared<ForwardingInformationBaseV4>();

  auto sharedPrefix = makeV4Prefix("10.0.0.0", 24);
  fib->addNode(makeResolvedRoute(sharedPrefix));

  IPv4NetworkToRouteMap rib;
  auto unresolvedRoute = makeUnresolvedRoute(sharedPrefix);
  rib.insert(sharedPrefix, unresolvedRoute);

  reconstructRibFromFib<folly::IPAddressV4>(fib, &rib);

  // The unresolved route overwrites the FIB route for the same prefix
  EXPECT_EQ(rib.size(), 1);
}

// --- Tests for reconstructMySidTableFromSwitchState ---

TEST(ReconstructMySidTable, EmptyMapEmptyTable) {
  auto mySidMap = std::make_shared<MultiSwitchMySidMap>();
  MySidTable table;

  reconstructMySidTableFromSwitchState(mySidMap, &table);

  EXPECT_EQ(table.size(), 0);
}

TEST(ReconstructMySidTable, PopulatesEmptyTable) {
  auto mySidMap = std::make_shared<MultiSwitchMySidMap>();

  auto prefix1 = makeSidPrefix("fc00:100::1", 48);
  auto prefix2 = makeSidPrefix("fc00:200::1", 64);
  mySidMap->addNode(makeMySid(prefix1), scope());
  mySidMap->addNode(makeMySid(prefix2), scope());

  MySidTable table;
  reconstructMySidTableFromSwitchState(mySidMap, &table);

  EXPECT_EQ(table.size(), 2);
  EXPECT_NE(table.find(prefix1), table.end());
  EXPECT_NE(table.find(prefix2), table.end());
}

TEST(ReconstructMySidTable, ClearsExistingEntries) {
  // MySidTable has entries not in the state map.
  // After reconstruction, only entries from the state map should remain.
  auto mySidMap = std::make_shared<MultiSwitchMySidMap>();

  auto statePrefix = makeSidPrefix("fc00:100::1", 48);
  mySidMap->addNode(makeMySid(statePrefix), scope());

  MySidTable table;
  auto oldPrefix = makeSidPrefix("fc00:999::1", 48);
  table[oldPrefix] = makeMySid(oldPrefix);

  reconstructMySidTableFromSwitchState(mySidMap, &table);

  EXPECT_EQ(table.size(), 1);
  EXPECT_NE(table.find(statePrefix), table.end());
  EXPECT_EQ(table.find(oldPrefix), table.end());
}

TEST(ReconstructMySidTable, ReplacesTableWithStateEntries) {
  // MySidTable has some overlapping entries with state map.
  // After reconstruction, table should exactly match state map.
  auto mySidMap = std::make_shared<MultiSwitchMySidMap>();

  auto prefix1 = makeSidPrefix("fc00:100::1", 48);
  auto prefix2 = makeSidPrefix("fc00:200::1", 64);
  mySidMap->addNode(makeMySid(prefix1), scope());
  mySidMap->addNode(makeMySid(prefix2), scope());

  MySidTable table;
  // Overlapping entry
  table[prefix1] = makeMySid(prefix1);
  // Extra entry not in state
  auto extraPrefix = makeSidPrefix("fc00:300::1", 48);
  table[extraPrefix] = makeMySid(extraPrefix);

  reconstructMySidTableFromSwitchState(mySidMap, &table);

  EXPECT_EQ(table.size(), 2);
  EXPECT_NE(table.find(prefix1), table.end());
  EXPECT_NE(table.find(prefix2), table.end());
  EXPECT_EQ(table.find(extraPrefix), table.end());
}

TEST(ReconstructMySidTable, EmptyMapClearsTable) {
  auto mySidMap = std::make_shared<MultiSwitchMySidMap>();

  MySidTable table;
  auto prefix1 = makeSidPrefix("fc00:100::1", 48);
  auto prefix2 = makeSidPrefix("fc00:200::1", 64);
  table[prefix1] = makeMySid(prefix1);
  table[prefix2] = makeMySid(prefix2);

  reconstructMySidTableFromSwitchState(mySidMap, &table);

  EXPECT_EQ(table.size(), 0);
}

TEST(ReconstructMySidTable, PreservesMySidType) {
  auto mySidMap = std::make_shared<MultiSwitchMySidMap>();

  auto prefix = makeSidPrefix("fc00:100::1", 48);
  auto mySid = makeMySid(prefix, MySidType::DECAPSULATE_AND_LOOKUP);
  mySidMap->addNode(mySid, scope());

  MySidTable table;
  reconstructMySidTableFromSwitchState(mySidMap, &table);

  ASSERT_EQ(table.size(), 1);
  EXPECT_EQ(table[prefix]->getType(), MySidType::DECAPSULATE_AND_LOOKUP);
}
