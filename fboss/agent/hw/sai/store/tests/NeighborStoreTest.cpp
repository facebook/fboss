/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/api/NeighborApi.h"
#include "fboss/agent/hw/sai/fake/FakeSai.h"
#include "fboss/agent/hw/sai/store/LoggingUtil.h"
#include "fboss/agent/hw/sai/store/SaiObject.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/store/tests/SaiStoreTest.h"

using namespace facebook::fboss;

TEST_F(SaiStoreTest, loadNeighbor) {
  auto& neighborApi = saiApiTable->neighborApi();
  folly::IPAddress ip4{"10.10.10.1"};
  SaiNeighborTraits::NeighborEntry n(0, 0, ip4);
  folly::MacAddress dstMac{"42:42:42:42:42:42"};
  SaiNeighborTraits::Attributes::DstMac daAttr{dstMac};
  neighborApi.create<SaiNeighborTraits>(n, {daAttr});

  SaiStore s(0);
  s.reload();
  auto& store = s.get<SaiNeighborTraits>();

  auto got = store.get(n);
  EXPECT_EQ(got->adapterKey(), n);
  EXPECT_EQ(
      std::get<SaiNeighborTraits::Attributes::DstMac>(got->attributes())
          .value(),
      dstMac);
}

TEST_F(SaiStoreTest, neighborLoadCtor) {
  auto& neighborApi = saiApiTable->neighborApi();
  folly::IPAddress ip4{"10.10.10.1"};
  SaiNeighborTraits::NeighborEntry n(0, 0, ip4);
  folly::MacAddress dstMac{"42:42:42:42:42:42"};
  SaiNeighborTraits::Attributes::DstMac daAttr{dstMac};
  neighborApi.create<SaiNeighborTraits>(n, {daAttr});

  SaiObject<SaiNeighborTraits> obj(n);
  EXPECT_EQ(obj.adapterKey(), n);
  EXPECT_EQ(GET_ATTR(Neighbor, DstMac, obj.attributes()), dstMac);
}

TEST_F(SaiStoreTest, neighborCreateCtor) {
  folly::IPAddress ip4{"10.10.10.1"};
  SaiNeighborTraits::NeighborEntry n(0, 0, ip4);
  folly::MacAddress dstMac{"42:42:42:42:42:42"};
  SaiObject<SaiNeighborTraits> obj(n, {dstMac}, 0);

  EXPECT_EQ(obj.adapterKey(), n);
  EXPECT_EQ(GET_ATTR(Neighbor, DstMac, obj.attributes()), dstMac);
}

TEST_F(SaiStoreTest, setDstMac) {
  folly::IPAddress ip4{"10.10.10.1"};
  SaiNeighborTraits::NeighborEntry n(0, 0, ip4);
  folly::MacAddress dstMac{"42:42:42:42:42:42"};
  SaiObject<SaiNeighborTraits> obj(n, {dstMac}, 0);

  EXPECT_EQ(obj.adapterKey(), n);
  EXPECT_EQ(GET_ATTR(Neighbor, DstMac, obj.attributes()), dstMac);
  folly::MacAddress dstMac2{"43:43:43:43:43:43"};
  obj.setAttributes({dstMac2});
  EXPECT_EQ(GET_ATTR(Neighbor, DstMac, obj.attributes()), dstMac2);
  // Check that dst mac really changed according to SAI
  auto& neighborApi = saiApiTable->neighborApi();
  auto apiDstMac = neighborApi.getAttribute(
      obj.adapterKey(), SaiNeighborTraits::Attributes::DstMac{});
  EXPECT_EQ(apiDstMac, dstMac2);
}

TEST_F(SaiStoreTest, formatTest) {
  folly::IPAddress ip4{"10.10.10.1"};
  SaiNeighborTraits::NeighborEntry n(0, 0, ip4);
  folly::MacAddress dstMac{"42:42:42:42:42:42"};
  SaiObject<SaiNeighborTraits> obj(n, {dstMac}, 0);

  auto expected =
      "NeighborEntry(switch:0, rif: 0, ip: 10.10.10.1): "
      "(DstMac: 42:42:42:42:42:42)";
  EXPECT_EQ(expected, fmt::format("{}", obj));
}
