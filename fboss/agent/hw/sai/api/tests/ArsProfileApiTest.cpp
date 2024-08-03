/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/api/ArsProfileApi.h"
#include "fboss/agent/hw/sai/fake/FakeSai.h"

#include <folly/logging/xlog.h>

#include <gtest/gtest.h>

using namespace facebook::fboss;

class ArsProfileApiTest : public ::testing::Test {
 public:
  void SetUp() override {
    fs = FakeSai::getInstance();
    sai_api_initialize(0, nullptr);
    arsProfileApi = std::make_unique<ArsProfileApi>();
  }

  ArsProfileSaiId createArsProfile() const {
    SaiArsProfileTraits::Attributes::Algo arsProfileAlgoAttribute{
        SAI_ARS_PROFILE_ALGO_EWMA};
    SaiArsProfileTraits::Attributes::SamplingInterval
        arsProfileSamplingIntervalAttribute{kSamplingInterval()};
    SaiArsProfileTraits::Attributes::RandomSeed arsProfileRandomSeedAttribute{
        kRandomSeed()};
    SaiArsProfileTraits::Attributes::EnableIPv4 arsProfileEnableIPv4Attribute{
        false};
    SaiArsProfileTraits::Attributes::EnableIPv6 arsProfileEnableIPv6Attribute{
        false};
    SaiArsProfileTraits::Attributes::PortLoadPast
        arsProfilePortLoadPastAttribute{true};
    SaiArsProfileTraits::Attributes::PortLoadPastWeight
        arsProfilePortLoadPastWeightAttribute{kPastWeight()};
    SaiArsProfileTraits::Attributes::LoadPastMinVal
        arsProfileLoadPastMinValAttribute{kLoadPastMinVal()};
    SaiArsProfileTraits::Attributes::LoadPastMaxVal
        arsProfileLoadPastMaxValAttribute{kLoadPastMaxVal()};
    SaiArsProfileTraits::Attributes::PortLoadFuture
        arsProfilePortLoadFutureAttribute{true};
    SaiArsProfileTraits::Attributes::PortLoadFutureWeight
        arsProfilePortLoadFutureWeightAttribute{kFutureWeight()};
    SaiArsProfileTraits::Attributes::LoadFutureMinVal
        arsProfileLoadFutureMinValAttribute{kLoadFutureMinVal()};
    SaiArsProfileTraits::Attributes::LoadFutureMaxVal
        arsProfileLoadFutureMaxValAttribute{kLoadFutureMaxVal()};
    SaiArsProfileTraits::Attributes::PortLoadCurrent
        arsProfilePortLoadCurrentAttribute{true};
    SaiArsProfileTraits::Attributes::PortLoadExponent
        arsProfilePortLoadExponentAttribute{kExponent()};
    SaiArsProfileTraits::Attributes::LoadCurrentMinVal
        arsProfileLoadCurrentMinValAttribute{kLoadCurrentMinVal()};
    SaiArsProfileTraits::Attributes::LoadCurrentMaxVal
        arsProfileLoadCurrentMaxValAttribute{kLoadCurrentMaxVal()};

    return arsProfileApi->create<SaiArsProfileTraits>(
        {
            arsProfileAlgoAttribute,
            arsProfileSamplingIntervalAttribute,
            arsProfileRandomSeedAttribute,
            arsProfileEnableIPv4Attribute,
            arsProfileEnableIPv6Attribute,
            arsProfilePortLoadPastAttribute,
            arsProfilePortLoadPastWeightAttribute,
            arsProfileLoadPastMinValAttribute,
            arsProfileLoadPastMaxValAttribute,
            arsProfilePortLoadFutureAttribute,
            arsProfilePortLoadFutureWeightAttribute,
            arsProfileLoadFutureMinValAttribute,
            arsProfileLoadFutureMaxValAttribute,
            arsProfilePortLoadCurrentAttribute,
            arsProfilePortLoadExponentAttribute,
            arsProfileLoadCurrentMinValAttribute,
            arsProfileLoadCurrentMaxValAttribute,
        },
        0);
  }

  void checkArsProfile(ArsProfileSaiId arsProfileId) const {
    EXPECT_EQ(arsProfileId, fs->arsProfileManager.get(arsProfileId).id);
  }

  sai_uint32_t kSamplingInterval() const {
    return 10000;
  }

  sai_uint32_t kRandomSeed() const {
    return 0x5555;
  }

  sai_uint32_t kPastWeight() const {
    return 60;
  }

  sai_uint32_t kLoadPastMinVal() const {
    return 1000;
  }

  sai_uint32_t kLoadPastMaxVal() const {
    return 10000;
  }

  sai_uint32_t kFutureWeight() const {
    return 20;
  }

  sai_uint32_t kLoadFutureMinVal() const {
    return 500;
  }

  sai_uint32_t kLoadFutureMaxVal() const {
    return 5000;
  }

  sai_uint8_t kExponent() const {
    return 5;
  }

  sai_uint32_t kLoadCurrentMinVal() const {
    return 500;
  }

  sai_uint32_t kLoadCurrentMaxVal() const {
    return 5000;
  }

  void checkArsProfileAttrs(
      ArsProfileSaiId arsProfileId,
      uint32_t samplingInterval,
      uint32_t randomSeed,
      uint32_t pastWeight,
      uint32_t pastMinVal,
      uint32_t pastMaxVal,
      uint32_t futureWeight,
      uint32_t futureMinVal,
      uint32_t futureMaxVal,
      uint32_t currentWeight,
      uint32_t currentMinVal,
      uint32_t currentMaxVal) {
    auto arsProfileAlgoGot = arsProfileApi->getAttribute(
        arsProfileId, SaiArsProfileTraits::Attributes::Algo());
    auto arsProfileSamplingIntervalGot = arsProfileApi->getAttribute(
        arsProfileId, SaiArsProfileTraits::Attributes::SamplingInterval());
    auto arsProfileRandomSeedGot = arsProfileApi->getAttribute(
        arsProfileId, SaiArsProfileTraits::Attributes::RandomSeed());
    auto arsProfileEnableIPv4Got = arsProfileApi->getAttribute(
        arsProfileId, SaiArsProfileTraits::Attributes::EnableIPv4());
    auto arsProfileEnableIPv6Got = arsProfileApi->getAttribute(
        arsProfileId, SaiArsProfileTraits::Attributes::EnableIPv6());
    auto arsProfilePortLoadPastGot = arsProfileApi->getAttribute(
        arsProfileId, SaiArsProfileTraits::Attributes::PortLoadPast());
    auto arsProfilePortLoadPastWeightGot = arsProfileApi->getAttribute(
        arsProfileId, SaiArsProfileTraits::Attributes::PortLoadPastWeight());
    auto arsProfileLoadPastMinValGot = arsProfileApi->getAttribute(
        arsProfileId, SaiArsProfileTraits::Attributes::LoadPastMinVal());
    auto arsProfileLoadPastMaxValGot = arsProfileApi->getAttribute(
        arsProfileId, SaiArsProfileTraits::Attributes::LoadPastMaxVal());
    auto arsProfilePortLoadFutureGot = arsProfileApi->getAttribute(
        arsProfileId, SaiArsProfileTraits::Attributes::PortLoadFuture());
    auto arsProfilePortLoadFutureWeightGot = arsProfileApi->getAttribute(
        arsProfileId, SaiArsProfileTraits::Attributes::PortLoadFutureWeight());
    auto arsProfileLoadFutureMinValGot = arsProfileApi->getAttribute(
        arsProfileId, SaiArsProfileTraits::Attributes::LoadFutureMinVal());
    auto arsProfileLoadFutureMaxValGot = arsProfileApi->getAttribute(
        arsProfileId, SaiArsProfileTraits::Attributes::LoadFutureMaxVal());
    auto arsProfilePortLoadCurrentGot = arsProfileApi->getAttribute(
        arsProfileId, SaiArsProfileTraits::Attributes::PortLoadCurrent());
    auto arsProfilePortLoadExponentGot = arsProfileApi->getAttribute(
        arsProfileId, SaiArsProfileTraits::Attributes::PortLoadExponent());
    auto arsProfileLoadCurrentMinValGot = arsProfileApi->getAttribute(
        arsProfileId, SaiArsProfileTraits::Attributes::LoadCurrentMinVal());
    auto arsProfileLoadCurrentMaxValGot = arsProfileApi->getAttribute(
        arsProfileId, SaiArsProfileTraits::Attributes::LoadCurrentMaxVal());

    EXPECT_EQ(arsProfileAlgoGot, SAI_ARS_PROFILE_ALGO_EWMA);
    EXPECT_EQ(arsProfileSamplingIntervalGot, samplingInterval);
    EXPECT_EQ(arsProfileRandomSeedGot, randomSeed);
    EXPECT_EQ(arsProfileEnableIPv4Got, false);
    EXPECT_EQ(arsProfileEnableIPv6Got, false);
    EXPECT_EQ(arsProfilePortLoadPastGot, true);
    EXPECT_EQ(arsProfilePortLoadPastWeightGot, pastWeight);
    EXPECT_EQ(arsProfileLoadPastMinValGot, pastMinVal);
    EXPECT_EQ(arsProfileLoadPastMaxValGot, pastMaxVal);
    EXPECT_EQ(arsProfilePortLoadFutureGot, true);
    EXPECT_EQ(arsProfilePortLoadFutureWeightGot, futureWeight);
    EXPECT_EQ(arsProfileLoadFutureMinValGot, futureMinVal);
    EXPECT_EQ(arsProfileLoadFutureMaxValGot, futureMaxVal);
    EXPECT_EQ(arsProfilePortLoadCurrentGot, true);
    EXPECT_EQ(arsProfilePortLoadExponentGot, currentWeight);
    EXPECT_EQ(arsProfileLoadCurrentMinValGot, currentMinVal);
    EXPECT_EQ(arsProfileLoadCurrentMaxValGot, currentMaxVal);
  }

  std::shared_ptr<FakeSai> fs;
  std::unique_ptr<ArsProfileApi> arsProfileApi;
};

TEST_F(ArsProfileApiTest, createArsProfile) {
  auto arsProfileId = createArsProfile();
  checkArsProfile(arsProfileId);
}

TEST_F(ArsProfileApiTest, removeArsProfile) {
  auto arsProfileId = createArsProfile();
  checkArsProfile(arsProfileId);
  arsProfileApi->remove(arsProfileId);
}

TEST_F(ArsProfileApiTest, getArsProfileAttribute) {
  auto arsProfileId = createArsProfile();
  checkArsProfile(arsProfileId);

  checkArsProfileAttrs(
      arsProfileId,
      kSamplingInterval(),
      kRandomSeed(),
      kPastWeight(),
      kLoadPastMinVal(),
      kLoadPastMaxVal(),
      kFutureWeight(),
      kLoadFutureMinVal(),
      kLoadFutureMaxVal(),
      kExponent(),
      kLoadCurrentMinVal(),
      kLoadCurrentMaxVal());
}

TEST_F(ArsProfileApiTest, setArsProfileAttribute) {
  auto arsProfileId = createArsProfile();
  checkArsProfile(arsProfileId);

  SaiArsProfileTraits::Attributes::Algo arsProfileAlgoAttribute{
      SAI_ARS_PROFILE_ALGO_EWMA};
  SaiArsProfileTraits::Attributes::SamplingInterval
      arsProfileSamplingIntervalAttribute{60000};
  SaiArsProfileTraits::Attributes::RandomSeed arsProfileRandomSeedAttribute{
      0x2222};
  SaiArsProfileTraits::Attributes::PortLoadPastWeight
      arsProfilePortLoadPastWeightAttribute{40};
  SaiArsProfileTraits::Attributes::LoadPastMinVal
      arsProfileLoadPastMinValAttribute{4000};
  SaiArsProfileTraits::Attributes::LoadPastMaxVal
      arsProfileLoadPastMaxValAttribute{40000};
  SaiArsProfileTraits::Attributes::PortLoadFutureWeight
      arsProfilePortLoadFutureWeightAttribute{50};
  SaiArsProfileTraits::Attributes::LoadFutureMinVal
      arsProfileLoadFutureMinValAttribute{5000};
  SaiArsProfileTraits::Attributes::LoadFutureMaxVal
      arsProfileLoadFutureMaxValAttribute{50000};
  SaiArsProfileTraits::Attributes::PortLoadExponent
      arsProfilePortLoadExponentAttribute{60};
  SaiArsProfileTraits::Attributes::LoadCurrentMinVal
      arsProfileLoadCurrentMinValAttribute{6000};
  SaiArsProfileTraits::Attributes::LoadCurrentMaxVal
      arsProfileLoadCurrentMaxValAttribute{60000};

  arsProfileApi->setAttribute(arsProfileId, arsProfileAlgoAttribute);
  arsProfileApi->setAttribute(
      arsProfileId, arsProfileSamplingIntervalAttribute);
  arsProfileApi->setAttribute(arsProfileId, arsProfileRandomSeedAttribute);
  arsProfileApi->setAttribute(
      arsProfileId, arsProfilePortLoadPastWeightAttribute);
  arsProfileApi->setAttribute(arsProfileId, arsProfileLoadPastMinValAttribute);
  arsProfileApi->setAttribute(arsProfileId, arsProfileLoadPastMaxValAttribute);
  arsProfileApi->setAttribute(
      arsProfileId, arsProfilePortLoadFutureWeightAttribute);
  arsProfileApi->setAttribute(
      arsProfileId, arsProfileLoadFutureMinValAttribute);
  arsProfileApi->setAttribute(
      arsProfileId, arsProfileLoadFutureMaxValAttribute);
  arsProfileApi->setAttribute(
      arsProfileId, arsProfilePortLoadExponentAttribute);
  arsProfileApi->setAttribute(
      arsProfileId, arsProfileLoadCurrentMinValAttribute);
  arsProfileApi->setAttribute(
      arsProfileId, arsProfileLoadCurrentMaxValAttribute);

  checkArsProfileAttrs(
      arsProfileId,
      60000,
      0x2222,
      40,
      4000,
      40000,
      50,
      5000,
      50000,
      60,
      6000,
      60000);
}
