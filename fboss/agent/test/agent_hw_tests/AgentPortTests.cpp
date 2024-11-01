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

#include "fboss/agent/ApplyThriftConfig.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/test/utils/AsicUtils.h"
#include "fboss/agent/test/utils/ConfigUtils.h"

namespace facebook::fboss {

class AgentPortTest : public AgentHwTest {
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
    return {production_features::ProductionFeature::HW_SWITCH};
  }

  std::vector<utility::PortInfo> getPortInfos(std::vector<PortID> ports) {
    auto switchId = getAgentEnsemble()
                        ->getSw()
                        ->getScopeResolver()
                        ->scope(getAgentEnsemble()->masterLogicalPortIds())
                        .switchId();
    auto client = getAgentEnsemble()->getHwAgentTestClient(switchId);
    std::vector<int32_t> portIds;
    for (auto port : ports) {
      portIds.push_back(port);
    }
    std::vector<utility::PortInfo> portInfos;
    client->sync_getPortInfo(portInfos, std::move(portIds));
    return portInfos;
  }
};

TEST_F(AgentPortTest, PortLoopbackMode) {
  auto setup = [this]() { applyNewConfig(initialConfig(*getAgentEnsemble())); };
  auto verify = [this]() {
    auto asic =
        utility::checkSameAndGetAsic(this->getAgentEnsemble()->getL3Asics());
    for (const auto& loopbackMode : asic->desiredLoopbackModes()) {
      auto portInfos =
          getPortInfos({PortID(masterLogicalPortIds({loopbackMode.first})[0])});
      EXPECT_EQ(
          *portInfos[0].loopbackMode(), static_cast<int>(loopbackMode.second));
    }
  };

  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
