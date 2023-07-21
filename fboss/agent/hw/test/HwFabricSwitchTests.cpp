// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwLinkStateDependentTest.h"
#include "fboss/agent/hw/test/HwTest.h"
#include "fboss/agent/hw/test/HwTestFabricUtils.h"

namespace facebook::fboss {

class HwFabricSwitchTest : public HwLinkStateDependentTest {
 public:
  cfg::SwitchConfig initialConfig() const override {
    auto cfg = utility::onePortPerInterfaceConfig(
        getHwSwitch(),
        masterLogicalPortIds(),
        getAsic()->desiredLoopbackModes(),
        false /*interfaceHasSubnet*/,
        false, /*setInterfaceMac*/
        utility::kBaseVlanId,
        true /*enable fabric ports*/
    );
    populatePortExpectedNeighbors(masterLogicalPortIds(), cfg);
    return cfg;
  }
  void SetUp() override {
    HwLinkStateDependentTest::SetUp();
    ASSERT_EQ(getHwSwitch()->getSwitchType(), cfg::SwitchType::FABRIC);
  }

 private:
  bool hideFabricPorts() const override {
    return false;
  }
};

TEST_F(HwFabricSwitchTest, init) {
  auto setup = [this]() {};
  auto verify = [this]() {
    auto state = getProgrammedState();
    for (auto& portMap : std::as_const(*state->getPorts())) {
      for (auto& port : std::as_const(*portMap.second)) {
        EXPECT_EQ(port.second->getAdminState(), cfg::PortState::ENABLED);
        EXPECT_EQ(
            port.second->getLoopbackMode(),
            getAsic()->getDesiredLoopbackMode(port.second->getPortType()));
      }
    }
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwFabricSwitchTest, checkFabricReachabilityStats) {
  auto setup = [=]() {
    auto newCfg = initialConfig();
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
    checkFabricReachabilityStats(getHwSwitch());
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwFabricSwitchTest, collectStats) {
  auto verify = [this]() {
    EXPECT_GT(getProgrammedState()->getPorts()->numNodes(), 0);
    SwitchStats dummy;
    getHwSwitch()->updateStats(&dummy);
  };
  verifyAcrossWarmBoots([] {}, verify);
}

TEST_F(HwFabricSwitchTest, checkFabricReachability) {
  auto verify = [this]() {
    EXPECT_GT(getProgrammedState()->getPorts()->numNodes(), 0);
    checkFabricReachability(getHwSwitch());
  };
  verifyAcrossWarmBoots([] {}, verify);
}

TEST_F(HwFabricSwitchTest, fabricIsolate) {
  auto setup = [=]() {
    auto newCfg = initialConfig();
    auto fabricPortId =
        PortID(masterLogicalPortIds({cfg::PortType::FABRIC_PORT})[0]);
    for (auto& portCfg : *newCfg.ports()) {
      if (PortID(*portCfg.logicalID()) == fabricPortId) {
        *portCfg.drainState() = cfg::PortDrainState::DRAINED;
        break;
      }
    }
    applyNewConfig(newCfg);
  };

  auto verify = [=]() {
    EXPECT_GT(getProgrammedState()->getPorts()->numNodes(), 0);
    auto fabricPortId =
        PortID(masterLogicalPortIds({cfg::PortType::FABRIC_PORT})[0]);
    checkPortFabricReachability(getHwSwitch(), fabricPortId);
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwFabricSwitchTest, fabricSwitchIsolate) {
  auto setup = [=]() {
    setSwitchDrainState(initialConfig(), cfg::SwitchDrainState::DRAINED);
  };

  auto verify = [=]() {
    EXPECT_GT(getProgrammedState()->getPorts()->numNodes(), 0);
    checkFabricReachability(getHwSwitch());
  };
  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
