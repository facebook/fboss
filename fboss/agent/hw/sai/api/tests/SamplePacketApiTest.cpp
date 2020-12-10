/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/api/SamplePacketApi.h"
#include "fboss/agent/hw/sai/api/SaiObjectApi.h"
#include "fboss/agent/hw/sai/fake/FakeSai.h"

#include <folly/logging/xlog.h>

#include <gtest/gtest.h>

using namespace facebook::fboss;

class SamplePacketApiTest : public ::testing::Test {
 public:
  void SetUp() override {
    fs = FakeSai::getInstance();
    sai_api_initialize(0, nullptr);
    samplePacketApi = std::make_unique<SamplePacketApi>();
  }

  SamplePacketSaiId createSamplePacket(
      const uint16_t sampleRate,
      sai_samplepacket_type_t type,
      sai_samplepacket_mode_t mode) const {
    return samplePacketApi->create<SaiSamplePacketTraits>(
        {sampleRate, type, mode}, 0);
  }

  void checkSamplePacket(SamplePacketSaiId samplePacketSaiId) const {
    auto fsp = fs->samplePacketManager.get(samplePacketSaiId);
    EXPECT_EQ(fsp.id, samplePacketSaiId);
    SaiSamplePacketTraits::Attributes::SampleRate sampleRate;
    auto gotSampleRate =
        samplePacketApi->getAttribute(samplePacketSaiId, sampleRate);
    EXPECT_EQ(
        fs->samplePacketManager.get(samplePacketSaiId).sampleRate,
        gotSampleRate);
    auto gotType = samplePacketApi->getAttribute(
        samplePacketSaiId, SaiSamplePacketTraits::Attributes::Type{});
    EXPECT_EQ(fs->samplePacketManager.get(samplePacketSaiId).type, gotType);
    auto gotMode = samplePacketApi->getAttribute(
        samplePacketSaiId, SaiSamplePacketTraits::Attributes::Mode{});
    EXPECT_EQ(fs->samplePacketManager.get(samplePacketSaiId).mode, gotMode);
  }

  std::shared_ptr<FakeSai> fs;
  std::unique_ptr<SamplePacketApi> samplePacketApi;
};

TEST_F(SamplePacketApiTest, createSamplePacket) {
  auto samplePacketSaiId = createSamplePacket(
      1000, SAI_SAMPLEPACKET_TYPE_SLOW_PATH, SAI_SAMPLEPACKET_MODE_EXCLUSIVE);
  checkSamplePacket(samplePacketSaiId);
}

TEST_F(SamplePacketApiTest, removeSamplePacket) {
  auto samplePacketSaiId = createSamplePacket(
      1000, SAI_SAMPLEPACKET_TYPE_SLOW_PATH, SAI_SAMPLEPACKET_MODE_EXCLUSIVE);
  EXPECT_EQ(fs->samplePacketManager.map().size(), 1);
  samplePacketApi->remove(samplePacketSaiId);
  EXPECT_EQ(fs->samplePacketManager.map().size(), 0);
}

TEST_F(SamplePacketApiTest, setSamplePacketAttributes) {
  auto samplePacketSaiId = createSamplePacket(
      1000, SAI_SAMPLEPACKET_TYPE_SLOW_PATH, SAI_SAMPLEPACKET_MODE_SHARED);
  checkSamplePacket(samplePacketSaiId);
  SaiSamplePacketTraits::Attributes::SampleRate sampleRate{2000};
  samplePacketApi->setAttribute(samplePacketSaiId, sampleRate);
  EXPECT_EQ(samplePacketApi->getAttribute(samplePacketSaiId, sampleRate), 2000);
}
