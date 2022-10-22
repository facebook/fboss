// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwLinkStateDependentTest.h"
#include "fboss/agent/hw/test/HwTest.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/hw/test/HwTestStatUtils.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/InterfaceMap.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/EcmpSetupHelper.h"

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
        true /*interfaceHasSubnet*/);
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
    auto newState = getProgrammedState();
    auto newRemoteInterfaces =
        newState->getRemoteInterfaces()->modify(&newState);
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
    auto outState = getProgrammedState();
    auto isRemote = encapIndex != std::nullopt;
    auto interfaceMap = isRemote
        ? outState->getRemoteInterfaces()->modify(&outState)
        : outState->getInterfaces()->modify(&outState);
    auto interface = interfaceMap->getInterface(intfID)->clone();
    auto ndpTable = interfaceMap->getInterface(intfID)->getNdpTable();
    if (add) {
      const folly::MacAddress kNeighborMac{"2:3:4:5:6:7"};
      state::NeighborEntryFields ndp;
      ndp.mac() = kNeighborMac.toString();
      ndp.ipaddress() = neighborIp.str();
      ndp.portId() = port.toThrift();
      ndp.interfaceId() = static_cast<int>(intfID);
      ndp.state() = state::NeighborState::Reachable;
      if (encapIndex) {
        ndp.encapIndex() = *encapIndex;
      }
      ndp.isLocal() = encapIndex == std::nullopt;
      ndpTable.insert({*ndp.ipaddress(), ndp});
    } else {
      ndpTable.erase(neighborIp.str());
    }
    interface->setNdpTable(ndpTable);
    interfaceMap->updateNode(interface);
    applyNewState(outState);
  }
  void addRemoveNeighbor(PortDescriptor port, bool add) {
    utility::EcmpSetupAnyNPorts6 ecmpHelper(getProgrammedState());
    if (add) {
      applyNewState(ecmpHelper.resolveNextHops(getProgrammedState(), {port}));
    } else {
      applyNewState(ecmpHelper.unresolveNextHops(getProgrammedState(), {port}));
    }
  }

  void sendPacketHelper(bool isFrontPanel) {
    auto kPort = masterLogicalPortIds({cfg::PortType::INTERFACE_PORT})[0];
    folly::IPAddressV6 kNeighborIp("1::2");

    auto setup = [this, kPort, kNeighborIp]() {
      const InterfaceID kIntfId{101};
      addRemoveNeighbor(
          kNeighborIp, kIntfId, PortDescriptor(kPort), true /*add neighbor */);
    };

    auto verify = [this, isFrontPanel, kPort, kNeighborIp]() {
      folly::IPAddressV6 kSrcIp("1::1");
      const auto srcMac = utility::kLocalCpuMac();
      const auto dstMac = utility::kLocalCpuMac();

      auto txPacket = utility::makeUDPTxPacket(
          getHwSwitch(),
          std::nullopt, // vlanID
          srcMac,
          dstMac,
          kSrcIp,
          kNeighborIp,
          8000, // l4 src port
          8001); // l4 dst port

      XLOG(DBG3) << "\n"
                 << folly::hexDump(
                        txPacket->buf()->data(), txPacket->buf()->length());

      auto beforeOutPkts = getLatestPortStats(kPort).get_outUnicastPkts_();

      if (isFrontPanel) {
        const PortID port =
            masterLogicalPortIds({cfg::PortType::INTERFACE_PORT})[1];
        getHwSwitchEnsemble()->ensureSendPacketOutOfPort(
            std::move(txPacket), port);
      } else {
        getHwSwitchEnsemble()->ensureSendPacketSwitched(std::move(txPacket));
      }

      auto afterOutPkts = getLatestPortStats(kPort).get_outUnicastPkts_();

      EXPECT_EQ(afterOutPkts - 1, beforeOutPkts);
    };

    verifyAcrossWarmBoots(setup, verify);
  }
};

TEST_F(HwVoqSwitchTest, init) {
  auto setup = [this]() {};

  auto verify = [this]() {
    auto state = getProgrammedState();
    for (auto& port : *state->getPorts()) {
      if (port->isEnabled()) {
        EXPECT_EQ(port->getLoopbackMode(), getAsic()->desiredLoopbackMode());
      }
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
    addRemoteSysPort(SystemPortID(401));
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
    const PortDescriptor kPort(masterLogicalPortIds()[0]);
    // Add neighbor
    addRemoveNeighbor(kPort, true);
    // Remove neighbor
    addRemoveNeighbor(kPort, false);
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
    auto constexpr remotePortId = 401;
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
    auto constexpr remotePortId = 401;
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
    uint64_t dummyEncapIndex = 401;
    PortDescriptor kPort(kRemoteSysPortId);
    // Add neighbor
    addRemoveNeighbor(kNeighborIp, kIntfId, kPort, true, dummyEncapIndex);
    // Remove neighbor
    addRemoveNeighbor(kNeighborIp, kIntfId, kPort, false, dummyEncapIndex);
  };
  verifyAcrossWarmBoots(setup, [] {});
}

TEST_F(HwVoqSwitchTest, sendPacketCpu) {
  sendPacketHelper(false /* cpu */);
}

TEST_F(HwVoqSwitchTest, sendPacketFrontPanel) {
  sendPacketHelper(true /* front panel */);
}

} // namespace facebook::fboss
