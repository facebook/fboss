// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwTest.h"

namespace facebook::fboss {

class HwVoqSwitchTest : public HwTest {
 public:
  void SetUp() override {
    HwTest::SetUp();
    ASSERT_EQ(getHwSwitch()->getSwitchType(), cfg::SwitchType::VOQ);
    ASSERT_TRUE(getHwSwitch()->getSwitchId().has_value());
  }
};

TEST_F(HwVoqSwitchTest, init) {
  auto setup = [this]() {
    auto newState = getProgrammedState()->clone();
    for (auto& port : *newState->getPorts()) {
      auto newPort = port->modify(&newState);
      newPort->setAdminState(cfg::PortState::ENABLED);
    }
    applyNewState(newState);
  };
  auto verify = [this]() {
    auto state = getProgrammedState();
    for (auto& port : *state->getPorts()) {
      EXPECT_EQ(port->getAdminState(), cfg::PortState::ENABLED);
    }
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwVoqSwitchTest, applyConfig) {
  auto setup = [this]() {
    auto config = utility::onePortPerInterfaceConfig(
        getHwSwitch(),
        masterLogicalPortIds(),
        getAsic()->desiredLoopbackMode());
    applyNewConfig(config);
  };
  auto verify = [this]() {
    auto state = getProgrammedState();
    for (auto& port : *state->getPorts()) {
      EXPECT_EQ(port->getAdminState(), cfg::PortState::ENABLED);
    }
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwVoqSwitchTest, remoteSystemPort) {
  auto setup = [this]() {
    auto config = utility::onePortPerInterfaceConfig(
        getHwSwitch(),
        masterLogicalPortIds(),
        getAsic()->desiredLoopbackMode());
    applyNewConfig(config);
    auto newState = getProgrammedState();
    auto systemPorts = newState->getSystemPorts()->modify(&newState);
    auto numLocalPorts = systemPorts->size();
    EXPECT_GT(numLocalPorts, 0);
    auto localPort = *systemPorts->begin();
    auto remoteSysPort = std::make_shared<SystemPort>(SystemPortID(301));
    remoteSysPort->setSwitchId(SwitchID(2));
    remoteSysPort->setNumVoqs(localPort->getNumVoqs());
    remoteSysPort->setCoreIndex(localPort->getCoreIndex());
    remoteSysPort->setCorePortIndex(localPort->getCorePortIndex());

    remoteSysPort->setSpeedMbps(localPort->getSpeedMbps());
    remoteSysPort->setEnabled(true);
    systemPorts->addSystemPort(remoteSysPort);
    applyNewState(newState);
    EXPECT_EQ(
        getProgrammedState()->getSystemPorts()->size(), numLocalPorts + 1);
  };
  verifyAcrossWarmBoots(setup, [] {});
}

TEST_F(HwVoqSwitchTest, addRemoveNeighbor) {
  auto addRemoveNeighbor = [this](
                               const std::shared_ptr<SwitchState>& inState,
                               bool add) {
    const VlanID kVlanID{utility::kBaseVlanId};
    folly::IPAddressV6 neighborIp{"1::2"};
    auto ip = neighborIp;
    auto outState{inState->clone()};
    auto neighborTable =
        outState->getVlans()->getVlan(kVlanID)->getNdpTable()->modify(
            kVlanID, &outState);
    if (add) {
      const folly::MacAddress kNeighborMac{"2:3:4:5:6:7"};
      const InterfaceID kIntfID(101);
      neighborTable->addPendingEntry(ip, InterfaceID(101));
      neighborTable->updateEntry(
          ip, kNeighborMac, PortDescriptor(masterLogicalPortIds()[0]), kIntfID);
    } else {
      neighborTable->removeEntry(ip);
    }
    return outState;
  };

  auto setup = [this, addRemoveNeighbor]() {
    auto config = utility::onePortPerInterfaceConfig(
        getHwSwitch(),
        masterLogicalPortIds(),
        getAsic()->desiredLoopbackMode());
    applyNewConfig(config);
    // Add neighbor
    applyNewState(addRemoveNeighbor(getProgrammedState(), true));
    // Remove neighbor
    applyNewState(addRemoveNeighbor(getProgrammedState(), false));
  };
  verifyAcrossWarmBoots(setup, [] {});
}
} // namespace facebook::fboss
