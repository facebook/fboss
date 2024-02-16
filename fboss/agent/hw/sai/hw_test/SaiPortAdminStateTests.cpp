// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/api/PortApi.h"
#include "fboss/agent/hw/sai/api/SystemPortApi.h"
#include "fboss/agent/hw/sai/switch/ConcurrentIndices.h"
#include "fboss/agent/hw/sai/switch/SaiSwitch.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwTest.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/PortMap.h"
#include "fboss/agent/state/SwitchState.h"

using namespace ::facebook::fboss;

class SaiPortAdminStateTest : public HwTest {
 public:
  bool hideFabricPorts() const override {
    return false;
  }
  cfg::SwitchConfig getConfig(cfg::PortState adminState) const {
    auto cfg = utility::onePortPerInterfaceConfig(
        getHwSwitch(),
        masterLogicalPortIds(),
        getAsic()->desiredLoopbackModes(),
        true, /*interfaceHasSubnet*/
        true, /*setInterfaceMac*/
        utility::kBaseVlanId,
        true /*enable fabric ports*/);
    for (auto& port : *cfg.ports()) {
      port.state() = adminState;
    }
    return cfg;
  }
  void assertPortAdminState(
      cfg::PortState expectedAdminState,
      bool configApplied = true) const {
    auto& portApi = SaiApiTable::getInstance()->portApi();
    auto& systemPortApi = SaiApiTable::getInstance()->systemPortApi();
    auto isVoq = getSwitchType() == cfg::SwitchType::VOQ;
    const auto& concurrentIndices =
        static_cast<const SaiSwitch*>(getHwSwitch())->concurrentIndices();
    for (const auto& portMap :
         std::as_const(*getProgrammedState()->getPorts())) {
      for (const auto& [_, port] : std::as_const(*portMap.second)) {
        EXPECT_EQ(port->getAdminState(), expectedAdminState);
        auto portSaiId =
            concurrentIndices.portSaiIds.find(port->getID())->second;
        auto gotAdminState = portApi.getAttribute(
            portSaiId, SaiPortTraits::Attributes::AdminState{});
        auto portEnabled = expectedAdminState == cfg::PortState::ENABLED;
        EXPECT_EQ(gotAdminState, portEnabled);
        // TODO - start looking at mgmt port TYPE here as well
        // once we start creating sys ports for MGMT port
        if (configApplied && isVoq &&
            port->getPortType() == cfg::PortType::INTERFACE_PORT) {
          auto sysPortId = getSystemPortID(
              port->getID(),
              getProgrammedState(),
              SwitchID(*getAsic()->getSwitchId()));
          auto sysPortSaiId =
              concurrentIndices.sysPortSaiIds.find(sysPortId)->second;
          EXPECT_TRUE(systemPortApi.getAttribute(
              sysPortSaiId, SaiSystemPortTraits::Attributes::AdminState{}));
        }
      }
    }
  }
};

TEST_F(SaiPortAdminStateTest, assertAdminState) {
  auto setup = [this]() {
    // Ports should be disabled post init
    assertPortAdminState(cfg::PortState::DISABLED, false /*config applied*/);
    // Apply config with ports disabled, they should stay disabled
    // For VOQ switches, we have a sys port for each NIF port.
    // Sys ports are always kept enabled. There was a bug in
    // SDK where sys port admin state ended up updating
    // NIF port admin state. The following asserts that
    // these are decoupled.
    applyNewConfig(getConfig(cfg::PortState::DISABLED));
  };
  verifyAcrossWarmBoots(
      setup, [this]() { assertPortAdminState(cfg::PortState::DISABLED); });
}
