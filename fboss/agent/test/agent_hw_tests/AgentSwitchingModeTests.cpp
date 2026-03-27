/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/test/AgentHwTest.h"

#include "fboss/agent/gen-cpp2/switch_config_types.h"

namespace facebook::fboss {

class AgentSwitchingModeTest : public AgentHwTest {
 protected:
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    return AgentHwTest::initialConfig(ensemble);
  }

  std::vector<test::production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    return {test::production_features::ProductionFeature::CUT_THROUGH};
  }

  void setPacketForwardingMode(cfg::PacketForwardingMode mode) {
    auto config = getAgentEnsemble()->getCurrentConfig();
    config.switchSettings()->packetForwardingMode() = mode;
    applyNewConfig(config);
  }
};

TEST_F(AgentSwitchingModeTest, VerifyCutThroughMode) {
  auto setup = [&]() {
    setPacketForwardingMode(cfg::PacketForwardingMode::CUT_THROUGH);
  };

  auto verify = [&]() {
    auto config = getAgentEnsemble()->getCurrentConfig();
    EXPECT_TRUE(config.switchSettings()->packetForwardingMode().has_value());
    EXPECT_EQ(
        cfg::PacketForwardingMode::CUT_THROUGH,
        *config.switchSettings()->packetForwardingMode());
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentSwitchingModeTest, VerifyStoreAndForwardMode) {
  auto setup = [&]() {
    setPacketForwardingMode(cfg::PacketForwardingMode::STORE_AND_FORWARD);
  };

  auto verify = [&]() {
    auto config = getAgentEnsemble()->getCurrentConfig();
    EXPECT_TRUE(config.switchSettings()->packetForwardingMode().has_value());
    EXPECT_EQ(
        cfg::PacketForwardingMode::STORE_AND_FORWARD,
        *config.switchSettings()->packetForwardingMode());
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentSwitchingModeTest, VerifySwitchingModeToggle) {
  constexpr int kLoopCount = 5;
  for (int i = 0; i < kLoopCount; i++) {
    setPacketForwardingMode(cfg::PacketForwardingMode::CUT_THROUGH);
    auto config = getAgentEnsemble()->getCurrentConfig();
    EXPECT_EQ(
        cfg::PacketForwardingMode::CUT_THROUGH,
        *config.switchSettings()->packetForwardingMode());

    setPacketForwardingMode(cfg::PacketForwardingMode::STORE_AND_FORWARD);
    config = getAgentEnsemble()->getCurrentConfig();
    EXPECT_EQ(
        cfg::PacketForwardingMode::STORE_AND_FORWARD,
        *config.switchSettings()->packetForwardingMode());
  }
}

} // namespace facebook::fboss
