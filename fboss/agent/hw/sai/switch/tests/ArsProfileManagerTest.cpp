/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/sai/switch/SaiArsProfileManager.h"
#include "fboss/agent/hw/sai/switch/tests/ManagerTestBase.h"
#include "fboss/agent/types.h"

#include <gtest/gtest.h>

using namespace facebook::fboss;

class ArsProfileManagerTest : public ManagerTestBase {};

TEST_F(ArsProfileManagerTest, testArsProfileManager) {
  std::shared_ptr<FlowletSwitchingConfig> oldFsc =
      std::make_shared<FlowletSwitchingConfig>();
  oldFsc->setDynamicSampleRate(1000000);
  oldFsc->setDynamicEgressLoadExponent(2);
  oldFsc->setDynamicQueueExponent(3);
  oldFsc->setDynamicPhysicalQueueExponent(4);
  oldFsc->setDynamicEgressMinThresholdBytes(10000);
  oldFsc->setDynamicEgressMaxThresholdBytes(20000);
  oldFsc->setDynamicQueueMinThresholdBytes(30000);
  oldFsc->setDynamicQueueMaxThresholdBytes(40000);
  // verify ars profile object creation
  saiManagerTable->arsProfileManager().addArsProfile(oldFsc);
  auto arsProfileHandle =
      saiManagerTable->arsProfileManager().getArsProfileHandle();
  auto arsProfileSaiId = arsProfileHandle->arsProfile->adapterKey();
  auto samplingRate = saiApiTable->arsProfileApi().getAttribute(
      arsProfileSaiId, SaiArsProfileTraits::Attributes::SamplingInterval{});
  EXPECT_EQ(samplingRate, 1000000);
  auto portLoadPastWeight = saiApiTable->arsProfileApi().getAttribute(
      arsProfileSaiId, SaiArsProfileTraits::Attributes::PortLoadPastWeight{});
  EXPECT_EQ(portLoadPastWeight, 2);
  auto portLoadFutureWeight = saiApiTable->arsProfileApi().getAttribute(
      arsProfileSaiId, SaiArsProfileTraits::Attributes::PortLoadFutureWeight{});
  EXPECT_EQ(portLoadFutureWeight, 3);
  auto portLoadExponent = saiApiTable->arsProfileApi().getAttribute(
      arsProfileSaiId, SaiArsProfileTraits::Attributes::PortLoadExponent{});
  EXPECT_EQ(portLoadExponent, 4);
  auto loadPastMinVal = saiApiTable->arsProfileApi().getAttribute(
      arsProfileSaiId, SaiArsProfileTraits::Attributes::LoadPastMinVal{});
  EXPECT_EQ(loadPastMinVal, 10000);
  auto loadPastMaxVal = saiApiTable->arsProfileApi().getAttribute(
      arsProfileSaiId, SaiArsProfileTraits::Attributes::LoadPastMaxVal{});
  EXPECT_EQ(loadPastMaxVal, 20000);
  auto loadFutureMinVal = saiApiTable->arsProfileApi().getAttribute(
      arsProfileSaiId, SaiArsProfileTraits::Attributes::LoadFutureMinVal{});
  EXPECT_EQ(loadFutureMinVal, 30000);
  auto loadFutureMaxVal = saiApiTable->arsProfileApi().getAttribute(
      arsProfileSaiId, SaiArsProfileTraits::Attributes::LoadFutureMaxVal{});
  EXPECT_EQ(loadFutureMaxVal, 40000);
  auto loadCurrentMinVal = saiApiTable->arsProfileApi().getAttribute(
      arsProfileSaiId, SaiArsProfileTraits::Attributes::LoadCurrentMinVal{});
  EXPECT_EQ(loadCurrentMinVal, 15000);
  auto loadCurrentMaxVal = saiApiTable->arsProfileApi().getAttribute(
      arsProfileSaiId, SaiArsProfileTraits::Attributes::LoadCurrentMaxVal{});
  EXPECT_EQ(loadCurrentMaxVal, 20000);

  std::shared_ptr<FlowletSwitchingConfig> newFsc =
      std::make_shared<FlowletSwitchingConfig>();
  newFsc->setDynamicSampleRate(2000000);
  newFsc->setDynamicEgressLoadExponent(4);
  newFsc->setDynamicQueueExponent(6);
  newFsc->setDynamicPhysicalQueueExponent(8);
  newFsc->setDynamicEgressMinThresholdBytes(20000);
  newFsc->setDynamicEgressMaxThresholdBytes(40000);
  newFsc->setDynamicQueueMinThresholdBytes(60000);
  newFsc->setDynamicQueueMaxThresholdBytes(80000);
  // verify ars profile config change
  saiManagerTable->arsProfileManager().changeArsProfile(oldFsc, newFsc);
  samplingRate = saiApiTable->arsProfileApi().getAttribute(
      arsProfileSaiId, SaiArsProfileTraits::Attributes::SamplingInterval{});
  EXPECT_EQ(samplingRate, 2000000);
  portLoadPastWeight = saiApiTable->arsProfileApi().getAttribute(
      arsProfileSaiId, SaiArsProfileTraits::Attributes::PortLoadPastWeight{});
  EXPECT_EQ(portLoadPastWeight, 4);
  portLoadFutureWeight = saiApiTable->arsProfileApi().getAttribute(
      arsProfileSaiId, SaiArsProfileTraits::Attributes::PortLoadFutureWeight{});
  EXPECT_EQ(portLoadFutureWeight, 6);
  portLoadExponent = saiApiTable->arsProfileApi().getAttribute(
      arsProfileSaiId, SaiArsProfileTraits::Attributes::PortLoadExponent{});
  EXPECT_EQ(portLoadExponent, 8);
  loadPastMinVal = saiApiTable->arsProfileApi().getAttribute(
      arsProfileSaiId, SaiArsProfileTraits::Attributes::LoadPastMinVal{});
  EXPECT_EQ(loadPastMinVal, 20000);
  loadPastMaxVal = saiApiTable->arsProfileApi().getAttribute(
      arsProfileSaiId, SaiArsProfileTraits::Attributes::LoadPastMaxVal{});
  EXPECT_EQ(loadPastMaxVal, 40000);
  loadFutureMinVal = saiApiTable->arsProfileApi().getAttribute(
      arsProfileSaiId, SaiArsProfileTraits::Attributes::LoadFutureMinVal{});
  EXPECT_EQ(loadFutureMinVal, 60000);
  loadFutureMaxVal = saiApiTable->arsProfileApi().getAttribute(
      arsProfileSaiId, SaiArsProfileTraits::Attributes::LoadFutureMaxVal{});
  EXPECT_EQ(loadFutureMaxVal, 80000);
  loadCurrentMinVal = saiApiTable->arsProfileApi().getAttribute(
      arsProfileSaiId, SaiArsProfileTraits::Attributes::LoadCurrentMinVal{});
  EXPECT_EQ(loadCurrentMinVal, 30000);
  loadCurrentMaxVal = saiApiTable->arsProfileApi().getAttribute(
      arsProfileSaiId, SaiArsProfileTraits::Attributes::LoadCurrentMaxVal{});
  EXPECT_EQ(loadCurrentMaxVal, 40000);
}
