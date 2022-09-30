// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwTest.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/InterfaceMap.h"
#include "fboss/agent/state/SwitchState.h"

namespace facebook::fboss {
namespace {
const SwitchID kRemoteSwitchId(2);
}
class HwVoqSwitchTest : public HwTest {
 public:
  void SetUp() override {
    HwTest::SetUp();
    ASSERT_EQ(getHwSwitch()->getSwitchType(), cfg::SwitchType::VOQ);
    ASSERT_TRUE(getHwSwitch()->getSwitchId().has_value());
  }

 protected:
  void addRemoteSysPort(SystemPortID portId) {
    auto newState = getProgrammedState()->clone();
    auto systemPorts = newState->getSystemPorts()->modify(&newState);
    auto numPrevPorts = systemPorts->size();
    EXPECT_GT(numPrevPorts, 0);
    auto localPort = *systemPorts->begin();
    auto remoteSysPort = std::make_shared<SystemPort>(portId);
    remoteSysPort->setSwitchId(kRemoteSwitchId);
    remoteSysPort->setNumVoqs(localPort->getNumVoqs());
    remoteSysPort->setCoreIndex(localPort->getCoreIndex());
    remoteSysPort->setCorePortIndex(localPort->getCorePortIndex());
    remoteSysPort->setSpeedMbps(localPort->getSpeedMbps());
    remoteSysPort->setEnabled(true);
    systemPorts->addSystemPort(remoteSysPort);
    applyNewState(newState);
    EXPECT_EQ(getProgrammedState()->getSystemPorts()->size(), numPrevPorts + 1);
  }
  void addRemoteInterface(
      InterfaceID intfId,
      const Interface::Addresses& subnets) {
    auto newState = getProgrammedState()->clone();
    auto newInterfaces = newState->getInterfaces()->clone();
    auto numPrevIntfs = newInterfaces->size();
    auto newInterface = std::make_shared<Interface>(
        intfId,
        RouterID(0),
        std::nullopt,
        "RemoteIntf",
        folly::MacAddress("c6:ca:2b:2a:b1:b6"),
        9000,
        false,
        false,
        cfg::InterfaceType::SYSTEM_PORT);
    newInterface->setAddresses(subnets);
    newInterfaces->addInterface(newInterface);
    newState->resetIntfs(newInterfaces);
    applyNewState(newState);
    EXPECT_EQ(getProgrammedState()->getInterfaces()->size(), numPrevIntfs + 1);
  }
  void addRemoveNeighbor(
      const folly::IPAddressV6& neighborIp,
      InterfaceID intfID,
      bool add) {
    const VlanID kVlanID{utility::kBaseVlanId};
    auto ip = neighborIp;
    auto outState{getProgrammedState()->clone()};
    auto neighborTable =
        outState->getVlans()->getVlan(kVlanID)->getNdpTable()->modify(
            kVlanID, &outState);
    if (add) {
      const folly::MacAddress kNeighborMac{"2:3:4:5:6:7"};
      neighborTable->addPendingEntry(ip, intfID);
      neighborTable->updateEntry(
          ip, kNeighborMac, PortDescriptor(masterLogicalPortIds()[0]), intfID);
    } else {
      neighborTable->removeEntry(ip);
    }
    applyNewState(outState);
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
    addRemoteSysPort(SystemPortID(301));
  };
  verifyAcrossWarmBoots(setup, [] {});
}

TEST_F(HwVoqSwitchTest, addRemoveNeighbor) {
  auto setup = [this]() {
    auto config = utility::onePortPerInterfaceConfig(
        getHwSwitch(),
        masterLogicalPortIds(),
        getAsic()->desiredLoopbackMode());
    applyNewConfig(config);
    folly::IPAddressV6 kNeighborIp("1::2");
    InterfaceID kIntfId{101};
    // Add neighbor
    addRemoveNeighbor(kNeighborIp, kIntfId, true);
    // Remove neighbor
    addRemoveNeighbor(kNeighborIp, kIntfId, false);
  };
  verifyAcrossWarmBoots(setup, [] {});
}

TEST_F(HwVoqSwitchTest, remoteRouterInterface) {
  auto setup = [this]() {
    auto config = utility::onePortPerInterfaceConfig(
        getHwSwitch(),
        masterLogicalPortIds(),
        getAsic()->desiredLoopbackMode());
    applyNewConfig(config);
    auto constexpr remotePortId = 301;
    addRemoteSysPort(SystemPortID(remotePortId));
    addRemoteInterface(
        InterfaceID(remotePortId),
        // TODO - following assumes we haven't
        // already used up the subnets below for
        // local interfaces. In that sense it
        // has a implicit coupling with how ConfigFactory
        // generates subnets for local interfaces
        {
            {folly::IPAddress("100::1"), 64},
            {folly::IPAddress("100.0.0.1"), 24},
        });
  };
  verifyAcrossWarmBoots(setup, [] {});
}

} // namespace facebook::fboss
