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

#include <folly/logging/xlog.h>

#include <gtest/gtest.h>

#include <memory>

using namespace facebook::fboss;

static constexpr folly::StringPiece str4 = "42.42.12.34";

class NextHopApiTest : public ::testing::Test {
 public:
  void SetUp() override {
    fs = FakeSai::getInstance();
    sai_api_initialize(0, nullptr);
    nextHopApi = std::make_unique<NextHopApi>();
  }
  NextHopSaiId createNextHop(folly::IPAddress ip) {
    SaiIpNextHopTraits::Attributes::Type typeAttribute(SAI_NEXT_HOP_TYPE_IP);
    SaiIpNextHopTraits::Attributes::RouterInterfaceId
        routerInterfaceIdAttribute(0);
    SaiIpNextHopTraits::Attributes::Ip ipAttribute(ip4);
    auto nextHopId = nextHopApi->create<SaiIpNextHopTraits>(
        {typeAttribute, routerInterfaceIdAttribute, ipAttribute, std::nullopt},
        0);
    auto fnh = fs->nextHopManager.get(nextHopId);
    EXPECT_EQ(SAI_NEXT_HOP_TYPE_IP, fnh.type);
    EXPECT_EQ(ip, fnh.ip);
    EXPECT_EQ(0, fnh.routerInterfaceId);
    return nextHopId;
  }

  NextHopSaiId createMplsNextHop(
      folly::IPAddress ip,
      std::vector<sai_uint32_t> stack) {
    SaiMplsNextHopTraits::Attributes::Type typeAttribute(
        SAI_NEXT_HOP_TYPE_MPLS);
    SaiMplsNextHopTraits::Attributes::RouterInterfaceId
        routerInterfaceIdAttribute(0);
    SaiMplsNextHopTraits::Attributes::Ip ipAttribute(ip4);
    SaiMplsNextHopTraits::Attributes::LabelStack labelStack{stack};
    auto nextHopId = nextHopApi->create<SaiMplsNextHopTraits>(
        {typeAttribute,
         routerInterfaceIdAttribute,
         ipAttribute,
         labelStack,
         std::nullopt},
        0);
    auto fnh = fs->nextHopManager.get(nextHopId);
    EXPECT_EQ(SAI_NEXT_HOP_TYPE_MPLS, fnh.type);
    EXPECT_EQ(ip, fnh.ip);
    EXPECT_EQ(0, fnh.routerInterfaceId);
    EXPECT_EQ(stack, fnh.labelStack);

    return nextHopId;
  }
  std::shared_ptr<FakeSai> fs;
  std::unique_ptr<NextHopApi> nextHopApi;
  folly::IPAddress ip4{str4};
};

TEST_F(NextHopApiTest, createNextHop) {
  createNextHop(ip4);
}

TEST_F(NextHopApiTest, createMplsNextHop) {
  createMplsNextHop(ip4, {1001, 2001, 3001});
}

TEST_F(NextHopApiTest, removeNextHop) {
  auto nextHopId = createNextHop(ip4);
  EXPECT_EQ(fs->nextHopManager.map().size(), 1);
  nextHopApi->remove(nextHopId);
  EXPECT_EQ(fs->nextHopManager.map().size(), 0);
}

TEST_F(NextHopApiTest, getIpTypeAttribute) {
  auto nextHopId = createNextHop(ip4);

  auto nextHopIpGot =
      nextHopApi->getAttribute(nextHopId, SaiIpNextHopTraits::Attributes::Ip());
  auto nextHopRouterInterfaceIdGot = nextHopApi->getAttribute(
      nextHopId, SaiIpNextHopTraits::Attributes::RouterInterfaceId());
  auto nextHopTypeGot = nextHopApi->getAttribute(
      nextHopId, SaiIpNextHopTraits::Attributes::Type());

  EXPECT_EQ(nextHopIpGot, ip4);
  EXPECT_EQ(nextHopRouterInterfaceIdGot, 0);
  EXPECT_EQ(nextHopTypeGot, SAI_NEXT_HOP_TYPE_IP);

  // IP does not support get LabelStack for Mpls
  EXPECT_THROW(
      nextHopApi->getAttribute(
          nextHopId, SaiMplsNextHopTraits::Attributes::LabelStack()),
      SaiApiError);
}

TEST_F(NextHopApiTest, getMplsTypeAttribute) {
  std::vector<sai_uint32_t> stack{1001, 2001, 3001};
  auto nextHopId = createMplsNextHop(ip4, stack);

  auto nextHopIpGot =
      nextHopApi->getAttribute(nextHopId, SaiIpNextHopTraits::Attributes::Ip());
  auto nextHopRouterInterfaceIdGot = nextHopApi->getAttribute(
      nextHopId, SaiIpNextHopTraits::Attributes::RouterInterfaceId());
  auto nextHopLabelStackGot = nextHopApi->getAttribute(
      nextHopId, SaiMplsNextHopTraits::Attributes::LabelStack());
  auto nextHopTypeGot = nextHopApi->getAttribute(
      nextHopId, SaiIpNextHopTraits::Attributes::Type());

  EXPECT_EQ(nextHopIpGot, ip4);
  EXPECT_EQ(nextHopRouterInterfaceIdGot, 0);
  EXPECT_EQ(nextHopLabelStackGot, stack);
  EXPECT_EQ(nextHopTypeGot, SAI_NEXT_HOP_TYPE_MPLS);
}

// IP is create only, so if we try to set it, we expect to fail
TEST_F(NextHopApiTest, setIpTypeAttribute) {
  auto nextHopId = createNextHop(ip4);

  SaiIpNextHopTraits::Attributes::Ip ipAttribute(ip4);
  SaiIpNextHopTraits::Attributes::RouterInterfaceId routerInterfaceIdAttribute{
      1};
  SaiIpNextHopTraits::Attributes::Type typeAttribute{SAI_NEXT_HOP_TYPE_IP};

  EXPECT_THROW(nextHopApi->setAttribute(nextHopId, ipAttribute);, SaiApiError);
  EXPECT_THROW(
      nextHopApi->setAttribute(nextHopId, routerInterfaceIdAttribute),
      SaiApiError);
  EXPECT_THROW(nextHopApi->setAttribute(nextHopId, typeAttribute), SaiApiError);
}

// MPLS is create only as well, so if we try to set it, we expect to fail
TEST_F(NextHopApiTest, setMplsTypeAttribute) {
  std::vector<sai_uint32_t> stack{1001, 2001, 3001};
  auto nextHopId = createMplsNextHop(ip4, stack);

  SaiIpNextHopTraits::Attributes::Ip ipAttribute(ip4);
  SaiIpNextHopTraits::Attributes::RouterInterfaceId routerInterfaceIdAttribute{
      1};
  SaiMplsNextHopTraits::Attributes::LabelStack labelStackAttribute{
      std::vector<sai_uint32_t>{2001, 3001, 4001}};
  SaiIpNextHopTraits::Attributes::Type typeAttribute{SAI_NEXT_HOP_TYPE_IP};

  EXPECT_THROW(nextHopApi->setAttribute(nextHopId, ipAttribute);, SaiApiError);
  EXPECT_THROW(
      nextHopApi->setAttribute(nextHopId, routerInterfaceIdAttribute),
      SaiApiError);
  EXPECT_THROW(
      nextHopApi->setAttribute(nextHopId, labelStackAttribute), SaiApiError);
  EXPECT_THROW(nextHopApi->setAttribute(nextHopId, typeAttribute), SaiApiError);
}

TEST_F(NextHopApiTest, formatNextHopAttributes) {
  SaiIpNextHopTraits::Attributes::Type t{SAI_NEXT_HOP_TYPE_IP};
  EXPECT_EQ("Type: 0", fmt::format("{}", t));
  SaiIpNextHopTraits::Attributes::RouterInterfaceId rifid{42};
  EXPECT_EQ("RouterInterfaceId: 42", fmt::format("{}", rifid));
  SaiIpNextHopTraits::Attributes::Ip ip{ip4};
  EXPECT_EQ(fmt::format("Ip: {}", str4), fmt::format("{}", ip));
  SaiMplsNextHopTraits::Attributes::LabelStack ls{{42, 100}};
  EXPECT_EQ("LabelStack: {42, 100}", fmt::format("{}", ls));
}
