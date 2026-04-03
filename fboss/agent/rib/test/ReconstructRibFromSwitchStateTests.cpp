// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gtest/gtest.h>
#include "fboss/agent/SwitchIdScopeResolver.h"
#include "fboss/agent/rib/NetworkToRouteMap.h"
#include "fboss/agent/rib/NextHopIDManager.h"
#include "fboss/agent/rib/RouteUpdater.h"
#include "fboss/agent/rib/RoutingInformationBase.h"
#include "fboss/agent/state/FibInfo.h"
#include "fboss/agent/state/FibInfoMap.h"
#include "fboss/agent/state/ForwardingInformationBase.h"
#include "fboss/agent/state/ForwardingInformationBaseMap.h"
#include "fboss/agent/state/MySid.h"
#include "fboss/agent/state/MySidMap.h"
#include "fboss/agent/state/NextHopIdMaps.h"
#include "fboss/agent/state/Route.h"
#include "fboss/agent/state/RouteTypes.h"
#include "fboss/agent/test/TestUtils.h"

using namespace facebook::fboss;

DECLARE_bool(enable_nexthop_id_manager);

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
  mySid->setUnresolveNextHopsId(std::nullopt);
  mySid->setResolvedNextHopsId(std::nullopt);
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

// --- Tests for fromThrift with mySidMap ---

TEST(FromThriftWithMySid, NullMySidMap) {
  std::map<int32_t, state::RouteTableFields> ribThrift;
  ribThrift.emplace(0, state::RouteTableFields{});

  auto rib =
      RoutingInformationBase::fromThrift(ribThrift, nullptr, nullptr, nullptr);

  EXPECT_EQ(rib->getMySidTableCopy().size(), 0);
}

TEST(FromThriftWithMySid, EmptyMySidMap) {
  std::map<int32_t, state::RouteTableFields> ribThrift;
  ribThrift.emplace(0, state::RouteTableFields{});

  auto mySidMap = std::make_shared<MultiSwitchMySidMap>();
  auto rib =
      RoutingInformationBase::fromThrift(ribThrift, nullptr, nullptr, mySidMap);

  EXPECT_EQ(rib->getMySidTableCopy().size(), 0);
}

TEST(FromThriftWithMySid, PopulatesMySidTable) {
  std::map<int32_t, state::RouteTableFields> ribThrift;
  ribThrift.emplace(0, state::RouteTableFields{});

  auto mySidMap = std::make_shared<MultiSwitchMySidMap>();
  auto prefix1 = makeSidPrefix("fc00:100::1", 48);
  auto prefix2 = makeSidPrefix("fc00:200::1", 64);
  mySidMap->addNode(makeMySid(prefix1), scope());
  mySidMap->addNode(makeMySid(prefix2), scope());

  auto rib =
      RoutingInformationBase::fromThrift(ribThrift, nullptr, nullptr, mySidMap);

  auto mySidTableCopy = rib->getMySidTableCopy();
  EXPECT_EQ(mySidTableCopy.size(), 2);
  EXPECT_NE(mySidTableCopy.find(prefix1), mySidTableCopy.end());
  EXPECT_NE(mySidTableCopy.find(prefix2), mySidTableCopy.end());
}

TEST(FromThriftWithMySid, PreservesMySidType) {
  std::map<int32_t, state::RouteTableFields> ribThrift;
  ribThrift.emplace(0, state::RouteTableFields{});

  auto mySidMap = std::make_shared<MultiSwitchMySidMap>();
  auto prefix = makeSidPrefix("fc00:100::1", 48);
  mySidMap->addNode(
      makeMySid(prefix, MySidType::DECAPSULATE_AND_LOOKUP), scope());

  auto rib =
      RoutingInformationBase::fromThrift(ribThrift, nullptr, nullptr, mySidMap);

  auto mySidTableCopy = rib->getMySidTableCopy();
  ASSERT_EQ(mySidTableCopy.size(), 1);
  EXPECT_EQ(*mySidTableCopy[prefix].type(), MySidType::DECAPSULATE_AND_LOOKUP);
}

// --- Tests for fromThrift initializing nextHopIDManager_ via fibsInfoMap ---

// Verifies that fromThrift with a real fibsInfoMap (containing nexthop id maps)
// and a mySidMap (with a MySid referencing a nexthop set) correctly populates
// the nextHopIDManager via reconstructFromSwitchStateMaps. This covers the
// warm-boot path where MySid entries have nexthop sets not referenced by
// routes.
TEST(FromThriftWithFibsInfoMap, NextHopIDManagerPopulatedViaMySid) {
  FLAGS_enable_nexthop_id_manager = true;
  constexpr int64_t kNhopId = 1;
  constexpr int64_t kSetId = 1LL << 62;

  // Build FibInfo with id maps containing one nexthop and one nexthop set
  NextHop nhop =
      makeResolvedNextHop(InterfaceID(1), "10.0.0.1", UCMP_DEFAULT_WEIGHT);
  auto idToNhopMap = std::make_shared<IdToNextHopMap>();
  idToNhopMap->addNextHop(kNhopId, nhop.toThrift());

  auto idToNhopIdSetMap = std::make_shared<IdToNextHopIdSetMap>();
  idToNhopIdSetMap->addNextHopIdSet(kSetId, {kNhopId});

  auto fibInfo = std::make_shared<FibInfo>();
  fibInfo->resetFibsMap(std::make_shared<ForwardingInformationBaseMap>());
  fibInfo->setIdToNextHopMap(idToNhopMap);
  fibInfo->setIdToNextHopIdSetMap(idToNhopIdSetMap);

  auto fibsInfoMap = std::make_shared<MultiSwitchFibInfoMap>();
  fibsInfoMap->addNode("id=0", fibInfo);

  // Build MySid whose resolvedNextHopsId references the nexthop set above
  auto mySid = makeMySid();
  mySid->setResolvedNextHopsId(NextHopSetID(kSetId));
  auto mySidMap = std::make_shared<MultiSwitchMySidMap>();
  mySidMap->addNode(mySid, scope());

  std::map<int32_t, state::RouteTableFields> ribThrift;
  ribThrift.emplace(0, state::RouteTableFields{});

  auto rib = RoutingInformationBase::fromThrift(
      ribThrift, fibsInfoMap, nullptr, mySidMap);

  const auto* idManager = rib->getNextHopIDManager();
  ASSERT_NE(idManager, nullptr);
  EXPECT_NE(
      idManager->getIdToNextHop().find(NextHopID(kNhopId)),
      idManager->getIdToNextHop().end());
  EXPECT_NE(
      idManager->getIdToNextHopIdSet().find(NextHopSetID(kSetId)),
      idManager->getIdToNextHopIdSet().end());
}
