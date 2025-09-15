/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/ApplyThriftConfig.h"
#include "fboss/agent/state/FlowletSwitchingConfig.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/TestUtils.h"

#include <gtest/gtest.h>

namespace {
facebook::fboss::HwSwitchMatcher scope() {
  return facebook::fboss::HwSwitchMatcher{
      std::unordered_set<facebook::fboss::SwitchID>{
          facebook::fboss::SwitchID(0)}};
}
} // namespace

using namespace facebook::fboss;

TEST(FlowletSwitching, addUpdate) {
  auto state = std::make_shared<SwitchState>();

  cfg::FlowletSwitchingConfig flowletCfg;
  auto flowletSwitchingConfig = std::make_shared<FlowletSwitchingConfig>();
  // convert to state
  flowletSwitchingConfig->fromThrift(flowletCfg);

  // update the state with flowletCfg
  auto switchSettings = std::make_shared<SwitchSettings>();
  switchSettings->setFlowletSwitchingConfig(flowletSwitchingConfig);
  auto multiSwitchSwitchSettings = std::make_shared<MultiSwitchSettings>();
  multiSwitchSwitchSettings->addNode(scope().matcherString(), switchSettings);
  state->resetSwitchSettings(multiSwitchSwitchSettings);
  auto flowletCfg1 = state->getFlowletSwitchingConfig();
  EXPECT_TRUE(flowletCfg1);
  EXPECT_FALSE(flowletCfg1->isPublished());
  // default should kick in
  EXPECT_EQ(
      flowletCfg1->getFlowletTableSize(),
      cfg::switch_config_constants::DEFAULT_FLOWLET_TABLE_SIZE());
  EXPECT_EQ(
      flowletCfg1->getSwitchingMode(), cfg::SwitchingMode::FLOWLET_QUALITY);
  EXPECT_EQ(
      flowletCfg1->getBackupSwitchingMode(),
      cfg::SwitchingMode::FIXED_ASSIGNMENT);

  flowletCfg.inactivityIntervalUsecs() = 60;
  flowletCfg.flowletTableSize() = 1024;
  flowletCfg.dynamicEgressLoadExponent() = 3;
  flowletCfg.dynamicQueueExponent() = 3;
  flowletCfg.dynamicQueueMinThresholdBytes() = 0x5D44;
  flowletCfg.dynamicQueueMaxThresholdBytes() = 0xFE00;
  flowletCfg.dynamicSampleRate() = 62500;
  flowletCfg.dynamicEgressMinThresholdBytes() = 1000;
  flowletCfg.dynamicEgressMaxThresholdBytes() = 10000;
  flowletCfg.dynamicPhysicalQueueExponent() = 3;
  flowletCfg.maxLinks() = 9;
  flowletCfg.switchingMode() = cfg::SwitchingMode::PER_PACKET_QUALITY;
  flowletCfg.backupSwitchingMode() = cfg::SwitchingMode::PER_PACKET_RANDOM;
  flowletCfg.primaryPathQualityThreshold() = 100;
  flowletCfg.alternatePathCost() = 50;
  flowletCfg.alternatePathBias() = 25;

  flowletSwitchingConfig->fromThrift(flowletCfg);
  switchSettings = std::make_shared<SwitchSettings>();
  switchSettings->setFlowletSwitchingConfig(flowletSwitchingConfig);
  multiSwitchSwitchSettings = std::make_shared<MultiSwitchSettings>();
  multiSwitchSwitchSettings->addNode(scope().matcherString(), switchSettings);
  state->resetSwitchSettings(multiSwitchSwitchSettings);
  auto flowletCfg2 = state->getFlowletSwitchingConfig();
  EXPECT_TRUE(flowletCfg2);
  EXPECT_FALSE(flowletCfg2->isPublished());
  EXPECT_EQ(flowletCfg2->getInactivityIntervalUsecs(), 60);
  EXPECT_EQ(flowletCfg2->getFlowletTableSize(), 1024);
  EXPECT_EQ(flowletCfg2->getDynamicEgressLoadExponent(), 3);
  EXPECT_EQ(flowletCfg2->getDynamicQueueExponent(), 3);
  EXPECT_EQ(flowletCfg2->getDynamicQueueMinThresholdBytes(), 0x5D44);
  EXPECT_EQ(flowletCfg2->getDynamicQueueMaxThresholdBytes(), 0xFE00);
  EXPECT_EQ(flowletCfg2->getDynamicSampleRate(), 62500);
  EXPECT_EQ(flowletCfg2->getDynamicEgressMinThresholdBytes(), 1000);
  EXPECT_EQ(flowletCfg2->getDynamicEgressMaxThresholdBytes(), 10000);
  EXPECT_EQ(flowletCfg2->getDynamicPhysicalQueueExponent(), 3);
  EXPECT_EQ(flowletCfg2->getMaxLinks(), 9);
  EXPECT_EQ(
      flowletCfg2->getSwitchingMode(), cfg::SwitchingMode::PER_PACKET_QUALITY);
  EXPECT_EQ(
      flowletCfg2->getBackupSwitchingMode(),
      cfg::SwitchingMode::PER_PACKET_RANDOM);
  EXPECT_TRUE(flowletCfg2->getPrimaryPathQualityThreshold().has_value());
  EXPECT_EQ(flowletCfg2->getPrimaryPathQualityThreshold().value(), 100);
  EXPECT_TRUE(flowletCfg2->getAlternatePathCost().has_value());
  EXPECT_EQ(flowletCfg2->getAlternatePathCost().value(), 50);
  EXPECT_TRUE(flowletCfg2->getAlternatePathBias().has_value());
  EXPECT_EQ(flowletCfg2->getAlternatePathBias().value(), 25);

  flowletCfg.switchingMode() = cfg::SwitchingMode::FIXED_ASSIGNMENT;
  flowletCfg.backupSwitchingMode() = cfg::SwitchingMode::PER_PACKET_RANDOM;
  flowletSwitchingConfig->fromThrift(flowletCfg);
  switchSettings = std::make_shared<SwitchSettings>();
  switchSettings->setFlowletSwitchingConfig(flowletSwitchingConfig);
  multiSwitchSwitchSettings = std::make_shared<MultiSwitchSettings>();
  multiSwitchSwitchSettings->addNode(scope().matcherString(), switchSettings);
  state->resetSwitchSettings(multiSwitchSwitchSettings);
  flowletCfg2 = state->getFlowletSwitchingConfig();
  EXPECT_TRUE(flowletCfg2);
  EXPECT_FALSE(flowletCfg2->isPublished());
  EXPECT_EQ(
      flowletCfg2->getSwitchingMode(), cfg::SwitchingMode::FIXED_ASSIGNMENT);
  EXPECT_EQ(
      flowletCfg2->getBackupSwitchingMode(),
      cfg::SwitchingMode::PER_PACKET_RANDOM);
}

TEST(FlowletSwitching, publish) {
  cfg::FlowletSwitchingConfig flowletCfg;
  auto flowletSwitchingConfig = std::make_shared<FlowletSwitchingConfig>();
  auto state = std::make_shared<SwitchState>();

  // convert to state
  flowletSwitchingConfig->fromThrift(flowletCfg);
  auto switchSettings = std::make_shared<SwitchSettings>();
  switchSettings->setFlowletSwitchingConfig(flowletSwitchingConfig);
  auto multiSwitchSwitchSettings = std::make_shared<MultiSwitchSettings>();
  multiSwitchSwitchSettings->addNode(scope().matcherString(), switchSettings);
  state->resetSwitchSettings(multiSwitchSwitchSettings);
  // update the state with flowletCfg
  state->publish();
  EXPECT_TRUE(state->getFlowletSwitchingConfig()->isPublished());
}

TEST(FlowletSwitching, serDeserSwitchState) {
  auto state = std::make_shared<SwitchState>();

  cfg::FlowletSwitchingConfig flowletCfg;
  auto flowletSwitchingConfig = std::make_shared<FlowletSwitchingConfig>();

  flowletCfg.inactivityIntervalUsecs() = 60;
  flowletCfg.flowletTableSize() = 1024;
  flowletCfg.dynamicEgressLoadExponent() = 3;
  flowletCfg.dynamicQueueExponent() = 3;
  flowletCfg.dynamicQueueMinThresholdBytes() = 0x5D44;
  flowletCfg.dynamicQueueMaxThresholdBytes() = 0xFE00;
  flowletCfg.dynamicSampleRate() = 62500;
  flowletCfg.dynamicEgressMinThresholdBytes() = 1000;
  flowletCfg.dynamicEgressMaxThresholdBytes() = 10000;
  flowletCfg.dynamicPhysicalQueueExponent() = 3;
  flowletCfg.maxLinks() = 7;
  flowletCfg.switchingMode() = cfg::SwitchingMode::PER_PACKET_QUALITY;
  flowletCfg.backupSwitchingMode() = cfg::SwitchingMode::FIXED_ASSIGNMENT;
  flowletCfg.primaryPathQualityThreshold() = 200;
  flowletCfg.alternatePathCost() = 75;
  flowletCfg.alternatePathBias() = 30;

  // convert to state
  flowletSwitchingConfig->fromThrift(flowletCfg);
  // update the state with flowletCfg
  auto switchSettings = std::make_shared<SwitchSettings>();
  switchSettings->setFlowletSwitchingConfig(flowletSwitchingConfig);
  auto multiSwitchSwitchSettings = std::make_shared<MultiSwitchSettings>();
  multiSwitchSwitchSettings->addNode(scope().matcherString(), switchSettings);
  state->resetSwitchSettings(multiSwitchSwitchSettings);

  auto serialized = state->toThrift();
  auto stateBack = SwitchState::fromThrift(serialized);
  EXPECT_EQ(state->toThrift(), stateBack->toThrift());
}

TEST(FlowletSwitching, applyConfig) {
  auto platform = createMockPlatform();
  auto stateV0 = std::make_shared<SwitchState>();

  cfg::SwitchConfig config;
  cfg::FlowletSwitchingConfig flowletCfg;

  // make any random change, so that new state can be created
  // ensure that flowlet switching is not configured
  *config.switchSettings()->l2LearningMode() = cfg::L2LearningMode::SOFTWARE;
  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  ASSERT_NE(nullptr, stateV1);
  EXPECT_EQ(stateV1->getFlowletSwitchingConfig(), nullptr);

  config.flowletSwitchingConfig() = flowletCfg;
  auto stateV2 = publishAndApplyConfig(stateV1, &config, platform.get());
  ASSERT_NE(nullptr, stateV2);
  auto flowletCfg1 = stateV2->getFlowletSwitchingConfig();
  EXPECT_TRUE(flowletCfg1);
  EXPECT_FALSE(flowletCfg1->isPublished());
  // default should kick in
  EXPECT_EQ(
      flowletCfg1->getFlowletTableSize(),
      cfg::switch_config_constants::DEFAULT_FLOWLET_TABLE_SIZE());
  EXPECT_EQ(
      flowletCfg1->getSwitchingMode(), cfg::SwitchingMode::FLOWLET_QUALITY);
  EXPECT_EQ(
      flowletCfg1->getBackupSwitchingMode(),
      cfg::SwitchingMode::FIXED_ASSIGNMENT);

  // change config
  flowletCfg.inactivityIntervalUsecs() = 60;
  flowletCfg.flowletTableSize() = 1024;
  flowletCfg.dynamicEgressLoadExponent() = 3;
  flowletCfg.dynamicQueueExponent() = 3;
  flowletCfg.dynamicQueueMinThresholdBytes() = 0x5D44;
  flowletCfg.dynamicQueueMaxThresholdBytes() = 0xFE00;
  flowletCfg.dynamicSampleRate() = 62500;
  flowletCfg.dynamicEgressMinThresholdBytes() = 1000;
  flowletCfg.dynamicEgressMaxThresholdBytes() = 10000;
  flowletCfg.dynamicPhysicalQueueExponent() = 3;
  flowletCfg.switchingMode() = cfg::SwitchingMode::PER_PACKET_QUALITY;
  flowletCfg.backupSwitchingMode() = cfg::SwitchingMode::PER_PACKET_RANDOM;
  flowletCfg.primaryPathQualityThreshold() = 150;
  flowletCfg.alternatePathCost() = 60;
  flowletCfg.alternatePathBias() = 35;

  config.flowletSwitchingConfig() = flowletCfg;
  auto stateV3 = publishAndApplyConfig(stateV2, &config, platform.get());
  ASSERT_NE(nullptr, stateV3);
  auto flowletCfg2 = stateV3->getFlowletSwitchingConfig();
  EXPECT_TRUE(flowletCfg2);
  EXPECT_FALSE(flowletCfg2->isPublished());
  EXPECT_EQ(flowletCfg2->getInactivityIntervalUsecs(), 60);
  EXPECT_EQ(flowletCfg2->getFlowletTableSize(), 1024);
  EXPECT_EQ(flowletCfg2->getDynamicEgressLoadExponent(), 3);
  EXPECT_EQ(flowletCfg2->getDynamicQueueExponent(), 3);
  EXPECT_EQ(flowletCfg2->getDynamicQueueMinThresholdBytes(), 0x5D44);
  EXPECT_EQ(flowletCfg2->getDynamicQueueMaxThresholdBytes(), 0xFE00);
  EXPECT_EQ(flowletCfg2->getDynamicSampleRate(), 62500);
  EXPECT_EQ(flowletCfg2->getDynamicEgressMinThresholdBytes(), 1000);
  EXPECT_EQ(flowletCfg2->getDynamicEgressMaxThresholdBytes(), 10000);
  EXPECT_EQ(flowletCfg2->getDynamicPhysicalQueueExponent(), 3);
  EXPECT_EQ(
      flowletCfg2->getSwitchingMode(), cfg::SwitchingMode::PER_PACKET_QUALITY);
  EXPECT_EQ(
      flowletCfg2->getBackupSwitchingMode(),
      cfg::SwitchingMode::PER_PACKET_RANDOM);

  flowletCfg.switchingMode() = cfg::SwitchingMode::FIXED_ASSIGNMENT;
  flowletCfg.backupSwitchingMode() = cfg::SwitchingMode::FIXED_ASSIGNMENT;

  config.flowletSwitchingConfig() = flowletCfg;
  auto stateV4 = publishAndApplyConfig(stateV2, &config, platform.get());
  ASSERT_NE(nullptr, stateV4);
  flowletCfg2 = stateV4->getFlowletSwitchingConfig();
  EXPECT_TRUE(flowletCfg2);
  EXPECT_FALSE(flowletCfg2->isPublished());
  EXPECT_EQ(
      flowletCfg2->getSwitchingMode(), cfg::SwitchingMode::FIXED_ASSIGNMENT);
  EXPECT_EQ(
      flowletCfg2->getBackupSwitchingMode(),
      cfg::SwitchingMode::FIXED_ASSIGNMENT);

  // Ensure that the global and switchSettings field are set for
  // backward/forward compatibility
  EXPECT_EQ(
      stateV2->getFlowletSwitchingConfig(),
      utility::getFirstNodeIf(stateV2->getSwitchSettings())
          ->getFlowletSwitchingConfig());

  // undo flowlet switching cfg
  config.flowletSwitchingConfig().reset();
  auto stateV5 = publishAndApplyConfig(stateV3, &config, platform.get());
  ASSERT_NE(nullptr, stateV4);
  EXPECT_EQ(stateV5->getFlowletSwitchingConfig(), nullptr);
}

TEST(FlowletSwitching, modify) {
  auto platform = createMockPlatform();

  cfg::SwitchConfig config;
  cfg::FlowletSwitchingConfig flowletCfg;

  // make any random change, so that new state can be created
  // ensure that flowlet switching is not configured
  config.flowletSwitchingConfig() = flowletCfg;
  auto stateV0 = std::make_shared<SwitchState>();
  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  ASSERT_NE(stateV1, nullptr);
  stateV1->publish();
  EXPECT_NE(stateV1->getFlowletSwitchingConfig(), nullptr);
  EXPECT_TRUE(stateV1->isPublished());
  EXPECT_TRUE(stateV1->getSwitchSettings()->isPublished());
  EXPECT_TRUE(stateV1->getFlowletSwitchingConfig()->isPublished());
  auto origFlowletConfig = stateV1->getFlowletSwitchingConfig();
  auto newFlowletConfig = origFlowletConfig->modify(&stateV1);
  EXPECT_NE(newFlowletConfig, origFlowletConfig.get());
  EXPECT_FALSE(stateV1->isPublished());
  EXPECT_FALSE(stateV1->getSwitchSettings()->isPublished());
  EXPECT_FALSE(stateV1->getFlowletSwitchingConfig()->isPublished());
}

TEST(FlowletSwitching, mismatchedStateModify) {
  auto platform = createMockPlatform();

  cfg::SwitchConfig config;
  cfg::FlowletSwitchingConfig flowletCfg;

  // make any random change, so that new state can be created
  // ensure that flowlet switching is not configured
  config.flowletSwitchingConfig() = flowletCfg;
  auto stateV0 = std::make_shared<SwitchState>();
  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  ASSERT_NE(stateV1, nullptr);
  stateV1->publish();
  EXPECT_NE(stateV1->getFlowletSwitchingConfig(), nullptr);
  EXPECT_TRUE(stateV1->isPublished());
  EXPECT_TRUE(stateV1->getSwitchSettings()->isPublished());
  EXPECT_TRUE(stateV1->getFlowletSwitchingConfig()->isPublished());
  auto origFlowletConfig = stateV1->getFlowletSwitchingConfig();
  auto newFlowletConfig = origFlowletConfig->modify(&stateV1);
  EXPECT_NE(newFlowletConfig, origFlowletConfig.get());
  EXPECT_FALSE(stateV1->isPublished());
  EXPECT_FALSE(stateV1->getSwitchSettings()->isPublished());
  EXPECT_FALSE(stateV1->getFlowletSwitchingConfig()->isPublished());
  stateV1->publish();
  newFlowletConfig->publish();
  auto stateV2 = stateV1->clone();
  stateV2->publish();
  std::ignore = stateV2->getFlowletSwitchingConfig()->modify(&stateV2);
  // newFlowletConfig is part of stateV1 not stateV2
  EXPECT_THROW(newFlowletConfig->modify(&stateV2), FbossError);
}
