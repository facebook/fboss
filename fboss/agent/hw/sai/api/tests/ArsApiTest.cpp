/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/api/ArsApi.h"
#include "fboss/agent/hw/sai/fake/FakeSai.h"

#include <folly/logging/xlog.h>

#include <gtest/gtest.h>

using namespace facebook::fboss;

class ArsApiTest : public ::testing::Test {
 public:
  void SetUp() override {
    fs = FakeSai::getInstance();
    sai_api_initialize(0, nullptr);
    arsApi = std::make_unique<ArsApi>();
  }

  SaiArsTraits::CreateAttributes getArsAttributes(
      sai_int32_t mode = SAI_ARS_MODE_FLOWLET_QUALITY,
      std::optional<sai_uint32_t> idleTime = std::nullopt,
      std::optional<sai_uint32_t> maxFlows = std::nullopt) const {
    return SaiArsTraits::CreateAttributes{
        SaiArsTraits::Attributes::Mode{mode},
        SaiArsTraits::Attributes::IdleTime{idleTime.value_or(kIdleTime())},
        SaiArsTraits::Attributes::MaxFlows{maxFlows.value_or(kMaxFlows())},
        SaiArsTraits::Attributes::PrimaryPathQualityThreshold{
            kPrimaryPathQualityThreshold()},
        SaiArsTraits::Attributes::AlternatePathCost{kAlternatePathCost()},
        SaiArsTraits::Attributes::AlternatePathBias{kAlternatePathBias()},
        std::nullopt, // NextHopGroupType
        std::nullopt}; // SourcePortPrune
  }

  ArsSaiId createArs() const {
    return arsApi->create<SaiArsTraits>(getArsAttributes(), 0);
  }

  void checkArs(ArsSaiId arsId) const {
    EXPECT_EQ(arsId, fs->arsManager.get(arsId).id);
  }

  sai_uint32_t kIdleTime() const {
    return 10000;
  }

  sai_uint32_t kMaxFlows() const {
    return 2000;
  }

  sai_uint32_t kPrimaryPathQualityThreshold() const {
    return 100;
  }

  sai_uint32_t kAlternatePathCost() const {
    return 50;
  }

  sai_uint32_t kAlternatePathBias() const {
    return 25;
  }

  std::shared_ptr<FakeSai> fs;
  std::unique_ptr<ArsApi> arsApi;
};

TEST_F(ArsApiTest, createArs) {
  auto arsId = createArs();
  checkArs(arsId);
}

// Mode, IdleTime and MaxFlows are part of SaiArsTraits::AdapterHostKey, so
// CreateAttributes differing only in those attributes project to different
// keys. This is what keeps the standby DLB group ARS object distinct from the
// primary in the SaiStore.
TEST_F(ArsApiTest, adapterHostKeyIncludesMode) {
  auto flowletKey =
      getAdapterHostKey(getArsAttributes(SAI_ARS_MODE_FLOWLET_QUALITY));
  auto fixedKey = getAdapterHostKey(getArsAttributes(SAI_ARS_MODE_FIXED));

  EXPECT_NE(flowletKey, fixedKey);
  EXPECT_EQ(
      std::get<SaiArsTraits::Attributes::Mode>(fixedKey).value(),
      SAI_ARS_MODE_FIXED);
}

TEST_F(ArsApiTest, adapterHostKeyIncludesIdleTimeAndMaxFlows) {
  auto baseKey = getAdapterHostKey(getArsAttributes());
  auto idleTimeKey = getAdapterHostKey(
      getArsAttributes(SAI_ARS_MODE_FLOWLET_QUALITY, kIdleTime() * 2));
  auto maxFlowsKey = getAdapterHostKey(getArsAttributes(
      SAI_ARS_MODE_FLOWLET_QUALITY, std::nullopt, kMaxFlows() * 2));

  EXPECT_NE(baseKey, idleTimeKey);
  EXPECT_NE(baseKey, maxFlowsKey);
  EXPECT_NE(idleTimeKey, maxFlowsKey);
  EXPECT_EQ(
      std::get<std::optional<SaiArsTraits::Attributes::IdleTime>>(idleTimeKey)
          ->value(),
      kIdleTime() * 2);
  EXPECT_EQ(
      std::get<std::optional<SaiArsTraits::Attributes::MaxFlows>>(maxFlowsKey)
          ->value(),
      kMaxFlows() * 2);
}

TEST_F(ArsApiTest, removeArs) {
  auto arsId = createArs();
  checkArs(arsId);
  arsApi->remove(arsId);
}

TEST_F(ArsApiTest, multipleArs) {
  auto arsId1 = createArs();
  checkArs(arsId1);
  auto arsId2 = createArs();
  checkArs(arsId2);
  EXPECT_NE(arsId1, arsId2);
}

TEST_F(ArsApiTest, getArsAttribute) {
  auto arsId = createArs();
  checkArs(arsId);

  auto arsModeGot =
      arsApi->getAttribute(arsId, SaiArsTraits::Attributes::Mode());
  auto arsIdleTimeGot =
      arsApi->getAttribute(arsId, SaiArsTraits::Attributes::IdleTime());
  auto arsMaxFlowsGot =
      arsApi->getAttribute(arsId, SaiArsTraits::Attributes::MaxFlows());
  auto primaryPathQualityThresholdGot = arsApi->getAttribute(
      arsId, SaiArsTraits::Attributes::PrimaryPathQualityThreshold());
  auto alternatePathCostGot = arsApi->getAttribute(
      arsId, SaiArsTraits::Attributes::AlternatePathCost());
  auto alternatePathBiasGot = arsApi->getAttribute(
      arsId, SaiArsTraits::Attributes::AlternatePathBias());

  EXPECT_EQ(arsModeGot, SAI_ARS_MODE_FLOWLET_QUALITY);
  EXPECT_EQ(arsIdleTimeGot, kIdleTime());
  EXPECT_EQ(arsMaxFlowsGot, kMaxFlows());
  EXPECT_EQ(primaryPathQualityThresholdGot, kPrimaryPathQualityThreshold());
  EXPECT_EQ(alternatePathCostGot, kAlternatePathCost());
  EXPECT_EQ(alternatePathBiasGot, kAlternatePathBias());
}

TEST_F(ArsApiTest, setArsAttribute) {
  auto arsId = createArs();
  checkArs(arsId);

  SaiArsTraits::Attributes::Mode arsModeAttribute{SAI_ARS_MODE_FLOWLET_QUALITY};
  SaiArsTraits::Attributes::IdleTime arsIdleTimeAttribute{20000};
  SaiArsTraits::Attributes::MaxFlows arsMaxFlowsAttribute{1000};
  SaiArsTraits::Attributes::PrimaryPathQualityThreshold
      primaryPathQualityThresholdAttribute{200};
  SaiArsTraits::Attributes::AlternatePathCost alternatePathCostAttribute{75};
  SaiArsTraits::Attributes::AlternatePathBias alternatePathBiasAttribute{40};

  arsApi->setAttribute(arsId, arsModeAttribute);
  arsApi->setAttribute(arsId, arsIdleTimeAttribute);
  arsApi->setAttribute(arsId, arsMaxFlowsAttribute);
  arsApi->setAttribute(arsId, primaryPathQualityThresholdAttribute);
  arsApi->setAttribute(arsId, alternatePathCostAttribute);
  arsApi->setAttribute(arsId, alternatePathBiasAttribute);

  auto arsModeGot =
      arsApi->getAttribute(arsId, SaiArsTraits::Attributes::Mode());
  auto arsIdleTimeGot =
      arsApi->getAttribute(arsId, SaiArsTraits::Attributes::IdleTime());
  auto arsMaxFlowsGot =
      arsApi->getAttribute(arsId, SaiArsTraits::Attributes::MaxFlows());
  auto primaryPathQualityThresholdGot = arsApi->getAttribute(
      arsId, SaiArsTraits::Attributes::PrimaryPathQualityThreshold());
  auto alternatePathCostGot = arsApi->getAttribute(
      arsId, SaiArsTraits::Attributes::AlternatePathCost());
  auto alternatePathBiasGot = arsApi->getAttribute(
      arsId, SaiArsTraits::Attributes::AlternatePathBias());

  EXPECT_EQ(arsModeGot, SAI_ARS_MODE_FLOWLET_QUALITY);
  EXPECT_EQ(arsIdleTimeGot, 20000);
  EXPECT_EQ(arsMaxFlowsGot, 1000);
  EXPECT_EQ(primaryPathQualityThresholdGot, 200);
  EXPECT_EQ(alternatePathCostGot, 75);
  EXPECT_EQ(alternatePathBiasGot, 40);
}
