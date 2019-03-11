/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/sai/api/SaiApiTable.h"
#include "fboss/agent/hw/sai/fake/FakeSai.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiNextHopGroupManager.h"
#include "fboss/agent/hw/sai/switch/tests/ManagerTestBase.h"
#include "fboss/agent/state/RouteNextHopEntry.h"
#include "fboss/agent/types.h"

using namespace facebook::fboss;

/*
 * In these tests, we will assume 4 ports with one lane each, with IDs
 */

class NextHopGroupManagerTest : public ManagerTestBase {
 public:
  void SetUp() override {
    ManagerTestBase::SetUp();
    setupPorts();
    setupVlans();
    setupInterfaces();
  }
  void setupPorts() {
    addPort(1, true);
    addPort(2, true);
  }
  void setupVlans() {
    addVlan(1, {});
    addVlan(2, {});
  }
  void setupInterfaces() {
    addInterface(1, rmac1);
    addInterface(2, rmac2);
  }
  void setupNeighbors() {
    addArpEntry(1, dip1.asV4(), dmac1);
    addArpEntry(2, dip2.asV4(), dmac2);
  }

  void checkNextHopGroup(
      sai_object_id_t nextHopGroupId,
      const std::unordered_set<folly::IPAddress>& expectedNextHopIps) {
    auto& nextHopGroupApi = saiApiTable->nextHopGroupApi();
    auto& nextHopApi = saiApiTable->nextHopApi();
    auto size = expectedNextHopIps.size();
    std::vector<sai_object_id_t> m;
    m.resize(expectedNextHopIps.size());
    NextHopGroupApiParameters::Attributes::NextHopMemberList memberList{m};
    auto members = nextHopGroupApi.getAttribute(memberList, nextHopGroupId);
    EXPECT_EQ(members.size(), size);

    std::unordered_set<folly::IPAddress> gotNextHopIps;
    for (const auto& member : members) {
      sai_object_id_t nextHopId = nextHopGroupApi.getMemberAttribute(
          NextHopGroupApiParameters::MemberAttributes::NextHopId{}, member);
      folly::IPAddress ip = nextHopApi.getAttribute(
          NextHopApiParameters::Attributes::Ip{}, nextHopId);
      EXPECT_TRUE(gotNextHopIps.insert(ip).second);
    }
    EXPECT_EQ(gotNextHopIps, expectedNextHopIps);
  }

  folly::IPAddress dip1{"1.1.1.1"};
  folly::IPAddress dip2{"2.2.2.2"};
  folly::MacAddress rmac1{"41:41:41:41:41:41"};
  folly::MacAddress rmac2{"42:42:42:42:42:42"};
  folly::MacAddress dmac1{"11:11:11:11:11:11"};
  folly::MacAddress dmac2{"22:22:22:22:22:22"};
};

TEST_F(NextHopGroupManagerTest, addNextHopGroup) {
  ResolvedNextHop nh1{dip1, InterfaceID(1), ECMP_WEIGHT};
  ResolvedNextHop nh2{dip2, InterfaceID(2), ECMP_WEIGHT};
  RouteNextHopEntry::NextHopSet swNextHops{nh1, nh2};
  auto saiNextHopGroup =
      saiManagerTable->nextHopGroupManager().incRefOrAddNextHopGroup(
          swNextHops);
  EXPECT_EQ(saiNextHopGroup.use_count(), 1);
  checkNextHopGroup(saiNextHopGroup->id(), {});
}

TEST_F(NextHopGroupManagerTest, refNextHopGroup) {
  ResolvedNextHop nh1{dip1, InterfaceID(1), ECMP_WEIGHT};
  ResolvedNextHop nh2{dip2, InterfaceID(2), ECMP_WEIGHT};
  RouteNextHopEntry::NextHopSet swNextHops{nh1, nh2};
  auto saiNextHopGroup =
      saiManagerTable->nextHopGroupManager().incRefOrAddNextHopGroup(
          swNextHops);
  EXPECT_EQ(saiNextHopGroup.use_count(), 1);

  RouteNextHopEntry::NextHopSet swNextHops2{nh1, nh2};
  auto saiNextHopGroup2 =
      saiManagerTable->nextHopGroupManager().incRefOrAddNextHopGroup(
          swNextHops2);
  EXPECT_EQ(saiNextHopGroup.use_count(), 2);

  EXPECT_EQ(saiNextHopGroup, saiNextHopGroup2);
  checkNextHopGroup(saiNextHopGroup->id(), {});
}

TEST_F(NextHopGroupManagerTest, derefNextHopGroup) {
  ResolvedNextHop nh1{dip1, InterfaceID(1), ECMP_WEIGHT};
  ResolvedNextHop nh2{dip2, InterfaceID(2), ECMP_WEIGHT};
  RouteNextHopEntry::NextHopSet swNextHops{nh1, nh2};
  auto saiNextHopGroup =
      saiManagerTable->nextHopGroupManager().incRefOrAddNextHopGroup(
          swNextHops);
  EXPECT_EQ(saiNextHopGroup.use_count(), 1);
  {
    RouteNextHopEntry::NextHopSet swNextHops2{nh1, nh2};
    auto saiNextHopGroup2 =
        saiManagerTable->nextHopGroupManager().incRefOrAddNextHopGroup(
            swNextHops2);
    EXPECT_EQ(saiNextHopGroup.use_count(), 2);
  }
  EXPECT_EQ(saiNextHopGroup.use_count(), 1);
  checkNextHopGroup(saiNextHopGroup->id(), {});
}

TEST_F(NextHopGroupManagerTest, deleteNextHopGroup) {
  std::weak_ptr<SaiNextHopGroup> counter;
  {
    ResolvedNextHop nh1{dip1, InterfaceID(1), ECMP_WEIGHT};
    ResolvedNextHop nh2{dip2, InterfaceID(2), ECMP_WEIGHT};
    RouteNextHopEntry::NextHopSet swNextHops{nh1, nh2};
    auto saiNextHopGroup =
        saiManagerTable->nextHopGroupManager().incRefOrAddNextHopGroup(
            swNextHops);
    counter = saiNextHopGroup;
    EXPECT_EQ(counter.use_count(), 1);
  }
  EXPECT_EQ(counter.use_count(), 0);
  EXPECT_TRUE(counter.expired());
}

TEST_F(NextHopGroupManagerTest, resolveNeighborBefore) {
  setupNeighbors();
  ResolvedNextHop nh1{dip1, InterfaceID(1), ECMP_WEIGHT};
  ResolvedNextHop nh2{dip2, InterfaceID(2), ECMP_WEIGHT};
  RouteNextHopEntry::NextHopSet swNextHops{nh1, nh2};
  auto saiNextHopGroup =
      saiManagerTable->nextHopGroupManager().incRefOrAddNextHopGroup(
          swNextHops);
  checkNextHopGroup(saiNextHopGroup->id(), {dip1, dip2});
}

TEST_F(NextHopGroupManagerTest, resolveNeighborAfter) {
  ResolvedNextHop nh1{dip1, InterfaceID(1), ECMP_WEIGHT};
  ResolvedNextHop nh2{dip2, InterfaceID(2), ECMP_WEIGHT};
  RouteNextHopEntry::NextHopSet swNextHops{nh1, nh2};
  auto saiNextHopGroup =
      saiManagerTable->nextHopGroupManager().incRefOrAddNextHopGroup(
          swNextHops);
  setupNeighbors();
  /*
   * will pass in D13604057
  checkNextHopGroup(saiNextHopGroup->id(), {dip1, dip2});
  */
}

TEST_F(NextHopGroupManagerTest, unresolveNeighbor) {
}

TEST_F(NextHopGroupManagerTest, derefThenResolve) {
}

TEST_F(NextHopGroupManagerTest, deleteThenResolve) {
}

TEST_F(NextHopGroupManagerTest, l3UnresolvedNextHop) {
}
