// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/TxPacket.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwTestCoppUtils.h"
#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/utils/FabricTestUtils.h"
#include "fboss/lib/CommonUtils.h"

DECLARE_bool(enable_stats_update_thread);

namespace facebook::fboss {
class AgentVoqSwitchTest : public AgentHwTest {
 public:
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    // Before m-mpu agent test, use first Asic for initialization.
    auto switchIds = ensemble.getSw()->getHwAsicTable()->getSwitchIDs();
    CHECK_GE(switchIds.size(), 1);
    auto asic =
        ensemble.getSw()->getHwAsicTable()->getHwAsic(*switchIds.cbegin());
    auto config = utility::onePortPerInterfaceConfig(
        ensemble.getSw(),
        ensemble.masterLogicalPortIds(),
        true /*interfaceHasSubnet*/);
    const auto& cpuStreamTypes =
        asic->getQueueStreamTypes(cfg::PortType::CPU_PORT);
    for (const auto& cpuStreamType : cpuStreamTypes) {
      if (asic->getDefaultNumPortQueues(
              cpuStreamType, cfg::PortType::CPU_PORT)) {
        // cpu queues supported
        addCpuTrafficPolicy(config, asic);
        utility::addCpuQueueConfig(config, asic, ensemble.isSai());
        break;
      }
    }
    return config;
  }

  void SetUp() override {
    AgentHwTest::SetUp();
    if (!IsSkipped()) {
      ASSERT_TRUE(
          std::any_of(getAsics().begin(), getAsics().end(), [](auto& iter) {
            return iter.second->getSwitchType() == cfg::SwitchType::VOQ;
          }));
    }
  }

  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    return {production_features::ProductionFeature::VOQ};
  }

  // API to send a local service discovery packet which is an IPv6
  // multicast paket with UDP payload. This packet with a destination
  // MAC as the unicast NIF port MAC helps recreate CS00012323788.
  void sendLocalServiceDiscoveryMulticastPacket(
      const PortID outPort,
      const int numPackets) {
    auto vlanId = utility::firstVlanID(getProgrammedState());
    auto intfMac = utility::getFirstInterfaceMac(getProgrammedState());
    auto srcIp = folly::IPAddressV6("fe80::ff:fe00:f0b");
    auto dstIp = folly::IPAddressV6("ff15::efc0:988f");
    auto srcMac = utility::MacAddressGenerator().get(intfMac.u64NBO() + 1);
    std::vector<uint8_t> serviceDiscoveryPayload = {
        0x42, 0x54, 0x2d, 0x53, 0x45, 0x41, 0x52, 0x43, 0x48, 0x20, 0x2a, 0x20,
        0x48, 0x54, 0x54, 0x50, 0x2f, 0x31, 0x2e, 0x31, 0x0d, 0x0a, 0x48, 0x6f,
        0x73, 0x74, 0x3a, 0x20, 0x5b, 0x66, 0x66, 0x31, 0x35, 0x3a, 0x3a, 0x65,
        0x66, 0x63, 0x30, 0x3a, 0x39, 0x38, 0x38, 0x66, 0x5d, 0x3a, 0x36, 0x37,
        0x37, 0x31, 0x0d, 0x0a, 0x50, 0x6f, 0x72, 0x74, 0x3a, 0x20, 0x36, 0x38,
        0x38, 0x31, 0x0d, 0x0a, 0x49, 0x6e, 0x66, 0x6f, 0x68, 0x61, 0x73, 0x68,
        0x3a, 0x20, 0x61, 0x66, 0x31, 0x37, 0x34, 0x36, 0x35, 0x39, 0x64, 0x37,
        0x31, 0x31, 0x38, 0x64, 0x32, 0x34, 0x34, 0x61, 0x30, 0x36, 0x31, 0x33};
    for (int i = 0; i < numPackets; i++) {
      auto txPacket = utility::makeUDPTxPacket(
          getSw(),
          vlanId,
          srcMac,
          intfMac,
          srcIp,
          dstIp,
          6771,
          6771,
          0,
          254,
          serviceDiscoveryPayload);
      getSw()->sendPacketOutOfPortAsync(std::move(txPacket), outPort);
      ;
    }
  }

 private:
  void addCpuTrafficPolicy(cfg::SwitchConfig& cfg, const HwAsic* asic) const {
    cfg::CPUTrafficPolicyConfig cpuConfig;
    std::vector<cfg::PacketRxReasonToQueue> rxReasonToQueues;
    std::vector<std::pair<cfg::PacketRxReason, uint16_t>>
        rxReasonToQueueMappings = {
            std::pair(
                cfg::PacketRxReason::BGP, utility::getCoppHighPriQueueId(asic)),
            std::pair(
                cfg::PacketRxReason::BGPV6,
                utility::getCoppHighPriQueueId(asic)),
            std::pair(
                cfg::PacketRxReason::CPU_IS_NHOP, utility::kCoppMidPriQueueId),
        };
    for (auto rxEntry : rxReasonToQueueMappings) {
      auto rxReasonToQueue = cfg::PacketRxReasonToQueue();
      rxReasonToQueue.rxReason() = rxEntry.first;
      rxReasonToQueue.queueId() = rxEntry.second;
      rxReasonToQueues.push_back(rxReasonToQueue);
    }
    cpuConfig.rxReasonToQueueOrderedList() = rxReasonToQueues;
    cfg.cpuTrafficPolicy() = cpuConfig;
  }

 protected:
  void addRemoveNeighbor(
      PortDescriptor port,
      bool add,
      std::optional<int64_t> encapIdx = std::nullopt) {
    utility::EcmpSetupAnyNPorts6 ecmpHelper(getProgrammedState());
    if (add) {
      applyNewState([&](const std::shared_ptr<SwitchState>& in) {
        return ecmpHelper.resolveNextHops(in, {port});
      });
    } else {
      applyNewState([&](const std::shared_ptr<SwitchState>& in) {
        return ecmpHelper.unresolveNextHops(in, {port});
      });
    }
  }
};

class AgentVoqSwitchWithFabricPortsTest : public AgentVoqSwitchTest {
 public:
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto config = utility::onePortPerInterfaceConfig(
        ensemble.getSw(),
        ensemble.masterLogicalPortIds(),
        true, /*interfaceHasSubnet*/
        true, /*setInterfaceMac*/
        utility::kBaseVlanId,
        true /*enable fabric ports*/);
    utility::populatePortExpectedNeighbors(
        ensemble.masterLogicalPortIds(), config);
    return config;
  }

 private:
  bool hideFabricPorts() const override {
    return false;
  }
};

TEST_F(AgentVoqSwitchTest, addRemoveNeighbor) {
  auto setup = [this]() {
    const PortDescriptor kPort(getAgentEnsemble()->masterLogicalPortIds(
        {cfg::PortType::INTERFACE_PORT})[0]);
    // Add neighbor
    addRemoveNeighbor(kPort, true);
    // Remove neighbor
    addRemoveNeighbor(kPort, false);
  };
  verifyAcrossWarmBoots(setup, [] {});
}

TEST_F(AgentVoqSwitchWithFabricPortsTest, init) {
  auto setup = []() {};

  auto verify = [this]() {
    auto state = getProgrammedState();
    for (auto& portMap : std::as_const(*state->getPorts())) {
      for (auto& port : std::as_const(*portMap.second)) {
        if (port.second->isEnabled()) {
          EXPECT_EQ(
              port.second->getLoopbackMode(),
              // TODO: Handle multiple Asics
              getAsics().cbegin()->second->getDesiredLoopbackMode(
                  port.second->getPortType()));
        }
      }
    }
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentVoqSwitchWithFabricPortsTest, collectStats) {
  auto verify = [this]() {
    EXPECT_GT(getProgrammedState()->getPorts()->numNodes(), 0);
    getSw()->updateStats();
  };
  verifyAcrossWarmBoots([] {}, verify);
}

TEST_F(AgentVoqSwitchWithFabricPortsTest, checkFabricReachability) {
  auto verify = [this]() {
    for (const auto& switchId : getSw()->getHwAsicTable()->getSwitchIDs()) {
      utility::checkFabricReachability(getAgentEnsemble(), switchId);
    }
  };
  verifyAcrossWarmBoots([] {}, verify);
}

TEST_F(AgentVoqSwitchWithFabricPortsTest, fabricIsolate) {
  auto verify = [=, this]() {
    EXPECT_GT(getProgrammedState()->getPorts()->numNodes(), 0);
    auto fabricPortId =
        PortID(masterLogicalPortIds({cfg::PortType::FABRIC_PORT})[0]);
    utility::checkPortFabricReachability(
        getAgentEnsemble(), SwitchID(0), fabricPortId);
    auto assertActiveState = [this](PortID portId, bool expectActive) {
      WITH_RETRIES({
        auto port = getProgrammedState()->getPorts()->getNodeIf(portId);
        EXPECT_EVENTUALLY_TRUE(port->isActive().has_value());
        EXPECT_EVENTUALLY_EQ(*port->isActive(), expectActive);
      });
    };
    // Undrained fab port should be active
    assertActiveState(fabricPortId, true);
    applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      auto out = in->clone();
      auto port = out->getPorts()->getNodeIf(fabricPortId);
      auto newPort = port->modify(&out);
      newPort->setPortDrainState(cfg::PortDrainState::DRAINED);
      return out;
    });
    utility::checkPortFabricReachability(
        getAgentEnsemble(), SwitchID(0), fabricPortId);
    // Drained fab port should be inactive
    assertActiveState(fabricPortId, false);
  };
  verifyAcrossWarmBoots([] {}, verify);
}

TEST_F(AgentVoqSwitchWithFabricPortsTest, switchIsolate) {
  auto setup = [=, this]() {
    auto newCfg = initialConfig(*getAgentEnsemble());
    *newCfg.switchSettings()->switchDrainState() =
        cfg::SwitchDrainState::DRAINED;
    applyNewConfig(newCfg);
  };

  auto verify = [this]() {
    for (const auto& switchId : getSw()->getHwAsicTable()->getSwitchIDs()) {
      utility::checkFabricReachability(getAgentEnsemble(), switchId);
    }
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentVoqSwitchWithFabricPortsTest, verifyNifMulticastTrafficDropped) {
  constexpr static auto kNumPacketsToSend{1000};
  auto setup = []() {};

  auto verify = [=, this]() {
    auto beforePkts = getLatestPortStats(masterLogicalInterfacePortIds()[0])
                          .get_outUnicastPkts_();
    sendLocalServiceDiscoveryMulticastPacket(
        masterLogicalInterfacePortIds()[0], kNumPacketsToSend);
    WITH_RETRIES({
      auto afterPkts = getLatestPortStats(masterLogicalInterfacePortIds()[0])
                           .get_outUnicastPkts_();
      XLOG(DBG2) << "Before pkts: " << beforePkts
                 << " After pkts: " << afterPkts;
      EXPECT_EVENTUALLY_GE(afterPkts, beforePkts + kNumPacketsToSend);
    });

    // Wait for some time and make sure that fabric stats dont increment.
    sleep(5);
    auto fabricPortStats = getLatestPortStats(masterLogicalFabricPortIds());
    auto fabricBytes = 0;
    for (const auto& idAndStats : fabricPortStats) {
      fabricBytes += idAndStats.second.get_outBytes_();
    }
    // Even though NIF will see RX/TX bytes, fabric will always be zero
    // as these packets are expected to be dropped without being sent
    // out on fabric.
    EXPECT_EQ(fabricBytes, 0);
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentVoqSwitchWithFabricPortsTest, overdrainPct) {
  auto setup = []() {};
  auto verify = [this]() {
    WITH_RETRIES({
      EXPECT_EVENTUALLY_EQ(
          0, fb303::fbData->getCounter("switch.0.fabric_overdrain_pct"));
    });
    auto enableFabPorts = [this](bool enable) {
      auto cfg = initialConfig(*getAgentEnsemble());
      for (auto& port : *cfg.ports()) {
        if (*port.portType() == cfg::PortType::FABRIC_PORT) {
          port.state() =
              enable ? cfg::PortState::ENABLED : cfg::PortState::DISABLED;
        }
      }
      applyNewConfig(cfg);
    };
    // Disable all fabric port
    enableFabPorts(false);
    WITH_RETRIES({
      EXPECT_EVENTUALLY_EQ(
          100, fb303::fbData->getCounter("switch.0.fabric_overdrain_pct"));
    });
    // Enable all fabric port
    enableFabPorts(true);
    WITH_RETRIES({
      EXPECT_EVENTUALLY_EQ(
          0, fb303::fbData->getCounter("switch.0.fabric_overdrain_pct"));
    });
  };
  verifyAcrossWarmBoots(setup, verify);
}
} // namespace facebook::fboss
