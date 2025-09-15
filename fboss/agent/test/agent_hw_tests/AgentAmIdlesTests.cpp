// Copyright 2004-present Facebook. All Rights Reserved.

/**
 * @file AgentAmIdlesTests.cpp
 * @brief Tests for AM idles configuration using SAI.
 * @author Ashutosh Grewal (agrewal@meta.com)
 */

#include "fboss/agent/test/AgentHwTest.h"

#include "fboss/agent/test/utils/ConfigUtils.h"

namespace facebook::fboss {

class AgentAmIdlesTest : public AgentHwTest {
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
    return {ProductionFeature::AM_IDLES};
  }
};

/**
 * @brief Test AM idles configuration and warmboot persistence
 *
 * Test AM idles configuration using SAI.
 */
TEST_F(AgentAmIdlesTest, VerifyAmIdles) {
  static constexpr auto kAmIdlesEnabled = true;
  static constexpr auto kAmIdlesDisabled = false;

  auto setup = [=, this]() {
    auto newCfg{initialConfig(*(this->getAgentEnsemble()))};

    // Get the first two ports for testing
    auto masterPorts = masterLogicalPortIds();
    EXPECT_GE(masterPorts.size(), 2) << "Need at least 2 ports for this test";

    const auto& firstPortId = masterPorts[0];
    const auto& secondPortId = masterPorts[1];

    // Configure AM idles on ports
    for (auto& portCfg : *newCfg.ports()) {
      if (PortID(*portCfg.logicalID()) == firstPortId) {
        XLOG(INFO) << "Setting AM idles ENABLED for first port: "
                   << firstPortId;
        portCfg.amIdles() = kAmIdlesEnabled;
      } else if (PortID(*portCfg.logicalID()) == secondPortId) {
        XLOG(INFO) << "Setting AM idles DISABLED for second port: "
                   << secondPortId;
        portCfg.amIdles() = kAmIdlesDisabled;
      }
    }

    applyNewConfig(newCfg);
  };

  auto verify = [=, this]() {
    auto masterPorts = masterLogicalPortIds();

    // Verify first port has AM idles enabled
    auto firstPort =
        getProgrammedState()->getPorts()->getNodeIf(masterPorts[0]);
    EXPECT_TRUE(firstPort->getAmIdles().has_value());
    auto firstPortValue = firstPort->getAmIdles().value();
    XLOG(INFO) << "Retrieved AM idles value for first port: " << firstPortValue;
    EXPECT_EQ(firstPortValue, kAmIdlesEnabled);

    // Verify second port has AM idles disabled
    auto secondPort =
        getProgrammedState()->getPorts()->getNodeIf(masterPorts[1]);
    EXPECT_TRUE(secondPort->getAmIdles().has_value());
    auto secondPortValue = secondPort->getAmIdles().value();
    XLOG(INFO) << "Retrieved AM idles value for second port: "
               << secondPortValue;
    EXPECT_EQ(secondPortValue, kAmIdlesDisabled);
  };

  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
