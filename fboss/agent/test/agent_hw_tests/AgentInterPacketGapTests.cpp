// Copyright 2004-present Facebook. All Rights Reserved.

/**
 * @file AgentInterPacketGapTests.cpp
 * @brief Tests for inter-packet gap using SAI.
 * @author Ashutosh Grewal (agrewal@meta.com)
 */

#include "fboss/agent/test/AgentHwTest.h"

#include "fboss/agent/test/utils/ConfigUtils.h"

namespace facebook::fboss {

class AgentInterPacketGapTest : public AgentHwTest {
 protected:
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    return utility::onePortPerInterfaceConfig(
        ensemble.getSw(),
        ensemble.masterLogicalPortIds(),
        true /*interfaceHasSubnet*/);
  }

  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    return {ProductionFeature::INTER_PACKET_GAP};
  }
};

/**
 * @brief Test inter-packet gap configuration and warmboot persistence
 *
 * Test inter-packet gap using SAI.
 */
TEST_F(AgentInterPacketGapTest, VerifyInterPacketGap) {
  static constexpr auto kInterPacketGapBits = 256;

  auto setup = [=, this]() {
    auto newCfg{initialConfig(*(this->getAgentEnsemble()))};

    // Configure inter-packet gap on the first port
    auto targetPortId = masterLogicalPortIds()[0];
    for (auto& portCfg : *newCfg.ports()) {
      if (PortID(*portCfg.logicalID()) == targetPortId) {
        portCfg.interPacketGapBits() = kInterPacketGapBits;
        break;
      }
    }

    applyNewConfig(newCfg);
  };

  auto verify = [=, this]() {
    // Verify inter-packet gap is configured correctly in SW state
    auto port =
        getProgrammedState()->getPorts()->getNodeIf(masterLogicalPortIds()[0]);
    EXPECT_TRUE(port->getInterPacketGapBits().has_value());

    auto retrievedValue = port->getInterPacketGapBits().value();
    XLOG(INFO) << "Retrieved inter-packet gap bits: " << retrievedValue;

    EXPECT_EQ(retrievedValue, kInterPacketGapBits);
  };

  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
