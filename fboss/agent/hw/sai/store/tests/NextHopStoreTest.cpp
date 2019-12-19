/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/api/NextHopApi.h"
#include "fboss/agent/hw/sai/fake/FakeSai.h"
#include "fboss/agent/hw/sai/store/SaiObject.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/store/tests/SaiStoreTest.h"

using namespace facebook::fboss;

class NextHopStoreTest : public SaiStoreTest {
 public:
  NextHopSaiId createNextHop(const folly::IPAddress& ip) {
    return saiApiTable->nextHopApi().create<SaiNextHopTraits>(
        {SAI_NEXT_HOP_TYPE_IP, 42, ip}, 0);
  }
};

TEST_F(NextHopStoreTest, loadNextHops) {
  folly::IPAddress ip1{"4200::41"};
  folly::IPAddress ip2{"4200::42"};
  auto nextHopSaiId1 = createNextHop(ip1);
  auto nextHopSaiId2 = createNextHop(ip2);

  SaiStore s(0);
  s.reload();
  auto& store = s.get<SaiNextHopTraits>();

  SaiNextHopTraits::AdapterHostKey k1{42, ip1};
  SaiNextHopTraits::AdapterHostKey k2{42, ip2};
  auto got = store.get(k1);
  EXPECT_EQ(got->adapterKey(), nextHopSaiId1);
  got = store.get(k2);
  EXPECT_EQ(got->adapterKey(), nextHopSaiId2);
}

TEST_F(NextHopStoreTest, nextHopLoadCtor) {
  auto ip = folly::IPAddress("::");
  auto nextHopSaiId = createNextHop(ip);
  SaiObject<SaiNextHopTraits> obj(nextHopSaiId);
  EXPECT_EQ(obj.adapterKey(), nextHopSaiId);
  EXPECT_EQ(GET_ATTR(NextHop, Ip, obj.attributes()), ip);
}

TEST_F(NextHopStoreTest, nextHopCreateCtor) {
  auto ip = folly::IPAddress("::");
  SaiNextHopTraits::CreateAttributes c{SAI_NEXT_HOP_TYPE_IP, 42, ip};
  SaiNextHopTraits::AdapterHostKey k{42, ip};
  SaiObject<SaiNextHopTraits> obj(k, c, 0);
  EXPECT_EQ(GET_ATTR(NextHop, Ip, obj.attributes()), ip);
}
