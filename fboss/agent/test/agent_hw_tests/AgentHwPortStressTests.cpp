/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/utils/FabricTestUtils.h"

#include <string>

using std::make_shared;
using std::shared_ptr;
using std::string;

namespace facebook::fboss {

class AgentHwPortStressTest : public AgentHwTest {
 protected:
  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    return {};
  }

  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto config = AgentHwTest::initialConfig(ensemble);
    if (!config.dsfNodes()->empty()) {
      utility::populatePortExpectedNeighborsToSelf(
          ensemble.masterLogicalPortIds(), config);
    }
    return config;
  }

  void setCmdLineFlagOverrides() const override {
    AgentHwTest::setCmdLineFlagOverrides();
    FLAGS_hide_fabric_ports = false;
  }
};

TEST_F(AgentHwPortStressTest, adminStateToggle) {
  const AgentEnsemble* ensemble = getAgentEnsemble();
  auto setup = [=, this]() { applyNewConfig(initialConfig(*ensemble)); };

  auto verify = [=, this]() {
    // Use 2nd port as on some platforms, first port is rcy port
    // which will never flap in practice
    auto portId = PortID(masterLogicalPortIds()[1]);
    for (auto i = 0; i < 500; ++i) {
      this->applyNewState([&](const std::shared_ptr<SwitchState>&) {
        auto newState = ensemble->getProgrammedState();
        auto port = newState->getPorts()->getNodeIf(portId);
        auto newPort = port->modify(&newState);
        auto newAdminState = newPort->isEnabled() ? cfg::PortState::DISABLED
                                                  : cfg::PortState::ENABLED;
        newPort->setAdminState(newAdminState);
        return newState;
      });
    }
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentHwPortStressTest, linkStateToggle) {
  AgentEnsemble* ensemble = getAgentEnsemble();
  auto setup = [=, this]() { applyNewConfig(initialConfig(*ensemble)); };

  auto verify = [=, this]() {
    // Use 2nd port as on some platforms, first port is rcy port
    // which will never flap in practice
    auto portId = PortID(masterLogicalPortIds()[1]);
    for (auto i = 0; i < 200; ++i) {
      ensemble->getLinkToggler()->bringDownPorts({portId});
      ensemble->getLinkToggler()->bringUpPorts({portId});
    }
  };
  verifyAcrossWarmBoots(setup, verify);
}
} // namespace facebook::fboss
