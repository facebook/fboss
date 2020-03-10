/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/sai/api/MplsApi.h"
#include "fboss/agent/hw/sai/fake/FakeSai.h"

#include <folly/logging/xlog.h>

#include <gtest/gtest.h>

#include <memory>

using namespace facebook::fboss;

class MplsApiTest : public ::testing::Test {
 public:
  void SetUp() override {
    fs = FakeSai::getInstance();
    sai_api_initialize(0, nullptr);
    mplsApi = std::make_unique<MplsApi>();
  }
  std::shared_ptr<FakeSai> fs;
  std::unique_ptr<MplsApi> mplsApi;
};

TEST_F(MplsApiTest, createInSegEntry) {
  SaiInSegTraits::InSegEntry is{0, 42};
  SaiInSegTraits::CreateAttributes attrs{SAI_PACKET_ACTION_FORWARD, 1, 10};
  mplsApi->create<SaiInSegTraits>(is, attrs);
  EXPECT_EQ(
      mplsApi->getAttribute(is, SaiInSegTraits::Attributes::NumOfPop{}), 1);
}

TEST_F(MplsApiTest, removeInSegEntry) {
  SaiInSegTraits::InSegEntry is{0, 42};
  SaiInSegTraits::CreateAttributes attrs{SAI_PACKET_ACTION_FORWARD, 1, 10};
  mplsApi->create<SaiInSegTraits>(is, attrs);
  mplsApi->remove(is);
}

TEST_F(MplsApiTest, setAttributes) {
  SaiInSegTraits::InSegEntry is{0, 42};
  SaiInSegTraits::CreateAttributes attrs{SAI_PACKET_ACTION_FORWARD, 1, 10};
  mplsApi->create<SaiInSegTraits>(is, attrs);
  EXPECT_EQ(
      mplsApi->getAttribute(is, SaiInSegTraits::Attributes::NumOfPop{}), 1);
  mplsApi->setAttribute(is, SaiInSegTraits::Attributes::NumOfPop{2});
  EXPECT_EQ(
      mplsApi->getAttribute(is, SaiInSegTraits::Attributes::NumOfPop{}), 2);
}

TEST_F(MplsApiTest, inSegEntrySerDeser) {
  SaiInSegTraits::InSegEntry is{0, 42};
  EXPECT_EQ(
      is, SaiInSegTraits::InSegEntry::fromFollyDynamic(is.toFollyDynamic()));
}

TEST_F(MplsApiTest, formatInSegAttributes) {
  SaiInSegTraits::Attributes::NextHopId nhid{42};
  EXPECT_EQ("NextHopId: 42", fmt::format("{}", nhid));
  SaiInSegTraits::Attributes::PacketAction pa{SAI_PACKET_ACTION_FORWARD};
  EXPECT_EQ("PacketAction: 1", fmt::format("{}", pa));
  SaiInSegTraits::Attributes::NumOfPop nop{4};
  EXPECT_EQ("NumOfPop: 4", fmt::format("{}", nop));
}
