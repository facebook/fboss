// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/switch/ConcurrentIndices.h"
#include "fboss/agent/hw/sai/switch/SaiSwitch.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwTest.h"

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
  void assertPortAdminState(cfg::PortState expectedAdminState) const {
    auto& portApi = SaiApiTable::getInstance()->portApi();
    for (const auto& portMap :
         std::as_const(*getProgrammedState()->getPorts())) {
      for (const auto& [_, port] : std::as_const(*portMap.second)) {
        EXPECT_EQ(port->getAdminState(), expectedAdminState);
        auto portSaiId = static_cast<const SaiSwitch*>(getHwSwitch())
                             ->concurrentIndices()
                             .portSaiIds.find(port->getID())
                             ->second;
        auto gotAdminState = portApi.getAttribute(
            portSaiId, SaiPortTraits::Attributes::AdminState{});
        auto portEnabled = expectedAdminState == cfg::PortState::ENABLED;
        EXPECT_EQ(gotAdminState, portEnabled);
      }
    }
  }
};

TEST_F(SaiPortAdminStateTest, assertAdminState) {
  auto setup = [this]() {
    // Ports should be disabled post init
    assertPortAdminState(cfg::PortState::DISABLED);
    // Apply config with ports disabled, they should stay disabled
    // For VOQ switches, we have a sys port for each NIF port.
    // Sys ports are always kept enabled. There was a bug in
    // SDK where sys port admin state ended up updating
    // NIF port admin state. The following asserts that
    // these are decoupled.
    applyNewConfig(getConfig(cfg::PortState::DISABLED));
    assertPortAdminState(cfg::PortState::DISABLED);
  };
  verifyAcrossWarmBoots(
      setup, [this]() { assertPortAdminState(cfg::PortState::DISABLED); });
}
