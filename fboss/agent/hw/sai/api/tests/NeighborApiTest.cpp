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

#include <folly/IPAddress.h>
#include <folly/logging/xlog.h>

#include <gtest/gtest.h>

using namespace facebook::fboss;

static constexpr folly::StringPiece str4 = "42.42.12.34";
static constexpr folly::StringPiece str6 =
    "4242:4242:4242:4242:1234:1234:1234:1234";
static constexpr folly::StringPiece strMac = "42:42:42:12:34:56";

class NeighborApiTest : public ::testing::Test {
 public:
  void SetUp() override {
    fs = FakeSai::getInstance();
    sai_api_initialize(0, nullptr);
    neighborApi = std::make_unique<NeighborApi>();
  }
  SaiNeighborTraits::CreateAttributes createAttrs(
      std::optional<sai_uint32_t> metadata = std::nullopt,
      std::optional<sai_uint32_t> encapIndex = std::nullopt,
      bool noHostRoute = false) const {
    bool isLocal = encapIndex == std::nullopt;
    return {dstMac, metadata, encapIndex, isLocal, noHostRoute};
  }
  std::shared_ptr<FakeSai> fs;
  std::unique_ptr<NeighborApi> neighborApi;
  folly::IPAddress ip4{str4};
  folly::IPAddress ip6{str6};
  folly::MacAddress dstMac{strMac};
};

TEST_F(NeighborApiTest, createV4Neighbor) {
  SaiNeighborTraits::NeighborEntry n(0, 0, ip4);
  FakeNeighborEntry fn = std::make_tuple(0, 0, ip4);
  neighborApi->create<SaiNeighborTraits>(n, createAttrs());
  EXPECT_EQ(fs->neighborManager.get(fn).dstMac, dstMac);
  EXPECT_TRUE(fs->neighborManager.get(fn).isLocal);
}

TEST_F(NeighborApiTest, createV4NeighborRemote) {
  SaiNeighborTraits::NeighborEntry n(0, 0, ip4);
  FakeNeighborEntry fn = std::make_tuple(0, 0, ip4);
  neighborApi->create<SaiNeighborTraits>(n, createAttrs(std::nullopt, 42));
  EXPECT_EQ(fs->neighborManager.get(fn).dstMac, dstMac);
  EXPECT_FALSE(fs->neighborManager.get(fn).isLocal);
  EXPECT_EQ(fs->neighborManager.get(fn).encapIndex, 42);
  EXPECT_EQ(
      neighborApi->getAttribute(n, SaiNeighborTraits::Attributes::EncapIndex()),
      42);
  EXPECT_FALSE(
      neighborApi->getAttribute(n, SaiNeighborTraits::Attributes::IsLocal()));
}

TEST_F(NeighborApiTest, createV4NeighborWithMetadata) {
  SaiNeighborTraits::NeighborEntry n(0, 0, ip4);
  FakeNeighborEntry fn = std::make_tuple(0, 0, ip4);
  neighborApi->create<SaiNeighborTraits>(n, createAttrs(42));
  EXPECT_EQ(fs->neighborManager.get(fn).dstMac, dstMac);
  EXPECT_EQ(fs->neighborManager.get(fn).metadata, 42);
  EXPECT_TRUE(fs->neighborManager.get(fn).isLocal);
}

TEST_F(NeighborApiTest, createV4NeighborWithMetadataRemote) {
  SaiNeighborTraits::NeighborEntry n(0, 0, ip4);
  FakeNeighborEntry fn = std::make_tuple(0, 0, ip4);
  neighborApi->create<SaiNeighborTraits>(n, createAttrs(42, 24));
  EXPECT_EQ(fs->neighborManager.get(fn).dstMac, dstMac);
  EXPECT_EQ(fs->neighborManager.get(fn).metadata, 42);
  EXPECT_FALSE(fs->neighborManager.get(fn).isLocal);
  EXPECT_EQ(fs->neighborManager.get(fn).encapIndex, 24);
  EXPECT_EQ(
      neighborApi->getAttribute(n, SaiNeighborTraits::Attributes::EncapIndex()),
      24);
  EXPECT_FALSE(
      neighborApi->getAttribute(n, SaiNeighborTraits::Attributes::IsLocal()));
}

TEST_F(NeighborApiTest, createV6Neighbor) {
  SaiNeighborTraits::NeighborEntry n(0, 0, ip6);
  FakeNeighborEntry fn = std::make_tuple(0, 0, ip6);
  neighborApi->create<SaiNeighborTraits>(n, createAttrs());
  EXPECT_EQ(fs->neighborManager.get(fn).dstMac, dstMac);
  EXPECT_TRUE(fs->neighborManager.get(fn).isLocal);
}

TEST_F(NeighborApiTest, createV6NeighborRemote) {
  SaiNeighborTraits::NeighborEntry n(0, 0, ip6);
  FakeNeighborEntry fn = std::make_tuple(0, 0, ip6);
  neighborApi->create<SaiNeighborTraits>(n, createAttrs(std::nullopt, 42));
  EXPECT_EQ(fs->neighborManager.get(fn).dstMac, dstMac);
  EXPECT_FALSE(fs->neighborManager.get(fn).isLocal);
  EXPECT_EQ(fs->neighborManager.get(fn).encapIndex, 42);
  EXPECT_EQ(
      neighborApi->getAttribute(n, SaiNeighborTraits::Attributes::EncapIndex()),
      42);
  EXPECT_FALSE(
      neighborApi->getAttribute(n, SaiNeighborTraits::Attributes::IsLocal()));
}

TEST_F(NeighborApiTest, createV6NeighborWithMetdata) {
  SaiNeighborTraits::Attributes::DstMac dstMacAttribute(dstMac);
  SaiNeighborTraits::NeighborEntry n(0, 0, ip6);
  FakeNeighborEntry fn = std::make_tuple(0, 0, ip6);
  neighborApi->create<SaiNeighborTraits>(n, createAttrs(42));
  EXPECT_EQ(fs->neighborManager.get(fn).dstMac, dstMac);
  EXPECT_EQ(fs->neighborManager.get(fn).metadata, 42);
  EXPECT_TRUE(fs->neighborManager.get(fn).isLocal);
}

TEST_F(NeighborApiTest, createV6NeighborWithMetdataRemote) {
  SaiNeighborTraits::Attributes::DstMac dstMacAttribute(dstMac);
  SaiNeighborTraits::NeighborEntry n(0, 0, ip6);
  FakeNeighborEntry fn = std::make_tuple(0, 0, ip6);
  neighborApi->create<SaiNeighborTraits>(n, createAttrs(42, 24));
  EXPECT_EQ(fs->neighborManager.get(fn).dstMac, dstMac);
  EXPECT_EQ(fs->neighborManager.get(fn).metadata, 42);
  EXPECT_FALSE(fs->neighborManager.get(fn).isLocal);
  EXPECT_EQ(fs->neighborManager.get(fn).encapIndex, 24);
  EXPECT_EQ(
      neighborApi->getAttribute(n, SaiNeighborTraits::Attributes::EncapIndex()),
      24);
  EXPECT_FALSE(
      neighborApi->getAttribute(n, SaiNeighborTraits::Attributes::IsLocal()));
}

TEST_F(NeighborApiTest, createNeighborNoHostRoute) {
  SaiNeighborTraits::Attributes::DstMac dstMacAttribute(dstMac);
  SaiNeighborTraits::NeighborEntry n(0, 0, ip6);
  FakeNeighborEntry fn = std::make_tuple(0, 0, ip6);
  neighborApi->create<SaiNeighborTraits>(
      n, createAttrs(std::nullopt, std::nullopt, true));
  EXPECT_EQ(fs->neighborManager.get(fn).dstMac, dstMac);
  EXPECT_TRUE(fs->neighborManager.get(fn).noHostRoute);
  EXPECT_TRUE(fs->neighborManager.get(fn).isLocal);
}

TEST_F(NeighborApiTest, removeV4Neighbor) {
  SaiNeighborTraits::NeighborEntry n(0, 0, ip4);
  neighborApi->create<SaiNeighborTraits>(n, createAttrs());
  EXPECT_EQ(fs->neighborManager.map().size(), 1);
  neighborApi->remove(n);
  EXPECT_EQ(fs->neighborManager.map().size(), 0);
}

TEST_F(NeighborApiTest, removeV6Neighbor) {
  SaiNeighborTraits::NeighborEntry n(0, 0, ip6);
  neighborApi->create<SaiNeighborTraits>(n, createAttrs());
  EXPECT_EQ(fs->neighborManager.map().size(), 1);
  neighborApi->remove(n);
  EXPECT_EQ(fs->neighborManager.map().size(), 0);
}

TEST_F(NeighborApiTest, getV4DstMac) {
  SaiNeighborTraits::NeighborEntry n(0, 0, ip4);
  neighborApi->create<SaiNeighborTraits>(n, createAttrs());
  auto gotMac =
      neighborApi->getAttribute(n, SaiNeighborTraits::Attributes::DstMac());
  EXPECT_EQ(gotMac, dstMac);
}

TEST_F(NeighborApiTest, setV4DstMac) {
  SaiNeighborTraits::NeighborEntry n(0, 0, ip4);
  neighborApi->create<SaiNeighborTraits>(n, createAttrs());
  folly::MacAddress dstMac2("22:22:22:22:22:22");
  SaiNeighborTraits::Attributes::DstMac dstMacAttribute2(dstMac2);
  neighborApi->setAttribute(n, dstMacAttribute2);
  auto gotMac =
      neighborApi->getAttribute(n, SaiNeighborTraits::Attributes::DstMac());
  EXPECT_EQ(gotMac, dstMac2);
}

TEST_F(NeighborApiTest, setV4Metadata) {
  SaiNeighborTraits::NeighborEntry n(0, 0, ip4);
  neighborApi->create<SaiNeighborTraits>(n, createAttrs());
  EXPECT_EQ(
      neighborApi->getAttribute(n, SaiNeighborTraits::Attributes::Metadata()),
      0);
  SaiNeighborTraits::Attributes::Metadata metadata(42);
  neighborApi->setAttribute(n, metadata);
  EXPECT_EQ(
      neighborApi->getAttribute(n, SaiNeighborTraits::Attributes::Metadata()),
      42);
}

TEST_F(NeighborApiTest, setV4EncapIndex) {
  SaiNeighborTraits::NeighborEntry n(0, 0, ip4);
  neighborApi->create<SaiNeighborTraits>(n, createAttrs());
  EXPECT_EQ(
      neighborApi->getAttribute(n, SaiNeighborTraits::Attributes::EncapIndex()),
      0);
  SaiNeighborTraits::Attributes::EncapIndex encap(42);
  neighborApi->setAttribute(n, encap);
  EXPECT_EQ(
      neighborApi->getAttribute(n, SaiNeighborTraits::Attributes::EncapIndex()),
      42);
}

TEST_F(NeighborApiTest, setV4IsLocal) {
  SaiNeighborTraits::NeighborEntry n(0, 0, ip4);
  neighborApi->create<SaiNeighborTraits>(n, createAttrs());
  EXPECT_TRUE(
      neighborApi->getAttribute(n, SaiNeighborTraits::Attributes::IsLocal()));
  SaiNeighborTraits::Attributes::IsLocal local(false);
  neighborApi->setAttribute(n, local);
  EXPECT_FALSE(
      neighborApi->getAttribute(n, SaiNeighborTraits::Attributes::IsLocal()));
}

TEST_F(NeighborApiTest, setV6DstMac) {
  SaiNeighborTraits::NeighborEntry n(0, 0, ip6);
  neighborApi->create<SaiNeighborTraits>(n, createAttrs());
  folly::MacAddress dstMac2("22:22:22:22:22:22");
  SaiNeighborTraits::Attributes::DstMac dstMacAttribute2(dstMac2);
  neighborApi->setAttribute(n, dstMacAttribute2);
  auto gotMac =
      neighborApi->getAttribute(n, SaiNeighborTraits::Attributes::DstMac());
  EXPECT_EQ(gotMac, dstMac2);
}

TEST_F(NeighborApiTest, setV6DstMetadata) {
  SaiNeighborTraits::NeighborEntry n(0, 0, ip6);
  neighborApi->create<SaiNeighborTraits>(n, createAttrs());
  EXPECT_EQ(
      neighborApi->getAttribute(n, SaiNeighborTraits::Attributes::Metadata()),
      0);
  SaiNeighborTraits::Attributes::Metadata metadata(42);
  neighborApi->setAttribute(n, metadata);
  EXPECT_EQ(
      neighborApi->getAttribute(n, SaiNeighborTraits::Attributes::Metadata()),
      42);
}
TEST_F(NeighborApiTest, setV6EncapIndex) {
  SaiNeighborTraits::NeighborEntry n(0, 0, ip6);
  neighborApi->create<SaiNeighborTraits>(n, createAttrs());
  EXPECT_EQ(
      neighborApi->getAttribute(n, SaiNeighborTraits::Attributes::EncapIndex()),
      0);
  SaiNeighborTraits::Attributes::EncapIndex encap(62);
  neighborApi->setAttribute(n, encap);
  EXPECT_EQ(
      neighborApi->getAttribute(n, SaiNeighborTraits::Attributes::EncapIndex()),
      62);
}

TEST_F(NeighborApiTest, setV6IsLocal) {
  SaiNeighborTraits::NeighborEntry n(0, 0, ip6);
  neighborApi->create<SaiNeighborTraits>(n, createAttrs());
  EXPECT_TRUE(
      neighborApi->getAttribute(n, SaiNeighborTraits::Attributes::IsLocal()));
  SaiNeighborTraits::Attributes::IsLocal local(false);
  neighborApi->setAttribute(n, local);
  EXPECT_FALSE(
      neighborApi->getAttribute(n, SaiNeighborTraits::Attributes::IsLocal()));
}

TEST_F(NeighborApiTest, v4NeighborSerDeser) {
  SaiNeighborTraits::NeighborEntry n(0, 0, ip4);
  EXPECT_EQ(
      n,
      SaiNeighborTraits::NeighborEntry::fromFollyDynamic(n.toFollyDynamic()));
}

TEST_F(NeighborApiTest, v6NeighborSerDeser) {
  SaiNeighborTraits::NeighborEntry n(0, 0, ip6);
  EXPECT_EQ(
      n,
      SaiNeighborTraits::NeighborEntry::fromFollyDynamic(n.toFollyDynamic()));
}

TEST_F(NeighborApiTest, formatNeighborAttributes) {
  SaiNeighborTraits::Attributes::DstMac dm(dstMac);
  EXPECT_EQ(fmt::format("DstMac: {}", strMac), fmt::format("{}", dm));
  SaiNeighborTraits::Attributes::Metadata m(42);
  EXPECT_EQ(fmt::format("Metadata: 42"), fmt::format("{}", m));
}
