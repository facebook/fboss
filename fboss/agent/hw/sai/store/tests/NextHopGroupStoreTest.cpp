/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/api/NextHopGroupApi.h"
#include "fboss/agent/hw/sai/fake/FakeSai.h"
#include "fboss/agent/hw/sai/store/SaiObject.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"

#include "fboss/agent/hw/sai/store/tests/SaiStoreTest.h"

using namespace facebook::fboss;

class NextHopGroupStoreTest : public SaiStoreTest {
 public:
  NextHopGroupSaiId createNextHopGroup() {
    auto& nextHopGroupApi = saiApiTable->nextHopGroupApi();
    return nextHopGroupApi.create<SaiNextHopGroupTraits>(
        {SAI_NEXT_HOP_GROUP_TYPE_ECMP}, 0);
  }

  NextHopGroupMemberSaiId createNextHopGroupMember(
      sai_object_id_t groupId,
      sai_object_id_t nextHopId,
      std::optional<sai_uint32_t> weight) {
    auto& nextHopGroupApi = saiApiTable->nextHopGroupApi();
    return nextHopGroupApi.create<SaiNextHopGroupMemberTraits>(
        {groupId, nextHopId, weight}, 0);
  }

  NextHopSaiId createNextHop(const folly::IPAddress& ip) {
    auto& nextHopApi = saiApiTable->nextHopApi();
    return nextHopApi.create<SaiNextHopTraits>(
        {SAI_NEXT_HOP_TYPE_IP, 42, ip, std::nullopt}, 0);
  }
};

TEST_F(NextHopGroupStoreTest, loadEmptyNextHopGroup) {
  auto nextHopGroupId = createNextHopGroup();

  SaiStore s(0);
  s.reload();
  auto& store = s.get<SaiNextHopGroupTraits>();

  SaiNextHopGroupTraits::AdapterHostKey k;
  auto got = store.get(k);
  EXPECT_EQ(got->adapterKey(), nextHopGroupId);
}

TEST_F(NextHopGroupStoreTest, loadNextHopGroup) {
  // Create a next hop group
  auto nextHopGroupId = createNextHopGroup();

  // Create two next hops and two next hop group members
  folly::IPAddress ip1{"10.10.10.1"};
  folly::IPAddress ip2{"10.10.10.2"};
  auto nextHopId1 = createNextHop(ip1);
  auto nextHopId2 = createNextHop(ip2);
  auto nextHopGroupMemberId1 =
      createNextHopGroupMember(nextHopGroupId, nextHopId1, std::nullopt);
  auto nextHopGroupMemberId2 =
      createNextHopGroupMember(nextHopGroupId, nextHopId2, std::nullopt);

  // perform a warm boot load
  SaiStore s(0);
  s.reload();
  auto& store = s.get<SaiNextHopGroupTraits>();

  SaiNextHopGroupTraits::AdapterHostKey k;
  k.insert({42, ip1, std::nullopt});
  k.insert({42, ip2, std::nullopt});
  auto got = store.get(k);
  EXPECT_TRUE(got);
  EXPECT_EQ(got->adapterKey(), nextHopGroupId);

  auto& memberStore = s.get<SaiNextHopGroupMemberTraits>();
  SaiNextHopGroupMemberTraits::AdapterHostKey k1{nextHopGroupId, nextHopId1};
  SaiNextHopGroupMemberTraits::AdapterHostKey k2{nextHopGroupId, nextHopId2};
  auto got1 = memberStore.get(k1);
  auto got2 = memberStore.get(k2);
  EXPECT_TRUE(got1);
  EXPECT_TRUE(got2);
  EXPECT_EQ(got1->adapterKey(), nextHopGroupMemberId1);
  EXPECT_EQ(got2->adapterKey(), nextHopGroupMemberId2);
}

TEST_F(NextHopGroupStoreTest, nextHopGroupLoadCtor) {
  auto id = createNextHopGroup();
  SaiObject<SaiNextHopGroupTraits> obj(id);
  EXPECT_EQ(obj.adapterKey(), id);
}

TEST_F(NextHopGroupStoreTest, nextHopGroupMemberLoadCtor) {
  auto nhgId = createNextHopGroup();
  auto id = createNextHopGroupMember(nhgId, NextHopSaiId{10}, std::nullopt);
  SaiObject<SaiNextHopGroupMemberTraits> obj(id);
  EXPECT_EQ(obj.adapterKey(), id);
}

TEST_F(NextHopGroupStoreTest, nextHopGroupCreateCtor) {
  SaiNextHopGroupTraits::AdapterHostKey k{{}};
  SaiNextHopGroupTraits::CreateAttributes c{SAI_NEXT_HOP_GROUP_TYPE_ECMP};
  SaiObject<SaiNextHopGroupTraits> obj(k, c, 0);
}

TEST_F(NextHopGroupStoreTest, nextHopGroupMemberCreateCtor) {
  auto nhgId = createNextHopGroup();
  SaiNextHopGroupMemberTraits::AdapterHostKey k{nhgId, 42};
  SaiNextHopGroupMemberTraits::CreateAttributes c{nhgId, 42, 2};
  SaiObject<SaiNextHopGroupMemberTraits> obj(k, c, 0);
  EXPECT_EQ(GET_ATTR(NextHopGroupMember, NextHopId, obj.attributes()), 42);
  EXPECT_EQ(GET_OPT_ATTR(NextHopGroupMember, Weight, obj.attributes()), 2);
}
