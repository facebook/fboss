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

#include <folly/json.h>
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
    return nextHopApi.create<SaiIpNextHopTraits>(
        {SAI_NEXT_HOP_TYPE_IP, 42, ip, std::nullopt}, 0);
  }

  NextHopSaiId createMplsNextHop(
      const folly::IPAddress& ip,
      std::vector<sai_uint32_t> labels) {
    auto& nextHopApi = saiApiTable->nextHopApi();
    return nextHopApi.create<SaiMplsNextHopTraits>(
        {SAI_NEXT_HOP_TYPE_MPLS, 42, ip, labels, std::nullopt}, 0);
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
  folly::IPAddress ip3{"10.10.10.3"};
  folly::IPAddress ip4{"10.10.10.4"};
  auto nextHopId1 = createNextHop(ip1);
  auto nextHopId2 = createNextHop(ip2);
  auto nextHopId3 = createMplsNextHop(ip3, {102, 103});
  auto nextHopId4 = createMplsNextHop(ip4, {201, 203});

  auto nextHopGroupMemberId1 =
      createNextHopGroupMember(nextHopGroupId, nextHopId1, std::nullopt);
  auto nextHopGroupMemberId2 =
      createNextHopGroupMember(nextHopGroupId, nextHopId2, std::nullopt);
  auto nextHopGroupMemberId3 =
      createNextHopGroupMember(nextHopGroupId, nextHopId3, std::nullopt);
  auto nextHopGroupMemberId4 =
      createNextHopGroupMember(nextHopGroupId, nextHopId4, std::nullopt);

  // perform a warm boot load
  SaiStore s(0);
  s.reload();
  auto& store = s.get<SaiNextHopGroupTraits>();

  SaiNextHopGroupTraits::AdapterHostKey k;
  k.insert(SaiIpNextHopTraits::AdapterHostKey{42, ip1});
  k.insert(SaiIpNextHopTraits::AdapterHostKey{42, ip2});
  k.insert(SaiMplsNextHopTraits::AdapterHostKey{
      42, ip3, std::vector<sai_uint32_t>{102, 103}});
  k.insert(SaiMplsNextHopTraits::AdapterHostKey{
      42, ip4, std::vector<sai_uint32_t>{201, 203}});
  auto got = store.get(k);
  EXPECT_TRUE(got);
  EXPECT_EQ(got->adapterKey(), nextHopGroupId);

  auto& memberStore = s.get<SaiNextHopGroupMemberTraits>();
  SaiNextHopGroupMemberTraits::AdapterHostKey k1{nextHopGroupId, nextHopId1};
  SaiNextHopGroupMemberTraits::AdapterHostKey k2{nextHopGroupId, nextHopId2};
  SaiNextHopGroupMemberTraits::AdapterHostKey k3{nextHopGroupId, nextHopId3};
  SaiNextHopGroupMemberTraits::AdapterHostKey k4{nextHopGroupId, nextHopId4};

  auto got1 = memberStore.get(k1);
  auto got2 = memberStore.get(k2);
  auto got3 = memberStore.get(k3);
  auto got4 = memberStore.get(k4);
  EXPECT_TRUE(got1);
  EXPECT_TRUE(got2);
  EXPECT_EQ(got1->adapterKey(), nextHopGroupMemberId1);
  EXPECT_EQ(got2->adapterKey(), nextHopGroupMemberId2);
  EXPECT_EQ(got3->adapterKey(), nextHopGroupMemberId3);
  EXPECT_EQ(got4->adapterKey(), nextHopGroupMemberId4);
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

TEST_F(NextHopGroupStoreTest, nhopGroupSerDeser) {
  auto nextHopGroupId = createNextHopGroup();
  verifyAdapterKeySerDeser<SaiNextHopGroupTraits>({nextHopGroupId});
}

TEST_F(NextHopGroupStoreTest, nhopGroupToStr) {
  std::ignore = createNextHopGroup();
  verifyToStr<SaiNextHopGroupTraits>();
}

TEST_F(NextHopGroupStoreTest, nhopGroupMemberSerDeser) {
  auto nextHopGroupId = createNextHopGroup();
  auto nextHopId = createNextHop(folly::IPAddress{"10.10.10.1"});
  auto nextHopGroupMemberId =
      createNextHopGroupMember(nextHopGroupId, nextHopId, std::nullopt);
  verifyAdapterKeySerDeser<SaiNextHopGroupMemberTraits>({nextHopGroupMemberId});
}

TEST_F(NextHopGroupStoreTest, nhopGroupMemberToStr) {
  auto nextHopGroupId = createNextHopGroup();
  auto nextHopId = createNextHop(folly::IPAddress{"10.10.10.1"});
  std::ignore =
      createNextHopGroupMember(nextHopGroupId, nextHopId, std::nullopt);
  verifyToStr<SaiNextHopGroupMemberTraits>();
}

TEST_F(NextHopGroupStoreTest, nextHopGroupJson) {
  // Create a next hop group
  auto nextHopGroupId = createNextHopGroup();

  // Create four next hops and four next hop group members
  folly::IPAddress ip1{"10.10.10.1"};
  folly::IPAddress ip2{"10.10.10.2"};
  folly::IPAddress ip3{"10.10.10.3"};
  folly::IPAddress ip4{"10.10.10.4"};
  auto nextHopId1 = createNextHop(ip1);
  auto nextHopId2 = createNextHop(ip2);
  auto nextHopId3 = createMplsNextHop(ip3, {102, 103});
  auto nextHopId4 = createMplsNextHop(ip4, {201, 203});

  createNextHopGroupMember(nextHopGroupId, nextHopId1, std::nullopt);
  createNextHopGroupMember(nextHopGroupId, nextHopId2, std::nullopt);
  createNextHopGroupMember(nextHopGroupId, nextHopId3, std::nullopt);
  createNextHopGroupMember(nextHopGroupId, nextHopId4, std::nullopt);

  // perform a warm boot load
  SaiStore s(0);
  s.reload();
  auto& store0 = s.get<SaiNextHopGroupTraits>();

  SaiNextHopGroupTraits::AdapterHostKey k;
  k.insert(SaiIpNextHopTraits::AdapterHostKey{42, ip1});
  k.insert(SaiIpNextHopTraits::AdapterHostKey{42, ip2});
  k.insert(SaiMplsNextHopTraits::AdapterHostKey{
      42, ip3, std::vector<sai_uint32_t>{102, 103}});
  k.insert(SaiMplsNextHopTraits::AdapterHostKey{
      42, ip4, std::vector<sai_uint32_t>{201, 203}});
  auto got = store0.get(k);
  EXPECT_TRUE(got);
  auto json = got->adapterHostKeyToFollyDynamic();
  auto k1 =
      SaiObject<SaiNextHopGroupTraits>::follyDynamicToAdapterHostKey(json);
  EXPECT_EQ(k1, k);

  auto ak2AhkJson = s.adapterKeys2AdapterHostKeysFollyDynamic();
  EXPECT_TRUE(!ak2AhkJson.empty());
  auto& nhgAk2AhkJson =
      ak2AhkJson[saiObjectTypeToString(SAI_OBJECT_TYPE_NEXT_HOP_GROUP)];
  EXPECT_TRUE(!nhgAk2AhkJson.empty());
  EXPECT_EQ(nhgAk2AhkJson.size(), 1);

  auto iter = nhgAk2AhkJson.find(folly::to<std::string>(got->adapterKey()));
  EXPECT_FALSE(nhgAk2AhkJson.items().end() == iter);
  EXPECT_EQ(iter->second, json);
}
