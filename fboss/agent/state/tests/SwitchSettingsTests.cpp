/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/mock/MockPlatform.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/TestUtils.h"

using namespace facebook::fboss;
using std::make_shared;

TEST(SwitchSettingsTest, applyL2LearningConfig) {
  auto platform = createMockPlatform();
  auto stateV0 = make_shared<SwitchState>();

  cfg::SwitchConfig config;
  *config.switchSettings_ref()->l2LearningMode_ref() =
      cfg::L2LearningMode::SOFTWARE;
  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  EXPECT_NE(nullptr, stateV1);

  auto switchSettingsV1 = stateV1->getSwitchSettings();
  ASSERT_NE(nullptr, switchSettingsV1);
  EXPECT_FALSE(switchSettingsV1->isPublished());
  EXPECT_EQ(
      cfg::L2LearningMode::SOFTWARE, switchSettingsV1->getL2LearningMode());
  EXPECT_FALSE(switchSettingsV1->isQcmEnable());

  *config.switchSettings_ref()->l2LearningMode_ref() =
      cfg::L2LearningMode::HARDWARE;

  auto stateV2 = publishAndApplyConfig(stateV1, &config, platform.get());
  EXPECT_NE(nullptr, stateV2);

  auto switchSettingsV2 = stateV2->getSwitchSettings();
  ASSERT_NE(nullptr, switchSettingsV2);
  EXPECT_FALSE(switchSettingsV2->isPublished());
  EXPECT_EQ(
      cfg::L2LearningMode::HARDWARE, switchSettingsV2->getL2LearningMode());
  EXPECT_FALSE(switchSettingsV2->isQcmEnable());
}

TEST(SwitchSettingsTest, applyQcmConfig) {
  auto platform = createMockPlatform();
  auto stateV0 = make_shared<SwitchState>();

  cfg::SwitchConfig config;
  *config.switchSettings_ref()->qcmEnable_ref() = true;
  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  EXPECT_NE(nullptr, stateV1);
  auto switchSettingsV1 = stateV1->getSwitchSettings();
  ASSERT_NE(nullptr, switchSettingsV1);
  EXPECT_FALSE(switchSettingsV1->isPublished());
  EXPECT_TRUE(switchSettingsV1->isQcmEnable());
  EXPECT_EQ(
      cfg::L2LearningMode::HARDWARE, switchSettingsV1->getL2LearningMode());

  *config.switchSettings_ref()->qcmEnable_ref() = false;
  auto stateV2 = publishAndApplyConfig(stateV1, &config, platform.get());
  EXPECT_NE(nullptr, stateV2);
  auto switchSettingsV2 = stateV2->getSwitchSettings();
  ASSERT_NE(nullptr, switchSettingsV2);
  EXPECT_FALSE(switchSettingsV2->isPublished());
  EXPECT_FALSE(switchSettingsV2->isQcmEnable());
  EXPECT_EQ(
      cfg::L2LearningMode::HARDWARE, switchSettingsV2->getL2LearningMode());
}

TEST(SwitchSettingsTest, ToFromJSON) {
  std::string jsonStr = R"(
        {
          "l2LearningMode": 1,
          "qcmEnable": true
        }
  )";

  auto switchSettings =
      SwitchSettings::fromFollyDynamic(folly::parseJson(jsonStr));
  EXPECT_EQ(cfg::L2LearningMode::SOFTWARE, switchSettings->getL2LearningMode());
  EXPECT_TRUE(switchSettings->isQcmEnable());

  auto dyn1 = switchSettings->toFollyDynamic();
  auto dyn2 = folly::parseJson(jsonStr);

  EXPECT_EQ(dyn1, dyn2);
}
