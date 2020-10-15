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
#include "fboss/agent/hw/sai/store/Traits.h"
#include "fboss/agent/hw/sai/store/tests/SaiStoreTest.h"

using namespace facebook::fboss;

SaiNeighborTraits::CreateAttributes createAttrs(
    folly::MacAddress dstMac,
    std::optional<sai_uint32_t> metadata = std::nullopt) {
  return {dstMac, metadata};
}

TEST_F(SaiStoreTest, loadNeighbor) {
  auto& neighborApi = saiApiTable->neighborApi();
  folly::IPAddress ip4{"10.10.10.1"};
  SaiNeighborTraits::NeighborEntry n(0, 0, ip4);
  folly::MacAddress dstMac{"42:42:42:42:42:42"};
  neighborApi.create<SaiNeighborTraits>(n, createAttrs(dstMac, 42));

  SaiStore s(0);
  s.reload();
  auto& store = s.get<SaiNeighborTraits>();

  auto got = store.get(n);
  EXPECT_EQ(got->adapterKey(), n);
  EXPECT_EQ(
      std::get<SaiNeighborTraits::Attributes::DstMac>(got->attributes())
          .value(),
      dstMac);
  EXPECT_EQ(
      std::get<std::optional<SaiNeighborTraits::Attributes::Metadata>>(
          got->attributes())
          .value(),
      42);
}

TEST_F(SaiStoreTest, neighborLoadCtor) {
  auto& neighborApi = saiApiTable->neighborApi();
  folly::IPAddress ip4{"10.10.10.1"};
  SaiNeighborTraits::NeighborEntry n(0, 0, ip4);
  folly::MacAddress dstMac{"42:42:42:42:42:42"};
  neighborApi.create<SaiNeighborTraits>(n, createAttrs(dstMac, 42));

  SaiObject<SaiNeighborTraits> obj(n);
  EXPECT_EQ(obj.adapterKey(), n);
  EXPECT_EQ(GET_ATTR(Neighbor, DstMac, obj.attributes()), dstMac);
  EXPECT_EQ(GET_OPT_ATTR(Neighbor, Metadata, obj.attributes()), 42);
}

TEST_F(SaiStoreTest, neighborCreateCtor) {
  folly::IPAddress ip4{"10.10.10.1"};
  SaiNeighborTraits::NeighborEntry n(0, 0, ip4);
  folly::MacAddress dstMac{"42:42:42:42:42:42"};
  SaiObject<SaiNeighborTraits> obj(n, createAttrs(dstMac, 42), 0);

  EXPECT_EQ(obj.adapterKey(), n);
  EXPECT_EQ(GET_ATTR(Neighbor, DstMac, obj.attributes()), dstMac);
  EXPECT_EQ(GET_OPT_ATTR(Neighbor, Metadata, obj.attributes()), 42);
}

TEST_F(SaiStoreTest, setDstMac) {
  folly::IPAddress ip4{"10.10.10.1"};
  SaiNeighborTraits::NeighborEntry n(0, 0, ip4);
  folly::MacAddress dstMac{"42:42:42:42:42:42"};
  SaiObject<SaiNeighborTraits> obj(n, createAttrs(dstMac), 0);

  EXPECT_EQ(obj.adapterKey(), n);
  EXPECT_EQ(GET_ATTR(Neighbor, DstMac, obj.attributes()), dstMac);
  folly::MacAddress dstMac2{"43:43:43:43:43:43"};
  obj.setAttributes({dstMac2, std::nullopt});
  EXPECT_EQ(GET_ATTR(Neighbor, DstMac, obj.attributes()), dstMac2);
  // Check that dst mac really changed according to SAI
  auto& neighborApi = saiApiTable->neighborApi();
  auto apiDstMac = neighborApi.getAttribute(
      obj.adapterKey(), SaiNeighborTraits::Attributes::DstMac{});
  EXPECT_EQ(apiDstMac, dstMac2);
}

TEST_F(SaiStoreTest, setMetadata) {
  folly::IPAddress ip4{"10.10.10.1"};
  SaiNeighborTraits::NeighborEntry n(0, 0, ip4);
  folly::MacAddress dstMac{"42:42:42:42:42:42"};
  SaiObject<SaiNeighborTraits> obj(n, createAttrs(dstMac), 0);

  EXPECT_EQ(obj.adapterKey(), n);
  EXPECT_EQ(GET_ATTR(Neighbor, DstMac, obj.attributes()), dstMac);
  folly::MacAddress dstMac2{"43:43:43:43:43:43"};
  obj.setAttributes({dstMac, 42});
  EXPECT_EQ(GET_ATTR(Neighbor, DstMac, obj.attributes()), dstMac);
  EXPECT_EQ(GET_OPT_ATTR(Neighbor, Metadata, obj.attributes()), 42);
  // Check that dst mac really changed according to SAI
  auto& neighborApi = saiApiTable->neighborApi();
  EXPECT_EQ(
      neighborApi.getAttribute(
          obj.adapterKey(), SaiNeighborTraits::Attributes::Metadata{}),
      42);
}

TEST_F(SaiStoreTest, neighborFormatTest) {
  folly::IPAddress ip4{"10.10.10.1"};
  SaiNeighborTraits::NeighborEntry n(0, 0, ip4);
  folly::MacAddress dstMac{"42:42:42:42:42:42"};
  SaiObject<SaiNeighborTraits> obj(n, createAttrs(dstMac, 42), 0);

  auto expected =
      "NeighborEntry(switch:0, rif: 0, ip: 10.10.10.1): "
      "(DstMac: 42:42:42:42:42:42, Metadata: 42)";
  EXPECT_EQ(expected, fmt::format("{}", obj));
}

TEST_F(SaiStoreTest, neighbor4SerDeser) {
  auto& neighborApi = saiApiTable->neighborApi();
  folly::IPAddress ip{"10.10.10.1"};
  SaiNeighborTraits::NeighborEntry n(0, 0, ip);
  folly::MacAddress dstMac{"42:42:42:42:42:42"};
  SaiNeighborTraits::Attributes::DstMac daAttrcreateAttrs(dstMac);
  neighborApi.create<SaiNeighborTraits>(n, createAttrs(dstMac));
  verifyAdapterKeySerDeser<SaiNeighborTraits>({n});
}

TEST_F(SaiStoreTest, neighbor4ToStr) {
  auto& neighborApi = saiApiTable->neighborApi();
  folly::IPAddress ip{"10.10.10.1"};
  SaiNeighborTraits::NeighborEntry n(0, 0, ip);
  folly::MacAddress dstMac{"42:42:42:42:42:42"};
  SaiNeighborTraits::Attributes::DstMac daAttrcreateAttrs(dstMac);
  neighborApi.create<SaiNeighborTraits>(n, createAttrs(dstMac));
  verifyToStr<SaiNeighborTraits>();
}

TEST_F(SaiStoreTest, neighbor6SerDeser) {
  auto& neighborApi = saiApiTable->neighborApi();
  folly::IPAddress ip{"42::1"};
  SaiNeighborTraits::NeighborEntry n(0, 0, ip);
  folly::MacAddress dstMac{"42:42:42:42:42:42"};
  SaiNeighborTraits::Attributes::DstMac daAttrcreateAttrs(dstMac);
  neighborApi.create<SaiNeighborTraits>(n, createAttrs(dstMac));
  verifyAdapterKeySerDeser<SaiNeighborTraits>({n});
}

TEST_F(SaiStoreTest, neighbor6ToStr) {
  auto& neighborApi = saiApiTable->neighborApi();
  folly::IPAddress ip{"42::1"};
  SaiNeighborTraits::NeighborEntry n(0, 0, ip);
  folly::MacAddress dstMac{"42:42:42:42:42:42"};
  SaiNeighborTraits::Attributes::DstMac daAttrcreateAttrs(dstMac);
  neighborApi.create<SaiNeighborTraits>(n, createAttrs(dstMac));
  verifyToStr<SaiNeighborTraits>();
}

TEST_F(SaiStoreTest, neighborPublisher) {
  EXPECT_TRUE(IsObjectPublisher<SaiNeighborTraits>::value);
  EXPECT_FALSE(IsObjectPublisher<SaiInSegTraits>::value);
}
