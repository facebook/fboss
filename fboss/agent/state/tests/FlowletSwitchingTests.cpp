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

  flowletCfg.inactivityIntervalUsecs() = 60;
  flowletCfg.flowletTableSize() = 1024;
  flowletCfg.dynamicEgressLoadExponent() = 3;
  flowletCfg.dynamicQueueExponent() = 3;
  flowletCfg.dynamicQueueMinThresholdBytes() = 0x5D44;
  flowletCfg.dynamicQueueMaxThresholdBytes() = 0xFE00;
  flowletCfg.dynamicSampleRate() = 62500;
  flowletCfg.portScalingFactor() = 400;
  flowletCfg.portLoadWeight() = 50;
  flowletCfg.portQueueWeight() = 40;

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
  EXPECT_EQ(flowletCfg2->getPortScalingFactor(), 400);
  EXPECT_EQ(flowletCfg2->getPortLoadWeight(), 50);
  EXPECT_EQ(flowletCfg2->getPortQueueWeight(), 40);
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
  flowletCfg.portScalingFactor() = 400;
  flowletCfg.portLoadWeight() = 50;
  flowletCfg.portQueueWeight() = 40;

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

  // change config
  flowletCfg.inactivityIntervalUsecs() = 60;
  flowletCfg.flowletTableSize() = 1024;
  flowletCfg.dynamicEgressLoadExponent() = 3;
  flowletCfg.dynamicQueueExponent() = 3;
  flowletCfg.dynamicQueueMinThresholdBytes() = 0x5D44;
  flowletCfg.dynamicQueueMaxThresholdBytes() = 0xFE00;
  flowletCfg.dynamicSampleRate() = 62500;
  flowletCfg.portScalingFactor() = 400;
  flowletCfg.portLoadWeight() = 50;
  flowletCfg.portQueueWeight() = 40;

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
  EXPECT_EQ(flowletCfg2->getPortScalingFactor(), 400);
  EXPECT_EQ(flowletCfg2->getPortLoadWeight(), 50);
  EXPECT_EQ(flowletCfg2->getPortQueueWeight(), 40);

  // Ensure that the global and switchSettings field are set for
  // backward/forward compatibility
  EXPECT_EQ(
      stateV2->getFlowletSwitchingConfig(),
      util::getFirstNodeIf(stateV2->getSwitchSettings())
          ->getFlowletSwitchingConfig());

  // undo flowlet switching cfg
  config.flowletSwitchingConfig().reset();
  auto stateV4 = publishAndApplyConfig(stateV3, &config, platform.get());
  ASSERT_NE(nullptr, stateV4);
  EXPECT_EQ(stateV4->getFlowletSwitchingConfig(), nullptr);
}
