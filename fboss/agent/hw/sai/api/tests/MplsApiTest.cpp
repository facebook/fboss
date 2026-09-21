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

  SaiInSegTraits::InSegEntry createInSegEntry(
      const sai_packet_action_t packetAction,
      const sai_uint8_t numOfPop,
      const sai_object_id_t nextHopId) const {
    sai_object_id_t switchId = 0;
    uint32_t label = 42;
    SaiInSegTraits::InSegEntry inSegEntry{switchId, label};

    SaiInSegTraits::CreateAttributes attrs{packetAction, numOfPop, nextHopId};
    mplsApi->create<SaiInSegTraits>(inSegEntry, attrs);

    return inSegEntry;
  }

  std::shared_ptr<FakeSai> fs;
  std::unique_ptr<MplsApi> mplsApi;
};

TEST_F(MplsApiTest, createInSegEntry) {
  SaiInSegTraits::InSegEntry inSegEntry =
      createInSegEntry(SAI_PACKET_ACTION_FORWARD, 1, 10);
  EXPECT_EQ(
      mplsApi->getAttribute(inSegEntry, SaiInSegTraits::Attributes::NumOfPop{}),
      1);
}

TEST_F(MplsApiTest, removeInSegEntry) {
  SaiInSegTraits::InSegEntry inSegEntry =
      createInSegEntry(SAI_PACKET_ACTION_FORWARD, 1, 10);
  mplsApi->remove(inSegEntry);
}

TEST_F(MplsApiTest, getAttributes) {
  SaiInSegTraits::InSegEntry inSegEntry =
      createInSegEntry(SAI_PACKET_ACTION_FORWARD, 1, 10);

  SaiInSegTraits::Attributes::PacketAction packetActionGot =
      mplsApi->getAttribute(
          inSegEntry, SaiInSegTraits::Attributes::PacketAction{});
  SaiInSegTraits::Attributes::NumOfPop numOfPopGot =
      mplsApi->getAttribute(inSegEntry, SaiInSegTraits::Attributes::NumOfPop{});
  SaiInSegTraits::Attributes::NextHopId nextHopIdGot = mplsApi->getAttribute(
      inSegEntry, SaiInSegTraits::Attributes::NextHopId{});

  EXPECT_EQ(packetActionGot, SAI_PACKET_ACTION_FORWARD);
  EXPECT_EQ(numOfPopGot, 1);
  EXPECT_EQ(nextHopIdGot, 10);
}

TEST_F(MplsApiTest, setAttributes) {
  SaiInSegTraits::InSegEntry inSegEntry =
      createInSegEntry(SAI_PACKET_ACTION_FORWARD, 1, 10);

  SaiInSegTraits::Attributes::PacketAction packetAction{SAI_PACKET_ACTION_COPY};
  SaiInSegTraits::Attributes::NumOfPop numOfPop{2};
  SaiInSegTraits::Attributes::NextHopId nextHopId{11};

  mplsApi->setAttribute(inSegEntry, packetAction);
  mplsApi->setAttribute(inSegEntry, numOfPop);
  mplsApi->setAttribute(inSegEntry, nextHopId);

  SaiInSegTraits::Attributes::PacketAction packetActionGot =
      mplsApi->getAttribute(
          inSegEntry, SaiInSegTraits::Attributes::PacketAction{});
  SaiInSegTraits::Attributes::NumOfPop numOfPopGot =
      mplsApi->getAttribute(inSegEntry, SaiInSegTraits::Attributes::NumOfPop{});
  SaiInSegTraits::Attributes::NextHopId nextHopIdGot = mplsApi->getAttribute(
      inSegEntry, SaiInSegTraits::Attributes::NextHopId{});

  EXPECT_EQ(packetActionGot, packetAction);
  EXPECT_EQ(numOfPopGot, numOfPop);
  EXPECT_EQ(nextHopIdGot, nextHopId);
}

TEST_F(MplsApiTest, inSegEntrySerDeser) {
  SaiInSegTraits::InSegEntry inSegEntry{0, 42};
  EXPECT_EQ(
      inSegEntry,
      SaiInSegTraits::InSegEntry::fromFollyDynamic(
          inSegEntry.toFollyDynamic()));
}

TEST_F(MplsApiTest, formatInSegAttributes) {
  SaiInSegTraits::Attributes::NextHopId nhid{42};
  EXPECT_EQ("NextHopId: 42", fmt::format("{}", nhid));
  SaiInSegTraits::Attributes::PacketAction pa{SAI_PACKET_ACTION_FORWARD};
  EXPECT_EQ("PacketAction: 1", fmt::format("{}", pa));
  SaiInSegTraits::Attributes::NumOfPop nop{4};
  EXPECT_EQ("NumOfPop: 4", fmt::format("{}", nop));
}
