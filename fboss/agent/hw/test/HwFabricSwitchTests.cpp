// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwLinkStateDependentTest.h"
#include "fboss/agent/hw/test/HwTest.h"
#include "fboss/agent/test/utils/FabricTestUtils.h"
#include "fboss/lib/CommonUtils.h"

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
    utility::populatePortExpectedNeighbors(masterLogicalPortIds(), cfg);
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
  auto setup = [=, this]() {
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
    utility::checkFabricReachabilityStats(getHwSwitchEnsemble(), SwitchID(0));
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwFabricSwitchTest, collectStats) {
  auto verify = [this]() {
    EXPECT_GT(getProgrammedState()->getPorts()->numNodes(), 0);
    getHwSwitch()->updateStats();
  };
  verifyAcrossWarmBoots([] {}, verify);
}

TEST_F(HwFabricSwitchTest, checkFabricReachability) {
  auto verify = [this]() {
    EXPECT_GT(getProgrammedState()->getPorts()->numNodes(), 0);
    utility::checkFabricReachability(getHwSwitchEnsemble(), SwitchID(0));
  };
  verifyAcrossWarmBoots([] {}, verify);
}

TEST_F(HwFabricSwitchTest, fabricIsolate) {
  auto setup = [=, this]() {
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

  auto verify = [=, this]() {
    EXPECT_GT(getProgrammedState()->getPorts()->numNodes(), 0);
    auto fabricPortId =
        PortID(masterLogicalPortIds({cfg::PortType::FABRIC_PORT})[0]);
    utility::checkPortFabricReachability(
        getHwSwitchEnsemble(), SwitchID(0), fabricPortId);
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwFabricSwitchTest, fabricSwitchIsolate) {
  auto setup = [=, this]() {
    setSwitchDrainState(initialConfig(), cfg::SwitchDrainState::DRAINED);
  };

  auto verify = [=, this]() {
    EXPECT_GT(getProgrammedState()->getPorts()->numNodes(), 0);
    utility::checkFabricReachability(getHwSwitchEnsemble(), SwitchID(0));
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwFabricSwitchTest, reachDiscard) {
  auto verify = [this]() {
    getHwSwitch()->updateStats();
    auto beforeSwitchDrops = getHwSwitch()->getSwitchDropStats();
    std::string out;
    getHwSwitchEnsemble()->runDiagCommand(
        "TX 1 DeSTination=-1 DeSTinationModid=-1 flags=0x8000\n", out);
    getHwSwitchEnsemble()->runDiagCommand("quit\n", out);
    WITH_RETRIES({
      getHwSwitch()->updateStats();
      auto afterSwitchDrops = getHwSwitch()->getSwitchDropStats();
      XLOG(INFO) << " Before reach drops: "
                 << *beforeSwitchDrops.globalReachabilityDrops()
                 << " After reach drops: "
                 << *afterSwitchDrops.globalReachabilityDrops()
                 << " Before global drops: " << *beforeSwitchDrops.globalDrops()
                 << " After global drops: : "
                 << *afterSwitchDrops.globalDrops();
      EXPECT_EVENTUALLY_EQ(
          *afterSwitchDrops.globalReachabilityDrops(),
          *beforeSwitchDrops.globalReachabilityDrops() + 1);
      // Global drops are in bytes
      EXPECT_EVENTUALLY_GT(
          *afterSwitchDrops.globalDrops(), *beforeSwitchDrops.globalDrops());
    });
    checkNoStatsChange();
  };
  verifyAcrossWarmBoots([]() {}, verify);
}

} // namespace facebook::fboss
