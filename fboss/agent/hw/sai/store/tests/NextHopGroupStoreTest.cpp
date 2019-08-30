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

#include <folly/logging/xlog.h>

#include <gtest/gtest.h>

using namespace facebook::fboss;

class NextHopGroupStoreTest : public ::testing::Test {
 public:
  void SetUp() override {
    fs = FakeSai::getInstance();
    sai_api_initialize(0, nullptr);
    saiApiTable = SaiApiTable::getInstance();
    saiApiTable->queryApis();
  }
  std::shared_ptr<FakeSai> fs;
  std::shared_ptr<SaiApiTable> saiApiTable;

  NextHopGroupSaiId createNextHopGroup() {
    auto& nextHopGroupApi = saiApiTable->nextHopGroupApi();
    return nextHopGroupApi.create2<SaiNextHopGroupTraits>(
        {SAI_NEXT_HOP_GROUP_TYPE_ECMP}, 0);
  }

  NextHopGroupMemberSaiId createNextHopGroupMember(
      sai_object_id_t groupId,
      sai_object_id_t nextHopId) {
    auto& nextHopGroupApi = saiApiTable->nextHopGroupApi();
    return nextHopGroupApi.create2<SaiNextHopGroupMemberTraits>(
        {groupId, nextHopId}, 0);
  }

  NextHopSaiId createNextHop(const folly::IPAddress& ip) {
    auto& nextHopApi = saiApiTable->nextHopApi();
    return nextHopApi.create2<SaiNextHopTraits>(
        {SAI_NEXT_HOP_TYPE_IP, 42, ip}, 0);
  }
};

TEST_F(NextHopGroupStoreTest, nextHopGroupLoadCtor) {
  auto id = createNextHopGroup();
  SaiObject<SaiNextHopGroupTraits> obj(id);
  EXPECT_EQ(obj.adapterKey(), id);
}

TEST_F(NextHopGroupStoreTest, nextHopGroupMemberLoadCtor) {
  auto nhgId = createNextHopGroup();
  auto id = createNextHopGroupMember(nhgId, NextHopSaiId{10});
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
  SaiNextHopGroupMemberTraits::CreateAttributes c{nhgId, 42};
  SaiObject<SaiNextHopGroupMemberTraits> obj(k, c, 0);
  EXPECT_EQ(GET_ATTR(NextHopGroupMember, NextHopId, obj.attributes()), 42);
}
