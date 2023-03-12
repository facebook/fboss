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
        getAsic()->desiredLoopbackMode(),
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
    for (auto& port : std::as_const(*state->getPorts())) {
      EXPECT_EQ(port.second->getAdminState(), cfg::PortState::ENABLED);
      EXPECT_EQ(
          port.second->getLoopbackMode(), getAsic()->desiredLoopbackMode());
    }
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwFabricSwitchTest, collectStats) {
  auto verify = [this]() {
    EXPECT_GT(getProgrammedState()->getPorts()->size(), 0);
    SwitchStats dummy;
    getHwSwitch()->updateStats(&dummy);
  };
  verifyAcrossWarmBoots([] {}, verify);
}

TEST_F(HwFabricSwitchTest, checkFabricReachability) {
  auto verify = [this]() {
    EXPECT_GT(getProgrammedState()->getPorts()->size(), 0);
    SwitchStats dummy;
    getHwSwitch()->updateStats(&dummy);
    checkFabricReachability(getHwSwitch());
  };
  verifyAcrossWarmBoots([] {}, verify);
}

TEST_F(HwFabricSwitchTest, fabricIsolate) {
  auto setup = [=]() { applyNewConfig(initialConfig()); };

  auto verify = [=]() {
    EXPECT_GT(getProgrammedState()->getPorts()->size(), 0);
    SwitchStats dummy;
    getHwSwitch()->updateStats(&dummy);
    auto fabricPortId =
        PortID(masterLogicalPortIds({cfg::PortType::FABRIC_PORT})[0]);
    checkPortFabricReachability(getHwSwitch(), fabricPortId);
    auto newState = getProgrammedState();
    auto port = newState->getPorts()->getPort(fabricPortId);
    auto newPort = port->modify(&newState);
    newPort->setPortDrainState(cfg::PortDrainState::DRAINED);
    applyNewState(newState);
    getHwSwitch()->updateStats(&dummy);
    checkPortFabricReachability(getHwSwitch(), fabricPortId);
  };
  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
