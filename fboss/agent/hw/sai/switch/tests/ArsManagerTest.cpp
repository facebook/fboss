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
#include "fboss/agent/hw/sai/switch/SaiArsManager.h"
#include "fboss/agent/hw/sai/switch/tests/ManagerTestBase.h"
#include "fboss/agent/types.h"

#include <gtest/gtest.h>

using namespace facebook::fboss;

class ArsManagerTest : public ManagerTestBase {};

TEST_F(ArsManagerTest, testArsManager) {
  std::shared_ptr<FlowletSwitchingConfig> oldFsc =
      std::make_shared<FlowletSwitchingConfig>();
  oldFsc->setInactivityIntervalUsecs(1000);
  oldFsc->setFlowletTableSize(2000);
  oldFsc->setSwitchingMode(cfg::SwitchingMode::FLOWLET_QUALITY);
  // verify ars object creation
  saiManagerTable->arsManager().addArs(oldFsc);
  auto arsHandle = saiManagerTable->arsManager().getArsHandle();
  auto arsSaiId = arsHandle->ars->adapterKey();
  auto mode = saiApiTable->arsApi().getAttribute(
      arsSaiId, SaiArsTraits::Attributes::Mode{});
  EXPECT_EQ(mode, SAI_ARS_MODE_FLOWLET_QUALITY);
  auto maxFlows = saiApiTable->arsApi().getAttribute(
      arsSaiId, SaiArsTraits::Attributes::MaxFlows{});
  EXPECT_EQ(maxFlows, 2000);
  auto idleTime = saiApiTable->arsApi().getAttribute(
      arsSaiId, SaiArsTraits::Attributes::IdleTime{});
  EXPECT_EQ(idleTime, 1000);

  // Verify that no alternate ARS object is created when alternate path
  // attributes are not set
  auto alternateArsHandle =
      saiManagerTable->arsManager().getAlternateMemberArsHandle();
  EXPECT_EQ(alternateArsHandle->ars, nullptr);

  std::shared_ptr<FlowletSwitchingConfig> newFsc =
      std::make_shared<FlowletSwitchingConfig>();
  newFsc->setInactivityIntervalUsecs(2000);
  newFsc->setFlowletTableSize(1000);
  newFsc->setSwitchingMode(cfg::SwitchingMode::PER_PACKET_QUALITY);
  // verify ars config change
  saiManagerTable->arsManager().changeArs(oldFsc, newFsc);
  // Get the new ARS ID after changeArs since it creates a new ARS object
  arsHandle = saiManagerTable->arsManager().getArsHandle();
  arsSaiId = arsHandle->ars->adapterKey();
  mode = saiApiTable->arsApi().getAttribute(
      arsSaiId, SaiArsTraits::Attributes::Mode{});
  EXPECT_EQ(mode, SAI_ARS_MODE_PER_PACKET_QUALITY);
  maxFlows = saiApiTable->arsApi().getAttribute(
      arsSaiId, SaiArsTraits::Attributes::MaxFlows{});
  EXPECT_EQ(maxFlows, 1000);
  idleTime = saiApiTable->arsApi().getAttribute(
      arsSaiId, SaiArsTraits::Attributes::IdleTime{});
  EXPECT_EQ(idleTime, 2000);

  // Verify that still no alternate ARS object is created
  alternateArsHandle =
      saiManagerTable->arsManager().getAlternateMemberArsHandle();
  EXPECT_EQ(alternateArsHandle->ars, nullptr);
}

TEST_F(ArsManagerTest, testAlternateArsManager) {
  // Test with alternate path attributes set
  std::shared_ptr<FlowletSwitchingConfig> fscWithAlternate =
      std::make_shared<FlowletSwitchingConfig>();
  fscWithAlternate->setInactivityIntervalUsecs(1500);
  fscWithAlternate->setFlowletTableSize(3000);
  fscWithAlternate->setSwitchingMode(cfg::SwitchingMode::FLOWLET_QUALITY);
  fscWithAlternate->setAlternatePathCost(75);
  fscWithAlternate->setAlternatePathBias(40);
  fscWithAlternate->setPrimaryPathQualityThreshold(200);

  // Add ARS with alternate path attributes
  saiManagerTable->arsManager().addArs(fscWithAlternate);

  // Verify primary ARS object creation
  auto arsHandle = saiManagerTable->arsManager().getArsHandle();
  EXPECT_NE(arsHandle->ars, nullptr);
  auto arsSaiId = arsHandle->ars->adapterKey();

  auto mode = saiApiTable->arsApi().getAttribute(
      arsSaiId, SaiArsTraits::Attributes::Mode{});
  EXPECT_EQ(mode, SAI_ARS_MODE_FLOWLET_QUALITY);
  auto maxFlows = saiApiTable->arsApi().getAttribute(
      arsSaiId, SaiArsTraits::Attributes::MaxFlows{});
  EXPECT_EQ(maxFlows, 3000);
  auto idleTime = saiApiTable->arsApi().getAttribute(
      arsSaiId, SaiArsTraits::Attributes::IdleTime{});
  EXPECT_EQ(idleTime, 1500);

  // Verify alternate ARS object creation
  auto alternateArsHandle =
      saiManagerTable->arsManager().getAlternateMemberArsHandle();
  EXPECT_NE(alternateArsHandle->ars, nullptr);
  auto alternateArsSaiId = alternateArsHandle->ars->adapterKey();

  // Verify alternate ARS has the same basic attributes
  auto alternateMode = saiApiTable->arsApi().getAttribute(
      alternateArsSaiId, SaiArsTraits::Attributes::Mode{});
  EXPECT_EQ(alternateMode, SAI_ARS_MODE_FLOWLET_QUALITY);
  auto alternateMaxFlows = saiApiTable->arsApi().getAttribute(
      alternateArsSaiId, SaiArsTraits::Attributes::MaxFlows{});
  EXPECT_EQ(alternateMaxFlows, 3000);
  auto alternateIdleTime = saiApiTable->arsApi().getAttribute(
      alternateArsSaiId, SaiArsTraits::Attributes::IdleTime{});
  EXPECT_EQ(alternateIdleTime, 1500);

  // Verify alternate ARS has the additional attributes
  auto alternatePathCost = saiApiTable->arsApi().getAttribute(
      alternateArsSaiId, SaiArsTraits::Attributes::AlternatePathCost{});
  EXPECT_EQ(alternatePathCost, 75);
  auto alternatePathBias = saiApiTable->arsApi().getAttribute(
      alternateArsSaiId, SaiArsTraits::Attributes::AlternatePathBias{});
  EXPECT_EQ(alternatePathBias, 40);
  auto primaryPathQualityThreshold = saiApiTable->arsApi().getAttribute(
      alternateArsSaiId,
      SaiArsTraits::Attributes::PrimaryPathQualityThreshold{});
  EXPECT_EQ(primaryPathQualityThreshold, 200);

  // Test removal - both ARS objects should be cleaned up
  saiManagerTable->arsManager().removeArs(fscWithAlternate);
  arsHandle = saiManagerTable->arsManager().getArsHandle();
  EXPECT_EQ(arsHandle->ars, nullptr);
  alternateArsHandle =
      saiManagerTable->arsManager().getAlternateMemberArsHandle();
  EXPECT_EQ(alternateArsHandle->ars, nullptr);
}
