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

  SwSwitch* sw_ = nullptr;
  std::unique_ptr<HwTestHandle> handle_;
};

TEST_F(NamedNextHopGroupRibTest, AddNamedNextHopGroups) {
  auto rib = sw_->getRib();
  ASSERT_NE(rib, nullptr);

  bool stateUpdateCalled = false;
  std::vector<std::pair<std::string, RouteNextHopSet>> groups;
  groups.emplace_back("group1", makeResolvedNextHops({"1.1.1.10", "2.2.2.10"}));
  groups.emplace_back("group2", makeResolvedNextHops({"1.1.1.20"}));

  rib->addOrUpdateNamedNextHopGroups(
      groups, [&](const NextHopIDManager* manager) {
        stateUpdateCalled = true;
        ASSERT_NE(manager, nullptr);
        // Both groups should be allocated
        EXPECT_TRUE(manager->hasNamedNextHopGroup("group1"));
        EXPECT_TRUE(manager->hasNamedNextHopGroup("group2"));
        auto setId1 = manager->getNextHopSetIDForName("group1");
        auto setId2 = manager->getNextHopSetIDForName("group2");
        EXPECT_TRUE(setId1.has_value());
        EXPECT_TRUE(setId2.has_value());
        // Different groups should have different set IDs
        EXPECT_NE(*setId1, *setId2);
      });

  EXPECT_TRUE(stateUpdateCalled);

  // Verify through the copy API as well
  auto managerCopy = rib->getNextHopIDManagerCopy();
  ASSERT_NE(managerCopy, nullptr);
  EXPECT_TRUE(managerCopy->hasNamedNextHopGroup("group1"));
  EXPECT_TRUE(managerCopy->hasNamedNextHopGroup("group2"));
}

TEST_F(NamedNextHopGroupRibTest, UpdateNamedNextHopGroup) {
  auto rib = sw_->getRib();
  ASSERT_NE(rib, nullptr);

  // First add a group
  std::vector<std::pair<std::string, RouteNextHopSet>> groups;
  groups.emplace_back("group1", makeResolvedNextHops({"1.1.1.10"}));
  rib->addOrUpdateNamedNextHopGroups(groups, [](const NextHopIDManager*) {});

  // Update with new nexthops (allocateNamedNextHopGroup handles both
  // add/update)
  std::vector<std::pair<std::string, RouteNextHopSet>> updatedGroups;
  updatedGroups.emplace_back(
      "group1", makeResolvedNextHops({"1.1.1.10", "2.2.2.10"}));

  bool updateCalled = false;
  rib->addOrUpdateNamedNextHopGroups(
      updatedGroups, [&](const NextHopIDManager* manager) {
        updateCalled = true;
        ASSERT_NE(manager, nullptr);
        EXPECT_TRUE(manager->hasNamedNextHopGroup("group1"));
        // The nexthops for group1 should be the updated set
        auto nhops = manager->getNextHopsForName("group1");
        ASSERT_TRUE(nhops.has_value());
        EXPECT_EQ(nhops->size(), 2);
      });

  EXPECT_TRUE(updateCalled);
}

TEST_F(NamedNextHopGroupRibTest, DeleteNamedNextHopGroups) {
  auto rib = sw_->getRib();
  ASSERT_NE(rib, nullptr);

  // Add groups first
  std::vector<std::pair<std::string, RouteNextHopSet>> groups;
  groups.emplace_back("group1", makeResolvedNextHops({"1.1.1.10"}));
  groups.emplace_back("group2", makeResolvedNextHops({"2.2.2.10"}));
  rib->addOrUpdateNamedNextHopGroups(groups, [](const NextHopIDManager*) {});

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
  rib->addOrUpdateNamedNextHopGroups(groups, [](const NextHopIDManager*) {});

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
  rib->addOrUpdateNamedNextHopGroups(seed, [](const NextHopIDManager*) {});

  // Batch: valid group first, then invalid group with empty nexthop set.
  // Without pre-validation, the valid group would be allocated before the
  // invalid group throws, leaving partial state.
  std::vector<std::pair<std::string, RouteNextHopSet>> batch;
  batch.emplace_back("validNew", makeResolvedNextHops({"2.2.2.10"}));
  batch.emplace_back("badGroup", RouteNextHopSet{}); // invalid: empty

  EXPECT_THROW(
      rib->addOrUpdateNamedNextHopGroups(batch, [](const NextHopIDManager*) {}),
      FbossError);

  // Only the seed entry should exist — the valid entry from the failed
  // batch must not have been inserted (no partial mutation)
  auto manager = rib->getNextHopIDManagerCopy();
  EXPECT_TRUE(manager->hasNamedNextHopGroup("existing"));
  EXPECT_FALSE(manager->hasNamedNextHopGroup("validNew"));
  EXPECT_FALSE(manager->hasNamedNextHopGroup("badGroup"));
}

TEST_F(NamedNextHopGroupRibTest, emptyNameInBatchDoesNotPartiallyMutate) {
  auto rib = sw_->getRib();
  ASSERT_NE(rib, nullptr);

  // Seed the table with a known-good entry
  std::vector<std::pair<std::string, RouteNextHopSet>> seed;
  seed.emplace_back("existing", makeResolvedNextHops({"1.1.1.10"}));
  rib->addOrUpdateNamedNextHopGroups(seed, [](const NextHopIDManager*) {});

  // Batch: valid group first, then invalid group with empty name
  std::vector<std::pair<std::string, RouteNextHopSet>> batch;
  batch.emplace_back("validNew", makeResolvedNextHops({"2.2.2.10"}));
  batch.emplace_back(
      "", makeResolvedNextHops({"1.1.1.20"})); // invalid: empty name

  EXPECT_THROW(
      rib->addOrUpdateNamedNextHopGroups(batch, [](const NextHopIDManager*) {}),
      FbossError);

  // Only the seed entry should exist — no partial mutation
  auto manager = rib->getNextHopIDManagerCopy();
  EXPECT_TRUE(manager->hasNamedNextHopGroup("existing"));
  EXPECT_FALSE(manager->hasNamedNextHopGroup("validNew"));
}

} // namespace facebook::fboss
