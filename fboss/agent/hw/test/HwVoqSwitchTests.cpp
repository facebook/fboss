// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwLinkStateDependentTest.h"
#include "fboss/agent/hw/test/HwTest.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/InterfaceMap.h"
#include "fboss/agent/state/SwitchState.h"

namespace facebook::fboss {
namespace {
const SwitchID kRemoteSwitchId(2);
}
class HwVoqSwitchTest : public HwLinkStateDependentTest {
 public:
  cfg::SwitchConfig initialConfig() const override {
    return utility::onePortPerInterfaceConfig(
        getHwSwitch(),
        masterLogicalPortIds(),
        getAsic()->desiredLoopbackMode(),
        false /*interfaceHasSubnet*/);
  }
  void SetUp() override {
    HwLinkStateDependentTest::SetUp();
    ASSERT_EQ(getHwSwitch()->getSwitchType(), cfg::SwitchType::VOQ);
    ASSERT_TRUE(getHwSwitch()->getSwitchId().has_value());
  }

 protected:
  void addRemoteSysPort(SystemPortID portId) {
    auto newState = getProgrammedState()->clone();
    auto localPort = *newState->getSystemPorts()->begin();
    auto remoteSystemPorts =
        newState->getRemoteSystemPorts()->modify(&newState);
    auto numPrevPorts = remoteSystemPorts->size();
    auto remoteSysPort = std::make_shared<SystemPort>(portId);
    remoteSysPort->setSwitchId(kRemoteSwitchId);
    remoteSysPort->setNumVoqs(localPort->getNumVoqs());
    remoteSysPort->setCoreIndex(localPort->getCoreIndex());
    remoteSysPort->setCorePortIndex(localPort->getCorePortIndex());
    remoteSysPort->setSpeedMbps(localPort->getSpeedMbps());
    remoteSysPort->setEnabled(true);
    remoteSystemPorts->addSystemPort(remoteSysPort);
    applyNewState(newState);
    EXPECT_EQ(
        getProgrammedState()->getRemoteSystemPorts()->size(), numPrevPorts + 1);
  }
  void addRemoteInterface(
      InterfaceID intfId,
      const Interface::Addresses& subnets) {
    auto newState = getProgrammedState()->clone();
    auto newRemoteInterfaces = newState->getRemoteInterfaces()->clone();
    auto numPrevIntfs = newRemoteInterfaces->size();
    auto newRemoteInterface = std::make_shared<Interface>(
        intfId,
        RouterID(0),
        std::nullopt,
        "RemoteIntf",
        folly::MacAddress("c6:ca:2b:2a:b1:b6"),
        9000,
        false,
        false,
        cfg::InterfaceType::SYSTEM_PORT);
    newRemoteInterface->setAddresses(subnets);
    newRemoteInterfaces->addInterface(newRemoteInterface);
    newState->resetRemoteIntfs(newRemoteInterfaces);
    applyNewState(newState);
    EXPECT_EQ(
        getProgrammedState()->getRemoteInterfaces()->size(), numPrevIntfs + 1);
  }
  void addRemoveNeighbor(
      const folly::IPAddressV6& neighborIp,
      InterfaceID intfID,
      PortDescriptor port,
      bool add,
      std::optional<int64_t> encapIndex = std::nullopt) {
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
          ip,
          kNeighborMac,
          port,
          intfID,
          std::nullopt,
          encapIndex,
          encapIndex == std::nullopt);
    } else {
      neighborTable->removeEntry(ip);
    }
    applyNewState(outState);
  }
};

TEST_F(HwVoqSwitchTest, init) {
  auto setup = [this]() {};

  auto verify = [this]() {
    auto state = getProgrammedState();
    for (auto& port : *state->getPorts()) {
      EXPECT_EQ(port->getAdminState(), cfg::PortState::ENABLED);
      EXPECT_EQ(port->getLoopbackMode(), getAsic()->desiredLoopbackMode());
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
    const InterfaceID kIntfId{101};
    const PortDescriptor kPort(masterLogicalPortIds()[0]);
    // Add neighbor
    addRemoveNeighbor(kNeighborIp, kIntfId, kPort, true);
    // Remove neighbor
    addRemoveNeighbor(kNeighborIp, kIntfId, kPort, false);
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

TEST_F(HwVoqSwitchTest, addRemoveRemoteNeighbor) {
  auto setup = [this]() {
    auto config = utility::onePortPerInterfaceConfig(
        getHwSwitch(),
        masterLogicalPortIds(),
        getAsic()->desiredLoopbackMode());
    applyNewConfig(config);
    auto constexpr remotePortId = 301;
    const SystemPortID kRemoteSysPortId(remotePortId);
    addRemoteSysPort(kRemoteSysPortId);
    const InterfaceID kIntfId(remotePortId);
    addRemoteInterface(
        kIntfId,
        // TODO - following assumes we haven't
        // already used up the subnets below for
        // local interfaces. In that sense it
        // has a implicit coupling with how ConfigFactory
        // generates subnets for local interfaces
        {
            {folly::IPAddress("100::1"), 64},
            {folly::IPAddress("100.0.0.1"), 24},
        });
    folly::IPAddressV6 kNeighborIp("100::2");
    uint64_t dummyEncapIndex = 301;
    PortDescriptor kPort(kRemoteSysPortId);
    // Add neighbor
    addRemoveNeighbor(kNeighborIp, kIntfId, kPort, true, dummyEncapIndex);
    // Remove neighbor
    addRemoveNeighbor(kNeighborIp, kIntfId, kPort, false);
  };
  verifyAcrossWarmBoots(setup, [] {});
}

} // namespace facebook::fboss
