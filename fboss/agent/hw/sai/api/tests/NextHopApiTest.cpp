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
    NextHopApiParameters::Attributes::Type typeAttribute(SAI_NEXT_HOP_TYPE_IP);
    NextHopApiParameters::Attributes::RouterInterfaceId
        routerInterfaceIdAttribute(0);
    NextHopApiParameters::Attributes::Ip ipAttribute(ip4);
    auto nextHopId = nextHopApi->create2<SaiNextHopTraits>(
        {typeAttribute, routerInterfaceIdAttribute, ipAttribute}, 0);
    auto fnh = fs->nhm.get(nextHopId);
    EXPECT_EQ(SAI_NEXT_HOP_TYPE_IP, fnh.type);
    EXPECT_EQ(ip, fnh.ip);
    EXPECT_EQ(0, fnh.routerInterfaceId);
    return nextHopId;
  }
  std::shared_ptr<FakeSai> fs;
  std::unique_ptr<NextHopApi> nextHopApi;
  folly::IPAddress ip4{str4};
};

TEST_F(NextHopApiTest, createNextHop) {
  createNextHop(ip4);
}

TEST_F(NextHopApiTest, removeNextHop) {
  auto nextHopId = createNextHop(ip4);
  EXPECT_EQ(fs->nhm.map().size(), 1);
  nextHopApi->remove2(nextHopId);
  EXPECT_EQ(fs->nhm.map().size(), 0);
}

TEST_F(NextHopApiTest, getIp) {
  auto nextHopId = createNextHop(ip4);
  NextHopApiParameters::Attributes::Ip ipAttribute;
  EXPECT_EQ(ip4, nextHopApi->getAttribute2(nextHopId, ipAttribute));
}

// IP is create only, so if we try to set it, we expect to fail
TEST_F(NextHopApiTest, setIp) {
  auto nextHopId = createNextHop(ip4);
  NextHopApiParameters::Attributes::Ip ipAttribute(ip4);
  EXPECT_EQ(
      nextHopApi->setAttribute2(nextHopId, ipAttribute),
      SAI_STATUS_INVALID_PARAMETER);
}
