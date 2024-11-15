// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/test/AgentHwTest.h"

#include "fboss/agent/test/utils/AsicUtils.h"
#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/lib/CommonUtils.h"

namespace facebook::fboss {

class AgentPortLedTest : public AgentHwTest {
 protected:
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    return utility::onePortPerInterfaceConfig(
        ensemble.getSw(),
        ensemble.masterLogicalPortIds(),
        true /*interfaceHasSubnet*/);
  }

  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    return {production_features::ProductionFeature::LED_PROGRAMMING};
  }

  bool verifyLedStatus(PortID portID, bool up) {
    auto client = getAgentEnsemble()->getHwAgentTestClient(
        getSw()->getScopeResolver()->scope(portID).switchId());
    return client->sync_verifyPortLedStatus(portID, up);
  }
};

TEST_F(AgentPortLedTest, TestLed) {
  auto setup = []() {};
  auto verify = [=, this]() {
    auto portID = masterLogicalPortIds()[0];
    bringUpPort(portID);
    WITH_RETRIES({ EXPECT_EVENTUALLY_TRUE(verifyLedStatus(portID, true)); });
    bringDownPort(portID);
    WITH_RETRIES({ EXPECT_EVENTUALLY_TRUE(verifyLedStatus(portID, false)); });
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentPortLedTest, TestLedFromSwitchState) {
  auto portID = masterLogicalPortIds()[0];
  auto setup = [&]() {
    bringUpPort(portID);
    WITH_RETRIES({ EXPECT_EVENTUALLY_TRUE(verifyLedStatus(portID, true)); });
    applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      auto newState = in->clone();
      auto port = newState->getPorts()->getNode(portID);
      auto newPort = port->modify(&newState);
      newPort->setLedPortExternalState(
          PortLedExternalState::EXTERNAL_FORCE_OFF);
      return newState;
    });
  };
  auto verify = [&]() {
    WITH_RETRIES({ EXPECT_EVENTUALLY_TRUE(verifyLedStatus(portID, false)); });
  };
  verifyAcrossWarmBoots(setup, verify);
}
} // namespace facebook::fboss
