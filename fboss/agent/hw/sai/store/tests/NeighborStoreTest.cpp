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
#include "fboss/agent/hw/sai/store/SaiObject.h"

#include <folly/logging/xlog.h>

#include <gtest/gtest.h>

using namespace facebook::fboss;

class NeighborStoreTest : public ::testing::Test {
 public:
  void SetUp() override {
    fs = FakeSai::getInstance();
    sai_api_initialize(0, nullptr);
    saiApiTable = SaiApiTable::getInstance();
    saiApiTable->queryApis();
  }
  std::shared_ptr<FakeSai> fs;
  std::shared_ptr<SaiApiTable> saiApiTable;
};

TEST_F(NeighborStoreTest, neighborLoadCtor) {
  auto& neighborApi = saiApiTable->neighborApi();
  folly::IPAddress ip4{"10.10.10.1"};
  SaiNeighborTraits::NeighborEntry n(0, 0, ip4);
  folly::MacAddress dstMac{"42:42:42:42:42:42"};
  SaiNeighborTraits::Attributes::DstMac daAttr{dstMac};
  neighborApi.create2<SaiNeighborTraits>(n, {daAttr});

  SaiObject<SaiNeighborTraits> obj(n);
  EXPECT_EQ(obj.adapterKey(), n);
  EXPECT_EQ(GET_ATTR(Neighbor, DstMac, obj.attributes()), dstMac);
}

TEST_F(NeighborStoreTest, neighborCreateCtor) {
  folly::IPAddress ip4{"10.10.10.1"};
  SaiNeighborTraits::NeighborEntry n(0, 0, ip4);
  folly::MacAddress dstMac{"42:42:42:42:42:42"};
  SaiObject<SaiNeighborTraits> obj(n, {dstMac}, 0);

  EXPECT_EQ(obj.adapterKey(), n);
  EXPECT_EQ(GET_ATTR(Neighbor, DstMac, obj.attributes()), dstMac);
}

TEST_F(NeighborStoreTest, setDstMac) {
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
  auto apiDstMac = neighborApi.getAttribute2(
      obj.adapterKey(), SaiNeighborTraits::Attributes::DstMac{});
  EXPECT_EQ(apiDstMac, dstMac2);
}
