/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <gtest/gtest.h>

#include <folly/logging/xlog.h>
#include <gflags/gflags.h>

#include "fboss/agent/AddressUtil.h"
#include "fboss/agent/SwSwitchRouteUpdateWrapper.h"
#include "fboss/agent/if/gen-cpp2/common_types.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/agent/rib/NextHopIDManager.h"
#include "fboss/agent/rib/RoutingInformationBase.h"
#include "fboss/agent/state/RouteNextHop.h"
#include "fboss/agent/test/HwTestHandle.h"
#include "fboss/agent/test/TestUtils.h"

DECLARE_bool(enable_nexthop_id_manager);

namespace facebook::fboss {

class NamedNextHopGroupRibTest : public ::testing::Test {
 public:
  void SetUp() override {
    FLAGS_enable_nexthop_id_manager = true;
    auto config = initialConfig();
    handle_ = createTestHandle(&config);
    sw_ = handle_->getSw();
  }

  cfg::SwitchConfig initialConfig() const {
    cfg::SwitchConfig config;
    config.switchSettings()->switchIdToSwitchInfo() = {
        {0, createSwitchInfo(cfg::SwitchType::NPU)}};
    config.vlans()->resize(2);
    config.vlans()[0].id() = 1;
    config.vlans()[1].id() = 2;

    config.interfaces()->resize(2);
    config.interfaces()[0].intfID() = 1;
    config.interfaces()[0].vlanID() = 1;
    config.interfaces()[0].routerID() = 0;
    config.interfaces()[0].mac() = "00:00:00:00:00:11";
    config.interfaces()[0].ipAddresses()->resize(2);
    config.interfaces()[0].ipAddresses()[0] = "1.1.1.1/24";
    config.interfaces()[0].ipAddresses()[1] = "1::1/48";

    config.interfaces()[1].intfID() = 2;
    config.interfaces()[1].vlanID() = 2;
    config.interfaces()[1].routerID() = 0;
    config.interfaces()[1].mac() = "00:00:00:00:00:22";
    config.interfaces()[1].ipAddresses()->resize(2);
    config.interfaces()[1].ipAddresses()[0] = "2.2.2.2/24";
    config.interfaces()[1].ipAddresses()[1] = "2::1/48";
    return config;
  }

 protected:
  RouteNextHopSet makeResolvedNextHops(
      const std::vector<std::string>& nexthopStrs) {
    RouteNextHopSet nhops;
    for (size_t i = 0; i < nexthopStrs.size(); ++i) {
      nhops.emplace(UnresolvedNextHop(folly::IPAddress(nexthopStrs[i]), 1));
    }
    return nhops;
  }

  void addOrUpdateGroups(
      const std::vector<std::pair<std::string, RouteNextHopSet>>& groups) {
    auto rib = sw_->getRib();
    rib->addOrUpdateNamedNextHopGroups(
        sw_->getScopeResolver(), groups, createRibToSwitchStateFunction(), sw_);
  }

  SwSwitch* sw_ = nullptr;
  std::unique_ptr<HwTestHandle> handle_;
};

TEST_F(NamedNextHopGroupRibTest, AddNamedNextHopGroups) {
  auto rib = sw_->getRib();
  ASSERT_NE(rib, nullptr);

  std::vector<std::pair<std::string, RouteNextHopSet>> groups;
  groups.emplace_back("group1", makeResolvedNextHops({"1.1.1.10", "2.2.2.10"}));
  groups.emplace_back("group2", makeResolvedNextHops({"1.1.1.20"}));

  addOrUpdateGroups(groups);

  auto managerCopy = rib->getNextHopIDManagerCopy();
  ASSERT_NE(managerCopy, nullptr);
  EXPECT_TRUE(managerCopy->hasNamedNextHopGroup("group1"));
  EXPECT_TRUE(managerCopy->hasNamedNextHopGroup("group2"));
  auto setId1 = managerCopy->getNextHopSetIDForName("group1");
  auto setId2 = managerCopy->getNextHopSetIDForName("group2");
  EXPECT_TRUE(setId1.has_value());
  EXPECT_TRUE(setId2.has_value());
  EXPECT_NE(*setId1, *setId2);
}

TEST_F(NamedNextHopGroupRibTest, UpdateNamedNextHopGroup) {
  auto rib = sw_->getRib();

  // First add a group
  std::vector<std::pair<std::string, RouteNextHopSet>> groups;
  groups.emplace_back("group1", makeResolvedNextHops({"1.1.1.10"}));
  addOrUpdateGroups(groups);

  // Update with new nexthops (allocateNamedNextHopGroup handles both
  // add/update)
  std::vector<std::pair<std::string, RouteNextHopSet>> updatedGroups;
  updatedGroups.emplace_back(
      "group1", makeResolvedNextHops({"1.1.1.10", "2.2.2.10"}));

  addOrUpdateGroups(updatedGroups);

  auto managerCopy = rib->getNextHopIDManagerCopy();
  ASSERT_NE(managerCopy, nullptr);
  EXPECT_TRUE(managerCopy->hasNamedNextHopGroup("group1"));
  auto nhops = managerCopy->getNextHopsForName("group1");
  ASSERT_TRUE(nhops.has_value());
  EXPECT_EQ(nhops->size(), 2);
}

TEST_F(NamedNextHopGroupRibTest, DeleteNamedNextHopGroups) {
  auto rib = sw_->getRib();
  ASSERT_NE(rib, nullptr);

  // Add groups first
  std::vector<std::pair<std::string, RouteNextHopSet>> groups;
  groups.emplace_back("group1", makeResolvedNextHops({"1.1.1.10"}));
  groups.emplace_back("group2", makeResolvedNextHops({"2.2.2.10"}));
  addOrUpdateGroups(groups);

  // Delete one group
  bool deleteCalled = false;
  rib->deleteNamedNextHopGroups(
      {"group1"}, [&](const NextHopIDManager* manager) {
        deleteCalled = true;
        ASSERT_NE(manager, nullptr);
        EXPECT_FALSE(manager->hasNamedNextHopGroup("group1"));
        EXPECT_TRUE(manager->hasNamedNextHopGroup("group2"));
      });

  EXPECT_TRUE(deleteCalled);

  // Verify through copy
  auto managerCopy = rib->getNextHopIDManagerCopy();
  EXPECT_FALSE(managerCopy->hasNamedNextHopGroup("group1"));
  EXPECT_TRUE(managerCopy->hasNamedNextHopGroup("group2"));
}

TEST_F(NamedNextHopGroupRibTest, DeleteNonExistentGroupIsNoOp) {
  auto rib = sw_->getRib();
  ASSERT_NE(rib, nullptr);

  // Delete a group that doesn't exist — should not throw
  bool stateUpdateCalled = false;
  rib->deleteNamedNextHopGroups(
      {"nonexistent"}, [&](const NextHopIDManager* manager) {
        stateUpdateCalled = true;
        EXPECT_NE(manager, nullptr);
      });

  EXPECT_TRUE(stateUpdateCalled);
}

TEST_F(NamedNextHopGroupRibTest, AddAndDeleteMultipleGroups) {
  auto rib = sw_->getRib();
  ASSERT_NE(rib, nullptr);

  // Add multiple groups
  std::vector<std::pair<std::string, RouteNextHopSet>> groups;
  groups.emplace_back("g1", makeResolvedNextHops({"1.1.1.10"}));
  groups.emplace_back("g2", makeResolvedNextHops({"2.2.2.10"}));
  groups.emplace_back("g3", makeResolvedNextHops({"1.1.1.10", "2.2.2.10"}));
  addOrUpdateGroups(groups);

  // Delete all at once
  bool deleteCalled = false;
  rib->deleteNamedNextHopGroups(
      {"g1", "g2", "g3"}, [&](const NextHopIDManager* manager) {
        deleteCalled = true;
        EXPECT_FALSE(manager->hasNamedNextHopGroup("g1"));
        EXPECT_FALSE(manager->hasNamedNextHopGroup("g2"));
        EXPECT_FALSE(manager->hasNamedNextHopGroup("g3"));
      });

  EXPECT_TRUE(deleteCalled);
}

TEST_F(NamedNextHopGroupRibTest, invalidEntryInBatchDoesNotPartiallyMutate) {
  // Verify that if any entry in the batch is invalid, pre-validation throws
  // before any state is mutated. This matches the MySid batch validation
  // pattern (invalidEntryInBatchDoesNotPartiallyMutateMySidTable).
  auto rib = sw_->getRib();
  ASSERT_NE(rib, nullptr);

  // Seed the table with a known-good entry
  std::vector<std::pair<std::string, RouteNextHopSet>> seed;
  seed.emplace_back("existing", makeResolvedNextHops({"1.1.1.10"}));
  addOrUpdateGroups(seed);

  // Batch: valid group first, then invalid group with empty nexthop set.
  // Without pre-validation, the valid group would be allocated before the
  // invalid group throws, leaving partial state.
  std::vector<std::pair<std::string, RouteNextHopSet>> batch;
  batch.emplace_back("validNew", makeResolvedNextHops({"2.2.2.10"}));
  batch.emplace_back("badGroup", RouteNextHopSet{}); // invalid: empty

  EXPECT_THROW(addOrUpdateGroups(batch), FbossError);

  // Only the seed entry should exist — the valid entry from the failed
  // batch must not have been inserted (no partial mutation)
  auto manager = rib->getNextHopIDManagerCopy();
  EXPECT_TRUE(manager->hasNamedNextHopGroup("existing"));
  EXPECT_FALSE(manager->hasNamedNextHopGroup("validNew"));
  EXPECT_FALSE(manager->hasNamedNextHopGroup("badGroup"));
}

TEST_F(NamedNextHopGroupRibTest, emptyNameInBatchDoesNotPartiallyMutate) {
  auto rib = sw_->getRib();

  // Seed the table with a known-good entry
  std::vector<std::pair<std::string, RouteNextHopSet>> seed;
  seed.emplace_back("existing", makeResolvedNextHops({"1.1.1.10"}));
  addOrUpdateGroups(seed);

  // Batch: valid group first, then invalid group with empty name
  std::vector<std::pair<std::string, RouteNextHopSet>> batch;
  batch.emplace_back("validNew", makeResolvedNextHops({"2.2.2.10"}));
  batch.emplace_back(
      "", makeResolvedNextHops({"1.1.1.20"})); // invalid: empty name

  EXPECT_THROW(addOrUpdateGroups(batch), FbossError);

  // Only the seed entry should exist — no partial mutation
  auto manager = rib->getNextHopIDManagerCopy();
  EXPECT_TRUE(manager->hasNamedNextHopGroup("existing"));
  EXPECT_FALSE(manager->hasNamedNextHopGroup("validNew"));
}

TEST_F(NamedNextHopGroupRibTest, AddRouteWithNamedNhg) {
  auto rib = sw_->getRib();

  std::vector<std::pair<std::string, RouteNextHopSet>> groups;
  groups.emplace_back("nhg1", makeResolvedNextHops({"1.1.1.10", "2.2.2.10"}));
  addOrUpdateGroups(groups);

  UnicastRoute route;
  route.dest()->ip() =
      facebook::network::toBinaryAddress(folly::IPAddress("10.0.0.0"));
  route.dest()->prefixLength() = 24;
  NamedRouteDestination namedDest;
  namedDest.nextHopGroup() = "nhg1";
  route.namedRouteDestination() = namedDest;

  auto updater = sw_->getRouteUpdater();
  updater.addRoute(RouterID(0), ClientID::BGPD, route);
  updater.program();

  auto managerCopy = rib->getNextHopIDManagerCopy();
  EXPECT_TRUE(managerCopy->hasRoutesForNamedNhg("nhg1"));
  auto expectedPrefix = folly::CIDRNetwork(folly::IPAddress("10.0.0.0"), 24);
  EXPECT_EQ(
      managerCopy->getRoutesForNamedNhg("nhg1").count(
          {RouterID(0), expectedPrefix}),
      1);
}

TEST_F(NamedNextHopGroupRibTest, AddRouteWithNonExistentNhgFails) {
  UnicastRoute route;
  route.dest()->ip() =
      facebook::network::toBinaryAddress(folly::IPAddress("10.0.0.0"));
  route.dest()->prefixLength() = 24;
  NamedRouteDestination namedDest;
  namedDest.nextHopGroup() = "nonexistent";
  route.namedRouteDestination() = namedDest;

  auto updater = sw_->getRouteUpdater();
  updater.addRoute(RouterID(0), ClientID::BGPD, route);
  EXPECT_THROW(updater.program(), FbossError);
}

TEST_F(NamedNextHopGroupRibTest, AddRouteWithBothNextHopsAndNamedNhgFails) {
  std::vector<std::pair<std::string, RouteNextHopSet>> groups;
  groups.emplace_back("nhg1", makeResolvedNextHops({"1.1.1.10"}));
  addOrUpdateGroups(groups);

  UnicastRoute route;
  route.dest()->ip() =
      facebook::network::toBinaryAddress(folly::IPAddress("10.0.0.0"));
  route.dest()->prefixLength() = 24;

  NextHopThrift nh;
  nh.address() =
      facebook::network::toBinaryAddress(folly::IPAddress("1.1.1.10"));
  *nh.weight() = 1;
  route.nextHops() = {nh};

  NamedRouteDestination namedDest;
  namedDest.nextHopGroup() = "nhg1";
  route.namedRouteDestination() = namedDest;

  auto updater = sw_->getRouteUpdater();
  updater.addRoute(RouterID(0), ClientID::BGPD, route);
  EXPECT_THROW(updater.program(), FbossError);
}

TEST_F(NamedNextHopGroupRibTest, ReAddRouteWithDifferentNamedNhg) {
  auto rib = sw_->getRib();

  std::vector<std::pair<std::string, RouteNextHopSet>> groups;
  groups.emplace_back("nhg1", makeResolvedNextHops({"1.1.1.10"}));
  groups.emplace_back("nhg2", makeResolvedNextHops({"2.2.2.10"}));
  addOrUpdateGroups(groups);

  auto prefix = folly::CIDRNetwork(folly::IPAddress("10.0.0.0"), 24);

  // Add route with nhg1
  {
    UnicastRoute route;
    route.dest()->ip() =
        facebook::network::toBinaryAddress(folly::IPAddress("10.0.0.0"));
    route.dest()->prefixLength() = 24;
    NamedRouteDestination namedDest;
    namedDest.nextHopGroup() = "nhg1";
    route.namedRouteDestination() = namedDest;
    auto updater = sw_->getRouteUpdater();
    updater.addRoute(RouterID(0), ClientID::BGPD, route);
    updater.program();
  }

  auto managerCopy = rib->getNextHopIDManagerCopy();
  EXPECT_TRUE(managerCopy->hasRoutesForNamedNhg("nhg1"));
  EXPECT_FALSE(managerCopy->hasRoutesForNamedNhg("nhg2"));

  // Re-add the same route with nhg2
  {
    UnicastRoute route;
    route.dest()->ip() =
        facebook::network::toBinaryAddress(folly::IPAddress("10.0.0.0"));
    route.dest()->prefixLength() = 24;
    NamedRouteDestination namedDest;
    namedDest.nextHopGroup() = "nhg2";
    route.namedRouteDestination() = namedDest;
    auto updater = sw_->getRouteUpdater();
    updater.addRoute(RouterID(0), ClientID::BGPD, route);
    updater.program();
  }

  managerCopy = rib->getNextHopIDManagerCopy();
  EXPECT_FALSE(managerCopy->hasRoutesForNamedNhg("nhg1"));
  EXPECT_TRUE(managerCopy->hasRoutesForNamedNhg("nhg2"));
  EXPECT_EQ(
      managerCopy->getRoutesForNamedNhg("nhg2").count({RouterID(0), prefix}),
      1);
}

TEST_F(NamedNextHopGroupRibTest, DeleteRouteWithNamedNhgCleansUpMapping) {
  auto rib = sw_->getRib();

  std::vector<std::pair<std::string, RouteNextHopSet>> groups;
  groups.emplace_back("nhg1", makeResolvedNextHops({"1.1.1.10"}));
  addOrUpdateGroups(groups);

  // Add route with named NHG
  {
    UnicastRoute route;
    route.dest()->ip() =
        facebook::network::toBinaryAddress(folly::IPAddress("10.0.0.0"));
    route.dest()->prefixLength() = 24;
    NamedRouteDestination namedDest;
    namedDest.nextHopGroup() = "nhg1";
    route.namedRouteDestination() = namedDest;
    auto updater = sw_->getRouteUpdater();
    updater.addRoute(RouterID(0), ClientID::BGPD, route);
    updater.program();
  }

  auto managerCopy = rib->getNextHopIDManagerCopy();
  EXPECT_TRUE(managerCopy->hasRoutesForNamedNhg("nhg1"));

  // Delete the route
  {
    auto updater = sw_->getRouteUpdater();
    updater.delRoute(
        RouterID(0), folly::IPAddress("10.0.0.0"), 24, ClientID::BGPD);
    updater.program();
  }

  managerCopy = rib->getNextHopIDManagerCopy();
  EXPECT_FALSE(managerCopy->hasRoutesForNamedNhg("nhg1"));
}

TEST_F(NamedNextHopGroupRibTest, SyncFibCleansUpNamedNhgMapping) {
  auto rib = sw_->getRib();

  std::vector<std::pair<std::string, RouteNextHopSet>> groups;
  groups.emplace_back("nhg1", makeResolvedNextHops({"1.1.1.10"}));
  groups.emplace_back("nhg2", makeResolvedNextHops({"2.2.2.10"}));
  addOrUpdateGroups(groups);

  // Add two routes with named NHGs
  {
    UnicastRoute route1;
    route1.dest()->ip() =
        facebook::network::toBinaryAddress(folly::IPAddress("10.0.0.0"));
    route1.dest()->prefixLength() = 24;
    NamedRouteDestination namedDest1;
    namedDest1.nextHopGroup() = "nhg1";
    route1.namedRouteDestination() = namedDest1;

    UnicastRoute route2;
    route2.dest()->ip() =
        facebook::network::toBinaryAddress(folly::IPAddress("10.1.0.0"));
    route2.dest()->prefixLength() = 24;
    NamedRouteDestination namedDest2;
    namedDest2.nextHopGroup() = "nhg2";
    route2.namedRouteDestination() = namedDest2;

    auto updater = sw_->getRouteUpdater();
    updater.addRoute(RouterID(0), ClientID::BGPD, route1);
    updater.addRoute(RouterID(0), ClientID::BGPD, route2);
    updater.program();
  }

  auto managerCopy = rib->getNextHopIDManagerCopy();
  EXPECT_TRUE(managerCopy->hasRoutesForNamedNhg("nhg1"));
  EXPECT_TRUE(managerCopy->hasRoutesForNamedNhg("nhg2"));

  // syncFib with only route1 — route2 should be cleaned up
  {
    UnicastRoute route1;
    route1.dest()->ip() =
        facebook::network::toBinaryAddress(folly::IPAddress("10.0.0.0"));
    route1.dest()->prefixLength() = 24;
    NamedRouteDestination namedDest1;
    namedDest1.nextHopGroup() = "nhg1";
    route1.namedRouteDestination() = namedDest1;

    auto updater = sw_->getRouteUpdater();
    updater.addRoute(RouterID(0), ClientID::BGPD, route1);
    RouteUpdateWrapper::SyncFibInfo syncInfo;
    syncInfo.ridAndClients.insert({RouterID(0), ClientID::BGPD});
    syncInfo.type = RouteUpdateWrapper::SyncFibInfo::SyncFibType::IP_ONLY;
    updater.program(syncInfo);
  }

  managerCopy = rib->getNextHopIDManagerCopy();
  EXPECT_TRUE(managerCopy->hasRoutesForNamedNhg("nhg1"));
  EXPECT_FALSE(managerCopy->hasRoutesForNamedNhg("nhg2"));
}

TEST_F(NamedNextHopGroupRibTest, DeleteNhgWithRouteReferenceBlocked) {
  auto rib = sw_->getRib();

  std::vector<std::pair<std::string, RouteNextHopSet>> groups;
  groups.emplace_back("nhg1", makeResolvedNextHops({"1.1.1.10"}));
  addOrUpdateGroups(groups);

  // Add route referencing nhg1
  {
    UnicastRoute route;
    route.dest()->ip() =
        facebook::network::toBinaryAddress(folly::IPAddress("10.0.0.0"));
    route.dest()->prefixLength() = 24;
    NamedRouteDestination namedDest;
    namedDest.nextHopGroup() = "nhg1";
    route.namedRouteDestination() = namedDest;
    auto updater = sw_->getRouteUpdater();
    updater.addRoute(RouterID(0), ClientID::BGPD, route);
    updater.program();
  }

  // Deleting nhg1 should fail
  EXPECT_THROW(
      rib->deleteNamedNextHopGroups({"nhg1"}, [](const NextHopIDManager*) {}),
      FbossError);

  // nhg1 should still exist
  auto managerCopy = rib->getNextHopIDManagerCopy();
  EXPECT_TRUE(managerCopy->hasNamedNextHopGroup("nhg1"));
}

TEST_F(NamedNextHopGroupRibTest, DeleteNhgAfterRouteRemovedSucceeds) {
  auto rib = sw_->getRib();

  std::vector<std::pair<std::string, RouteNextHopSet>> groups;
  groups.emplace_back("nhg1", makeResolvedNextHops({"1.1.1.10"}));
  addOrUpdateGroups(groups);

  // Add and then delete a route referencing nhg1
  {
    UnicastRoute route;
    route.dest()->ip() =
        facebook::network::toBinaryAddress(folly::IPAddress("10.0.0.0"));
    route.dest()->prefixLength() = 24;
    NamedRouteDestination namedDest;
    namedDest.nextHopGroup() = "nhg1";
    route.namedRouteDestination() = namedDest;
    auto updater = sw_->getRouteUpdater();
    updater.addRoute(RouterID(0), ClientID::BGPD, route);
    updater.program();
  }
  {
    auto updater = sw_->getRouteUpdater();
    updater.delRoute(
        RouterID(0), folly::IPAddress("10.0.0.0"), 24, ClientID::BGPD);
    updater.program();
  }

  // Now deletion should succeed
  rib->deleteNamedNextHopGroups({"nhg1"}, [](const NextHopIDManager*) {});

  auto managerCopy = rib->getNextHopIDManagerCopy();
  EXPECT_FALSE(managerCopy->hasNamedNextHopGroup("nhg1"));
}

TEST_F(NamedNextHopGroupRibTest, UpdateNhgReprogramsRoutes) {
  auto rib = sw_->getRib();

  std::vector<std::pair<std::string, RouteNextHopSet>> groups;
  groups.emplace_back("nhg1", makeResolvedNextHops({"1.1.1.10"}));
  addOrUpdateGroups(groups);

  auto prefix = folly::CIDRNetwork(folly::IPAddress("10.0.0.0"), 24);

  // Add route referencing nhg1
  {
    UnicastRoute route;
    route.dest()->ip() =
        facebook::network::toBinaryAddress(folly::IPAddress("10.0.0.0"));
    route.dest()->prefixLength() = 24;
    NamedRouteDestination namedDest;
    namedDest.nextHopGroup() = "nhg1";
    route.namedRouteDestination() = namedDest;
    auto updater = sw_->getRouteUpdater();
    updater.addRoute(RouterID(0), ClientID::BGPD, route);
    updater.program();
  }

  // Update nhg1 with different nexthops
  std::vector<std::pair<std::string, RouteNextHopSet>> updatedGroups;
  updatedGroups.emplace_back("nhg1", makeResolvedNextHops({"2.2.2.10"}));
  addOrUpdateGroups(updatedGroups);

  // Verify the NHG was updated with new nexthops
  auto managerCopy = rib->getNextHopIDManagerCopy();
  auto nhops = managerCopy->getNextHopsForName("nhg1");
  ASSERT_TRUE(nhops.has_value());
  EXPECT_EQ(nhops->size(), 1);
  EXPECT_EQ(
      nhops->count(UnresolvedNextHop(folly::IPAddress("2.2.2.10"), 1)), 1);

  // Route should still reference nhg1
  EXPECT_EQ(
      managerCopy->getRoutesForNamedNhg("nhg1").count({RouterID(0), prefix}),
      1);
}

TEST_F(NamedNextHopGroupRibTest, UpdateNhgMultipleRoutesReprogrammed) {
  auto rib = sw_->getRib();

  std::vector<std::pair<std::string, RouteNextHopSet>> groups;
  groups.emplace_back("nhg1", makeResolvedNextHops({"1.1.1.10"}));
  addOrUpdateGroups(groups);

  auto prefix1 = folly::CIDRNetwork(folly::IPAddress("10.0.0.0"), 24);
  auto prefix2 = folly::CIDRNetwork(folly::IPAddress("10.1.0.0"), 24);

  // Add two routes referencing nhg1
  {
    UnicastRoute route1;
    route1.dest()->ip() =
        facebook::network::toBinaryAddress(folly::IPAddress("10.0.0.0"));
    route1.dest()->prefixLength() = 24;
    NamedRouteDestination namedDest;
    namedDest.nextHopGroup() = "nhg1";
    route1.namedRouteDestination() = namedDest;

    UnicastRoute route2;
    route2.dest()->ip() =
        facebook::network::toBinaryAddress(folly::IPAddress("10.1.0.0"));
    route2.dest()->prefixLength() = 24;
    route2.namedRouteDestination() = namedDest;

    auto updater = sw_->getRouteUpdater();
    updater.addRoute(RouterID(0), ClientID::BGPD, route1);
    updater.addRoute(RouterID(0), ClientID::BGPD, route2);
    updater.program();
  }

  auto managerCopy = rib->getNextHopIDManagerCopy();
  EXPECT_EQ(managerCopy->getRoutesForNamedNhg("nhg1").size(), 2);

  // Update nhg1
  std::vector<std::pair<std::string, RouteNextHopSet>> updatedGroups;
  updatedGroups.emplace_back(
      "nhg1", makeResolvedNextHops({"2.2.2.10", "1.1.1.10"}));
  addOrUpdateGroups(updatedGroups);

  // Both routes should still reference nhg1 with correct prefixes
  managerCopy = rib->getNextHopIDManagerCopy();
  const auto& routes = managerCopy->getRoutesForNamedNhg("nhg1");
  EXPECT_EQ(routes.size(), 2);
  EXPECT_EQ(routes.count({RouterID(0), prefix1}), 1);
  EXPECT_EQ(routes.count({RouterID(0), prefix2}), 1);

  // Verify NHG nexthops were updated
  auto nhops = managerCopy->getNextHopsForName("nhg1");
  ASSERT_TRUE(nhops.has_value());
  EXPECT_EQ(nhops->size(), 2);
}

TEST_F(NamedNextHopGroupRibTest, BatchUpdateAndNewNhg) {
  auto rib = sw_->getRib();

  std::vector<std::pair<std::string, RouteNextHopSet>> groups;
  groups.emplace_back("nhg1", makeResolvedNextHops({"1.1.1.10"}));
  addOrUpdateGroups(groups);

  auto prefix = folly::CIDRNetwork(folly::IPAddress("10.0.0.0"), 24);

  // Add route referencing nhg1
  {
    UnicastRoute route;
    route.dest()->ip() =
        facebook::network::toBinaryAddress(folly::IPAddress("10.0.0.0"));
    route.dest()->prefixLength() = 24;
    NamedRouteDestination namedDest;
    namedDest.nextHopGroup() = "nhg1";
    route.namedRouteDestination() = namedDest;
    auto updater = sw_->getRouteUpdater();
    updater.addRoute(RouterID(0), ClientID::BGPD, route);
    updater.program();
  }

  // Batch: update nhg1 (has route) + add nhg2 (new, no routes)
  std::vector<std::pair<std::string, RouteNextHopSet>> batchGroups;
  batchGroups.emplace_back("nhg1", makeResolvedNextHops({"2.2.2.10"}));
  batchGroups.emplace_back(
      "nhg2", makeResolvedNextHops({"1.1.1.20", "2.2.2.20"}));
  addOrUpdateGroups(batchGroups);

  auto managerCopy = rib->getNextHopIDManagerCopy();

  // nhg1 should be updated with new nexthops
  auto nhops1 = managerCopy->getNextHopsForName("nhg1");
  ASSERT_TRUE(nhops1.has_value());
  EXPECT_EQ(nhops1->size(), 1);
  EXPECT_EQ(
      nhops1->count(UnresolvedNextHop(folly::IPAddress("2.2.2.10"), 1)), 1);

  // Route should still reference nhg1
  EXPECT_EQ(
      managerCopy->getRoutesForNamedNhg("nhg1").count({RouterID(0), prefix}),
      1);

  // nhg2 should exist with its nexthops
  EXPECT_TRUE(managerCopy->hasNamedNextHopGroup("nhg2"));
  auto nhops2 = managerCopy->getNextHopsForName("nhg2");
  ASSERT_TRUE(nhops2.has_value());
  EXPECT_EQ(nhops2->size(), 2);
}

TEST_F(NamedNextHopGroupRibTest, RouteEntryReportsNamedNhg) {
  auto rib = sw_->getRib();

  std::vector<std::pair<std::string, RouteNextHopSet>> groups;
  groups.emplace_back("nhg1", makeResolvedNextHops({"1.1.1.10"}));
  addOrUpdateGroups(groups);

  // Add route with named NHG
  {
    UnicastRoute route;
    route.dest()->ip() =
        facebook::network::toBinaryAddress(folly::IPAddress("10.0.0.0"));
    route.dest()->prefixLength() = 24;
    NamedRouteDestination namedDest;
    namedDest.nextHopGroup() = "nhg1";
    route.namedRouteDestination() = namedDest;
    auto updater = sw_->getRouteUpdater();
    updater.addRoute(RouterID(0), ClientID::BGPD, route);
    updater.program();
  }

  // Verify the per-client entry has namedNextHopGroup set
  auto managerCopy = rib->getNextHopIDManagerCopy();
  EXPECT_TRUE(managerCopy->hasRoutesForNamedNhg("nhg1"));

  // Verify ClientAndNextHops via toThriftLegacy includes namedRouteDestination
  // by checking the route in the RIB
  bool foundNhg = false;
  auto ribRoutes = rib->getRouteTableDetails(RouterID(0));
  for (const auto& rd : ribRoutes) {
    auto prefix = folly::IPAddress::createNetwork(
        fmt::format(
            "{}/{}",
            facebook::network::toIPAddress(*rd.dest()->ip()).str(),
            *rd.dest()->prefixLength()));
    if (prefix == folly::IPAddress::createNetwork("10.0.0.0/24")) {
      // Check ClientAndNextHops.namedRouteDestination
      for (const auto& clientNhops : *rd.nextHopMulti()) {
        if (clientNhops.namedRouteDestination().has_value()) {
          EXPECT_EQ(
              *clientNhops.namedRouteDestination()->nextHopGroup(), "nhg1");
          foundNhg = true;
        }
      }
      // Check RouteDetails.namedRouteDestination
      ASSERT_TRUE(rd.namedRouteDestination().has_value());
      EXPECT_EQ(*rd.namedRouteDestination()->nextHopGroup(), "nhg1");
    }
  }
  EXPECT_TRUE(foundNhg);
}

} // namespace facebook::fboss
