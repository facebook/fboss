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

  ArsSaiId createArs() const {
    SaiArsTraits::Attributes::Mode arsModeAttribute{
        SAI_ARS_MODE_FLOWLET_QUALITY};
    SaiArsTraits::Attributes::IdleTime arsIdleTimeAttribute{kIdleTime()};
    SaiArsTraits::Attributes::MaxFlows arsMaxFlowsAttribute{kMaxFlows()};
    SaiArsTraits::Attributes::PrimaryPathQualityThreshold
        primaryPathQualityThresholdAttribute{kPrimaryPathQualityThreshold()};
    SaiArsTraits::Attributes::AlternatePathCost alternatePathCostAttribute{
        kAlternatePathCost()};
    SaiArsTraits::Attributes::AlternatePathBias alternatePathBiasAttribute{
        kAlternatePathBias()};

    return arsApi->create<SaiArsTraits>(
        {
            arsModeAttribute,
            arsIdleTimeAttribute,
            arsMaxFlowsAttribute,
            primaryPathQualityThresholdAttribute,
            alternatePathCostAttribute,
            alternatePathBiasAttribute,
        },
        0);
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
