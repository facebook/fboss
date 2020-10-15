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
    return saiApiTable->nextHopApi().create<SaiIpNextHopTraits>(
        {SAI_NEXT_HOP_TYPE_IP, 42, ip, std::nullopt}, 0);
  }
  NextHopSaiId createMplsNextHop(
      const folly::IPAddress& ip,
      std::vector<sai_uint32_t> stack) {
    return saiApiTable->nextHopApi().create<SaiMplsNextHopTraits>(
        {SAI_NEXT_HOP_TYPE_MPLS, 42, ip, std::move(stack), std::nullopt}, 0);
  }
};

TEST_F(NextHopStoreTest, loadNextHops) {
  folly::IPAddress ip1{"4200::41"};
  folly::IPAddress ip2{"4200::42"};
  auto nextHopSaiId1 = createNextHop(ip1);
  auto nextHopSaiId2 = createNextHop(ip2);
  auto nextHopSaiId3 =
      createMplsNextHop(ip1, std::vector<sai_uint32_t>{1001, 1002});
  auto nextHopSaiId4 =
      createMplsNextHop(ip2, std::vector<sai_uint32_t>{2001, 2002});

  SaiStore s(0);
  s.reload();
  auto& store = s.get<SaiIpNextHopTraits>();

  SaiIpNextHopTraits::AdapterHostKey k1{42, ip1};
  SaiIpNextHopTraits::AdapterHostKey k2{42, ip2};
  auto got = store.get(k1);
  EXPECT_EQ(got->adapterKey(), nextHopSaiId1);
  got = store.get(k2);
  EXPECT_EQ(got->adapterKey(), nextHopSaiId2);

  auto& mplsNextHopStore = s.get<SaiMplsNextHopTraits>();
  SaiMplsNextHopTraits::AdapterHostKey k3{
      42, ip1, std::vector<sai_uint32_t>{1001, 1002}};
  SaiMplsNextHopTraits::AdapterHostKey k4{
      42, ip2, std::vector<sai_uint32_t>{2001, 2002}};
  auto mplsNhop = mplsNextHopStore.get(k3);
  ASSERT_NE(mplsNhop, nullptr);
  EXPECT_EQ(mplsNhop->adapterKey(), nextHopSaiId3);
  mplsNhop = mplsNextHopStore.get(k4);
  ASSERT_NE(mplsNhop, nullptr);
  EXPECT_EQ(mplsNhop->adapterKey(), nextHopSaiId4);
}

TEST_F(NextHopStoreTest, nextHopLoadCtor) {
  auto ip = folly::IPAddress("::");
  auto nextHopSaiId = createNextHop(ip);
  SaiObject<SaiIpNextHopTraits> obj(nextHopSaiId);
  EXPECT_EQ(obj.adapterKey(), nextHopSaiId);
  EXPECT_EQ(GET_ATTR(IpNextHop, Ip, obj.attributes()), ip);
}

TEST_F(NextHopStoreTest, nextHopCreateCtor) {
  auto ip = folly::IPAddress("::");
  SaiIpNextHopTraits::CreateAttributes c{
      SAI_NEXT_HOP_TYPE_IP, 42, ip, std::nullopt};
  SaiIpNextHopTraits::AdapterHostKey k{42, ip};
  SaiObject<SaiIpNextHopTraits> obj(k, c, 0);
  EXPECT_EQ(GET_ATTR(IpNextHop, Ip, obj.attributes()), ip);
}

TEST_F(NextHopStoreTest, ipNextHopSerDeser) {
  auto ip = folly::IPAddress("::");
  auto nextHopSaiId = createNextHop(ip);
  verifyAdapterKeySerDeser<SaiIpNextHopTraits>({nextHopSaiId});
}

TEST_F(NextHopStoreTest, mplsNextHopSerDeser) {
  auto nextHopSaiId = createMplsNextHop(
      folly::IPAddress{"4200::41"}, std::vector<sai_uint32_t>{1001, 1002});
  verifyAdapterKeySerDeser<SaiMplsNextHopTraits>({nextHopSaiId});
}

TEST_F(NextHopStoreTest, ipNextHopToStr) {
  auto ip = folly::IPAddress("::");
  std::ignore = createNextHop(ip);
  verifyToStr<SaiIpNextHopTraits>();
}

TEST_F(NextHopStoreTest, mplsNextHopToStr) {
  std::ignore = createMplsNextHop(
      folly::IPAddress{"4200::41"}, std::vector<sai_uint32_t>{1001, 1002});
  verifyToStr<SaiMplsNextHopTraits>();
}
