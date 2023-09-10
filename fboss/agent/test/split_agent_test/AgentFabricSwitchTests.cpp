// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <algorithm>
#include "fboss/agent/HwAsicTable.h"
#include "fboss/agent/hw/HwSwitchFb303Stats.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwTestFabricUtils.h"
#include "fboss/agent/test/SplitAgentTest.h"
#include "fboss/lib/CommonUtils.h"

DECLARE_bool(enable_stats_update_thread);

namespace facebook::fboss {
class AgentFabricSwitchTest : public SplitAgentTest {
 public:
  cfg::SwitchConfig initialConfig(
      SwSwitch* swSwitch,
      const std::vector<PortID>& ports) const override {
    // Disable sw stats update thread
    FLAGS_enable_stats_update_thread = false;
    // Before m-mpu agent test, use first Asic for initialization.
    auto switchIds = swSwitch->getHwAsicTable()->getSwitchIDs();
    CHECK_GE(switchIds.size(), 1);
    auto asic = swSwitch->getHwAsicTable()->getHwAsic(*switchIds.cbegin());
    auto config = utility::onePortPerInterfaceConfig(
        swSwitch->getPlatformMapping(),
        asic,
        ports,
        asic->desiredLoopbackModes(),
        false /*interfaceHasSubnet*/,
        false /*setInterfaceMac*/,
        utility::kBaseVlanId,
        true /*enable fabric ports*/);
    populatePortExpectedNeighbors(ports, config);
    return config;
  }

  void SetUp() override {
    SplitAgentTest::SetUp();
    ASSERT_TRUE(
        std::any_of(getAsics().begin(), getAsics().end(), [](auto& iter) {
          return iter.second->getSwitchType() == cfg::SwitchType::FABRIC;
        }));
  }

  void checkFabricReachabilityStats(SwSwitch* sw) {
    sw->updateStats();
    auto reachability = sw->getHwSwitchHandler()->getFabricReachability();
    int count = 0;
    for (auto [_, endpoint] : reachability) {
      if (!*endpoint.isAttached()) {
        continue;
      }
      // all interfaces which have reachability info collected
      count++;
    }
    // expected all of interfaces to jump on mismatched and missing
    WITH_RETRIES(EXPECT_EVENTUALLY_EQ(
        *(sw->getHwSwitchHandler()
              ->getFabricReachabilityStats()
              .mismatchCount()),
        count));
    WITH_RETRIES(EXPECT_EVENTUALLY_EQ(
        *(sw->getHwSwitchHandler()
              ->getFabricReachabilityStats()
              .missingCount()),
        count));
  }

 private:
  bool hideFabricPorts() const override {
    return false;
  }
};

TEST_F(AgentFabricSwitchTest, init) {
  auto setup = []() {};
  auto verify = [this]() {
    auto state = getProgrammedState();
    for (auto& portMap : std::as_const(*state->getPorts())) {
      for (auto& port : std::as_const(*portMap.second)) {
        EXPECT_EQ(port.second->getAdminState(), cfg::PortState::ENABLED);
        EXPECT_EQ(
            port.second->getLoopbackMode(),
            // TODO: Handle multiple Asics
            getAsics().cbegin()->second->getDesiredLoopbackMode(
                port.second->getPortType()));
      }
    }
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentFabricSwitchTest, checkFabricReachabilityStats) {
  auto setup = [=]() {
    auto newCfg =
        initialConfig(getAgentEnsemble()->getSw(), masterLogicalPortIds());
    // reset the neighbor reachability information
    for (const auto& portID : masterLogicalPortIds()) {
      auto portCfg = utility::findCfgPort(newCfg, portID);
      if (portCfg->portType() == cfg::PortType::FABRIC_PORT) {
        portCfg->expectedNeighborReachability() = {};
      }
    }
    applyNewConfig(newCfg);
  };
  auto verify = [this]() {
    EXPECT_GT(getProgrammedState()->getPorts()->numNodes(), 0);
    checkFabricReachabilityStats(getAgentEnsemble()->getSw());
  };
  verifyAcrossWarmBoots(setup, verify);
}
} // namespace facebook::fboss
