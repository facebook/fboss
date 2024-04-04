// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/TxPacket.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwTestCoppUtils.h"
#include "fboss/agent/packet/EthFrame.h"
#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/utils/AclTestUtils.h"
#include "fboss/agent/test/utils/FabricTestUtils.h"
#include "fboss/agent/test/utils/LoadBalancerTestUtils.h"
#include "fboss/lib/CommonUtils.h"

DECLARE_int32(hwswitch_query_timeout);

namespace {
constexpr uint8_t kDefaultQueue = 0;
}

namespace facebook::fboss {
class AgentVoqSwitchTest : public AgentHwTest {
 public:
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    // Increase the query timeout to be 5sec
    FLAGS_hwswitch_query_timeout = 5000;
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

 protected:
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
    }
  }

  int sendPacket(
      const folly::IPAddressV6& dstIp,
      std::optional<PortID> frontPanelPort,
      std::optional<std::vector<uint8_t>> payload =
          std::optional<std::vector<uint8_t>>(),
      int dscp = 0x24) {
    folly::IPAddressV6 kSrcIp("1::1");
    const auto srcMac = utility::kLocalCpuMac();
    const auto dstMac = utility::kLocalCpuMac();

    auto txPacket = utility::makeUDPTxPacket(
        getSw(),
        std::nullopt, // vlanID
        srcMac,
        dstMac,
        kSrcIp,
        dstIp,
        8000, // l4 src port
        8001, // l4 dst port
        dscp << 2, // dscp
        255, // hopLimit
        payload);
    size_t txPacketSize = txPacket->buf()->length();

    XLOG(DBG5) << "\n"
               << folly::hexDump(
                      txPacket->buf()->data(), txPacket->buf()->length());

    if (frontPanelPort.has_value()) {
      getSw()->sendPacketOutOfPortAsync(std::move(txPacket), *frontPanelPort);
    } else {
      getSw()->sendPacketSwitchedAsync(std::move(txPacket));
    }
    return txPacketSize;
  }

  SystemPortID getSystemPortID(const PortDescriptor& port) {
    auto switchId =
        scopeResolver().scope(getProgrammedState(), port).switchId();
    auto sysPortRange = getProgrammedState()
                            ->getDsfNodes()
                            ->getNodeIf(switchId)
                            ->getSystemPortRange();
    CHECK(sysPortRange.has_value());
    return SystemPortID(port.intID() + *sysPortRange->minimum());
  }

  std::string kDscpAclName() const {
    return "dscp_acl";
  }
  std::string kDscpAclCounterName() const {
    return "dscp_acl_counter";
  }

  void addDscpAclWithCounter() {
    auto newCfg = initialConfig(*getAgentEnsemble());
    auto* acl = utility::addAcl(&newCfg, kDscpAclName());
    acl->dscp() = 0x24;
    utility::addAclStat(
        &newCfg,
        kDscpAclName(),
        kDscpAclCounterName(),
        utility::getAclCounterTypes(utility::getFirstAsic(this->getSw())));
    applyNewConfig(newCfg);
  }

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

  void setForceTrafficOverFabric(bool force) {
    applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      auto out = in->clone();
      for (const auto& [_, switchSetting] :
           std::as_const(*out->getSwitchSettings())) {
        auto newSwitchSettings = switchSetting->modify(&out);
        newSwitchSettings->setForceTrafficOverFabric(force);
      }
      return out;
    });
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
};

class AgentVoqSwitchWithFabricPortsTest : public AgentVoqSwitchTest {
 public:
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    // Increase the query timeout to be 5sec
    FLAGS_hwswitch_query_timeout = 5000;
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

 protected:
  void assertPortActiveState(PortID fabricPortId, bool expectActive) const {
    WITH_RETRIES({
      auto port = getProgrammedState()->getPorts()->getNodeIf(fabricPortId);
      EXPECT_EVENTUALLY_TRUE(port->isActive().has_value());
      EXPECT_EVENTUALLY_EQ(*port->isActive(), expectActive);
    });
  }
  void assertPortsActiveState(bool expectActive) const {
    WITH_RETRIES({
      for (const auto& fabricPortId : masterLogicalFabricPortIds()) {
        auto port = getProgrammedState()->getPorts()->getNodeIf(fabricPortId);
        EXPECT_EVENTUALLY_TRUE(port->isActive().has_value());
        EXPECT_EVENTUALLY_EQ(*port->isActive(), expectActive);
      }
    });
  }
  void assertPortAndDrainState(cfg::SwitchDrainState expectDrainState) const {
    bool expectDrained =
        expectDrainState == cfg::SwitchDrainState::DRAINED ? true : false;
    for (const auto& switchId : getSw()->getHwAsicTable()->getSwitchIDs()) {
      // reachability should always be there regardless of drain state
      utility::checkFabricReachability(getAgentEnsemble(), switchId);
      HwSwitchMatcher matcher(std::unordered_set<SwitchID>({switchId}));
      const auto& switchSettings =
          getProgrammedState()->getSwitchSettings()->getSwitchSettings(matcher);
      EXPECT_EQ(switchSettings->isSwitchDrained(), expectDrained);
    }
    // Drained - expect inactive
    // Undrained - expect active
    assertPortsActiveState(!expectDrained);
  }
  void verifyLocalForwarding() {
    // Setup neighbor entry
    utility::EcmpSetupAnyNPorts6 ecmpHelper(getProgrammedState());
    const auto kPort = ecmpHelper.ecmpPortDescriptorAt(0);
    addRemoveNeighbor(kPort, true /* add neighbor*/);
    auto sendPktAndVerify = [&](bool isFrontPanel) {
      auto beforeOutPkts =
          getLatestPortStats(kPort.phyPortID()).get_outUnicastPkts_();
      std::optional<PortID> frontPanelPort;
      if (isFrontPanel) {
        frontPanelPort = ecmpHelper.ecmpPortDescriptorAt(1).phyPortID();
      }
      sendPacket(ecmpHelper.ip(kPort), frontPanelPort);
      WITH_RETRIES({
        auto afterOutPkts =
            getLatestPortStats(kPort.phyPortID()).get_outUnicastPkts_();
        XLOG(DBG2) << " Before out pkts: " << beforeOutPkts
                   << " After out pkts: " << afterOutPkts;
        EXPECT_EVENTUALLY_EQ(afterOutPkts, beforeOutPkts + 1);
      });
    };
    sendPktAndVerify(false /*isFrontPanel*/);
    sendPktAndVerify(true /*isFrontPanel*/);
  }

 private:
  bool hideFabricPorts() const override {
    return false;
  }
};

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
    assertPortAndDrainState(cfg::SwitchDrainState::UNDRAINED);
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
    auto drainPort = [&](bool drain) {
      applyNewState([&](const std::shared_ptr<SwitchState>& in) {
        auto out = in->clone();
        auto port = out->getPorts()->getNodeIf(fabricPortId);
        auto newPort = port->modify(&out);
        newPort->setPortDrainState(
            drain ? cfg::PortDrainState::DRAINED
                  : cfg::PortDrainState::UNDRAINED);
        return out;
      });
      // Drained port == inactive, undrained port == active
      auto expectActive = !drain;
      assertPortActiveState(fabricPortId, expectActive);
      // Fabric reachability should be unchanged regardless of drain
      utility::checkPortFabricReachability(
          getAgentEnsemble(), SwitchID(0), fabricPortId);
    };
    drainPort(true);
    drainPort(false);
  };
  verifyAcrossWarmBoots([] {}, verify);
}

TEST_F(AgentVoqSwitchWithFabricPortsTest, fabricConnectivityMismatch) {
  auto verify = [this]() {
    auto fabricPortId = masterLogicalFabricPortIds()[0];
    auto cfg = initialConfig(*getAgentEnsemble());
    cfg::PortNeighbor nbr;
    nbr.remoteSystem() = "RemoteA";
    nbr.remotePort() = "portA";
    auto portCfg = utility::findCfgPort(cfg, fabricPortId);
    portCfg->expectedNeighborReachability() = {nbr};
    applyNewConfig(cfg);

    WITH_RETRIES({
      auto port = getProgrammedState()->getPorts()->getNodeIf(fabricPortId);
      EXPECT_EVENTUALLY_TRUE(port->getLedPortExternalState().has_value());
      EXPECT_EVENTUALLY_EQ(
          port->getLedPortExternalState().value(),
          PortLedExternalState::CABLING_ERROR);
    });
  };
  verifyAcrossWarmBoots([]() {}, verify);
}

TEST_F(AgentVoqSwitchWithFabricPortsTest, switchIsolate) {
  auto setup = [=, this]() {
    // Before drain all fabric ports should be active
    assertPortsActiveState(true);
    // Local forwarding works
    verifyLocalForwarding();
    auto newCfg = initialConfig(*getAgentEnsemble());
    *newCfg.switchSettings()->switchDrainState() =
        cfg::SwitchDrainState::DRAINED;
    applyNewConfig(newCfg);
  };

  auto verify = [this]() {
    assertPortAndDrainState(cfg::SwitchDrainState::DRAINED);
    // Local forwarding should continue to work even after drain
    verifyLocalForwarding();
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentVoqSwitchWithFabricPortsTest, minVoqThresholdDrainUndrain) {
  auto verify = [this]() {
    assertPortAndDrainState(cfg::SwitchDrainState::UNDRAINED);
    verifyLocalForwarding();
    auto updateMinLinksThreshold =
        [this](int minLinksToJoin, int minLinksToRemain) {
          auto newCfg = initialConfig(*getAgentEnsemble());
          // Set threshold higher than existing fabric ports
          newCfg.switchSettings()->minLinksToJoinVOQDomain() = minLinksToJoin;
          newCfg.switchSettings()->minLinksToRemainInVOQDomain() =
              minLinksToRemain;
          applyNewConfig(newCfg);
        };
    // Bump up threshold beyond existing fabric links, switch should drain
    updateMinLinksThreshold(
        masterLogicalFabricPortIds().size() + 10,
        masterLogicalFabricPortIds().size() + 5);
    assertPortAndDrainState(cfg::SwitchDrainState::DRAINED);
    // Verify local forwarding works post drain due to min links
    verifyLocalForwarding();
    // Bump up threshold beyond existing fabric links, switch should drain
    updateMinLinksThreshold(0, 0);
    assertPortAndDrainState(cfg::SwitchDrainState::UNDRAINED);
    // Verify local forwarding works post undrain once min links are satisfied
    verifyLocalForwarding();
  };
  verifyAcrossWarmBoots([]() {}, verify);
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

TEST_F(AgentVoqSwitchTest, fdrRciAndCoreRciWatermarks) {
  auto verify = [this]() {
    std::string out;
    for (const auto& switchId : getSw()->getHwAsicTable()->getSwitchIDs()) {
      getAgentEnsemble()->runDiagCommand(
          "setreg CIG_RCI_DEVICE_MAPPING 0\nsetreg CIG_RCI_CORE_MAPPING 0\n",
          out,
          switchId);
      getAgentEnsemble()->runDiagCommand("quit\n", out, switchId);
    }

    uint64_t fdrRciWatermarkBytes{};
    uint64_t coreRciWatermarkBytes{};

    WITH_RETRIES({
      getSw()->updateStats();
      for (const auto& switchWatermarksIter : getAllSwitchWatermarkStats()) {
        auto switchWatermarks = switchWatermarksIter.second;
        if (switchWatermarks.fdrRciWatermarkBytes().has_value()) {
          fdrRciWatermarkBytes +=
              switchWatermarks.fdrRciWatermarkBytes().value();
        }
        if (switchWatermarks.coreRciWatermarkBytes().has_value()) {
          coreRciWatermarkBytes +=
              switchWatermarks.coreRciWatermarkBytes().value();
        }
      }
      // Make sure that both counters have non zero values
      EXPECT_EVENTUALLY_GT(fdrRciWatermarkBytes, 0);
      EXPECT_EVENTUALLY_GT(coreRciWatermarkBytes, 0);
      XLOG(DBG2) << "FDR RCI watermark bytes : " << fdrRciWatermarkBytes
                 << ", Core DRM RCI watermark bytes : "
                 << coreRciWatermarkBytes;
    });
  };
  verifyAcrossWarmBoots([]() {}, verify);
}

class AgentVoqSwitchWithFabricPortsStartDrained
    : public AgentVoqSwitchWithFabricPortsTest {
 public:
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto config = AgentVoqSwitchWithFabricPortsTest::initialConfig(ensemble);
    // Set threshold higher than existing fabric ports
    config.switchSettings()->minLinksToJoinVOQDomain() =
        ensemble.masterLogicalFabricPortIds().size() + 10;
    config.switchSettings()->minLinksToRemainInVOQDomain() =
        ensemble.masterLogicalFabricPortIds().size() + 5;
    return config;
  }
};

TEST_F(AgentVoqSwitchWithFabricPortsStartDrained, assertLocalForwarding) {
  auto verify = [this]() {
    assertPortAndDrainState(cfg::SwitchDrainState::DRAINED);
    // Local forwarding should work even when we come up drained
    verifyLocalForwarding();
  };
  verifyAcrossWarmBoots([]() {}, verify);
}

TEST_F(AgentVoqSwitchWithFabricPortsTest, checkFabricPortSprayWithIsolate) {
  utility::EcmpSetupAnyNPorts6 ecmpHelper(getProgrammedState());
  const auto kPort = ecmpHelper.ecmpPortDescriptorAt(0);
  auto setup = [this, kPort, ecmpHelper]() {
    setForceTrafficOverFabric(true);
    addRemoveNeighbor(kPort, true /* add neighbor*/);
  };

  auto verify = [this, kPort, ecmpHelper]() {
    auto beforePkts =
        getLatestPortStats(kPort.phyPortID()).get_outUnicastPkts_();

    // Drain a fabric port
    auto fabricPortId =
        PortID(masterLogicalPortIds({cfg::PortType::FABRIC_PORT})[0]);
    applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      auto out = in->clone();
      auto port = out->getPorts()->getNodeIf(fabricPortId);
      auto newPort = port->modify(&out);
      newPort->setPortDrainState(cfg::PortDrainState::DRAINED);
      return out;
    });

    // Send 10K packets and spray on fabric ports
    for (auto i = 0; i < 10000; ++i) {
      sendPacket(
          ecmpHelper.ip(kPort), ecmpHelper.ecmpPortDescriptorAt(1).phyPortID());
    }
    WITH_RETRIES({
      auto afterPkts =
          getLatestPortStats(kPort.phyPortID()).get_outUnicastPkts_();
      XLOG(DBG2) << "Before pkts: " << beforePkts
                 << " After pkts: " << afterPkts;
      EXPECT_EVENTUALLY_GE(afterPkts, beforePkts + 10000);
      auto nifBytes = getLatestPortStats(kPort.phyPortID()).get_outBytes_();
      auto fabricPortStats = getLatestPortStats(masterLogicalFabricPortIds());
      auto fabricBytes = 0;
      for (const auto& idAndStats : fabricPortStats) {
        fabricBytes += idAndStats.second.get_outBytes_();
        if (idAndStats.first != fabricPortId) {
          EXPECT_EVENTUALLY_GT(idAndStats.second.get_outBytes_(), 0);
          EXPECT_EVENTUALLY_GT(idAndStats.second.get_inBytes_(), 0);
        }
      }
      XLOG(DBG2) << "NIF bytes: " << nifBytes
                 << " Fabric bytes: " << fabricBytes;
      EXPECT_EVENTUALLY_GE(fabricBytes, nifBytes);
      // Confirm load balance fails as the drained fabric port
      // should see close to 0 packets. We may see some control packtes.
      EXPECT_EVENTUALLY_FALSE(utility::isLoadBalanced(fabricPortStats, 15));

      // Confirm traffic is load balanced across all UNDRAINED fabric ports
      fabricPortStats.erase(fabricPortId);
      EXPECT_EVENTUALLY_TRUE(utility::isLoadBalanced(fabricPortStats, 15));
    });
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentVoqSwitchWithFabricPortsTest, checkFabricPortSpray) {
  utility::EcmpSetupAnyNPorts6 ecmpHelper(getProgrammedState());
  const auto kPort = ecmpHelper.ecmpPortDescriptorAt(0);
  auto setup = [this, kPort, ecmpHelper]() {
    setForceTrafficOverFabric(true);
    addRemoveNeighbor(kPort, true /* add neighbor*/);
  };

  auto verify = [this, kPort, ecmpHelper]() {
    auto beforePkts =
        getLatestPortStats(kPort.phyPortID()).get_outUnicastPkts_();
    for (auto i = 0; i < 10000; ++i) {
      sendPacket(
          ecmpHelper.ip(kPort), ecmpHelper.ecmpPortDescriptorAt(1).phyPortID());
    }
    WITH_RETRIES({
      auto afterPkts =
          getLatestPortStats(kPort.phyPortID()).get_outUnicastPkts_();
      XLOG(DBG2) << "Before pkts: " << beforePkts
                 << " After pkts: " << afterPkts;
      EXPECT_EVENTUALLY_GE(afterPkts, beforePkts + 10000);
      auto nifBytes = getLatestPortStats(kPort.phyPortID()).get_outBytes_();
      auto fabricPortStats = getLatestPortStats(masterLogicalFabricPortIds());
      auto fabricBytes = 0;
      for (const auto& idAndStats : fabricPortStats) {
        fabricBytes += idAndStats.second.get_outBytes_();
      }
      XLOG(DBG2) << "NIF bytes: " << nifBytes
                 << " Fabric bytes: " << fabricBytes;
      EXPECT_EVENTUALLY_GE(fabricBytes, nifBytes);
      EXPECT_EVENTUALLY_TRUE(utility::isLoadBalanced(fabricPortStats, 15));
    });
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentVoqSwitchWithFabricPortsTest, fdrCellDrops) {
  utility::EcmpSetupAnyNPorts6 ecmpHelper(getProgrammedState());
  const auto kPort = ecmpHelper.ecmpPortDescriptorAt(0);
  auto setup = [this, kPort]() {
    addRemoveNeighbor(kPort, true /* add neighbor*/);
    setForceTrafficOverFabric(true);
    std::string out;
    for (const auto& switchId : getSw()->getHwAsicTable()->getSwitchIDs()) {
      getAgentEnsemble()->runDiagCommand(
          "setreg FDA_OFM_CLASS_DROP_TH_CORE 0x001001001001001001001001\n",
          out,
          switchId);
      getAgentEnsemble()->runDiagCommand("quit\n", out, switchId);
    }
  };

  auto verify = [this, kPort, &ecmpHelper]() {
    auto sendPkts = [this, kPort, &ecmpHelper]() {
      for (auto i = 0; i < 1000; ++i) {
        sendPacket(
            ecmpHelper.ip(kPort),
            std::nullopt,
            std::vector<uint8_t>(1024, 0xff));
      }
    };
    int64_t fdrCellDrops = 0;
    WITH_RETRIES({
      sendPkts();
      fdrCellDrops = *getAggregatedSwitchDropStats().fdrCellDrops();
      // TLTimeseries value > 0
      EXPECT_EVENTUALLY_GT(fdrCellDrops, 0);
    });
    // Assert that we don't spuriously increment fdrCellDrops on every drop
    // stats. This would happen if we treated a stat as clear on read, while
    // in HW it was cumulative
    checkNoStatsChange(30 /*retries*/);
  };
  verifyAcrossWarmBoots(setup, verify);
}

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

TEST_F(AgentVoqSwitchTest, sendPacketCpuAndFrontPanel) {
  utility::EcmpSetupAnyNPorts6 ecmpHelper(getProgrammedState());
  const auto kPort = ecmpHelper.ecmpPortDescriptorAt(0);

  auto setup = [this, kPort, ecmpHelper]() {
    if (isSupportedOnAllAsics(HwAsic::Feature::ACL_TABLE_GROUP)) {
      addDscpAclWithCounter();
    }
    addRemoveNeighbor(kPort, true /* add neighbor*/);
  };

  auto verify = [this, kPort, ecmpHelper]() {
    auto sendPacketCpuFrontPanelHelper = [this, kPort, ecmpHelper](
                                             bool isFrontPanel) {
      auto getPortOutPktsBytes = [this](PortID port) {
        return std::make_pair(
            getLatestPortStats(port).get_outUnicastPkts_(),
            getLatestPortStats(port).get_outBytes_());
      };

      auto getAllQueueOutPktsBytes = [kPort, this]() {
        return std::make_pair(
            getLatestPortStats(kPort.phyPortID()).get_queueOutPackets_(),
            getLatestPortStats(kPort.phyPortID()).get_queueOutBytes_());
      };
      auto getAllVoQOutBytes = [kPort, this]() {
        return getLatestSysPortStats(getSystemPortID(kPort))
            .get_queueOutBytes_();
      };
      auto getAclPackets = [this]() {
        return utility::getAclInOutPackets(getSw(), kDscpAclCounterName());
      };

      auto printQueueStats = [](std::string queueStatDesc,
                                std::string packetsOrBytes,
                                std::map<int16_t, int64_t> queueStatsMap) {
        for (const auto& [queueId, pktsOrBytes] : queueStatsMap) {
          XLOG(DBG2) << queueStatDesc << ": Queue ID: " << queueId << ", "
                     << packetsOrBytes << ": " << pktsOrBytes;
        }
      };
      auto getRecyclePortPkts = [this]() {
        return *getLatestPortStats(PortID(1)).inUnicastPkts_();
      };

      int64_t beforeQueueOutPkts = 0, beforeQueueOutBytes = 0;
      int64_t afterQueueOutPkts = 0, afterQueueOutBytes = 0;
      int64_t beforeVoQOutBytes = 0, afterVoQOutBytes = 0;

      if (isSupportedOnAllAsics(HwAsic::Feature::L3_QOS)) {
        auto beforeAllQueueOut = getAllQueueOutPktsBytes();
        beforeQueueOutPkts = beforeAllQueueOut.first.at(kDefaultQueue);
        beforeQueueOutBytes = beforeAllQueueOut.second.at(kDefaultQueue);
        printQueueStats("Before Queue Out", "Packets", beforeAllQueueOut.first);
        printQueueStats("Before Queue Out", "Bytes", beforeAllQueueOut.second);
        auto beforeAllVoQOutBytes = getAllVoQOutBytes();
        beforeVoQOutBytes = beforeAllVoQOutBytes.at(kDefaultQueue);
        printQueueStats("Before VoQ Out", "Bytes", beforeAllVoQOutBytes);
      }

      auto [beforeOutPkts, beforeOutBytes] =
          getPortOutPktsBytes(kPort.phyPortID());
      auto beforeAclPkts =
          isSupportedOnAllAsics(HwAsic::Feature::ACL_TABLE_GROUP)
          ? getAclPackets()
          : 0;
      std::optional<PortID> frontPanelPort;
      uint64_t beforeFrontPanelOutBytes{0}, beforeFrontPanelOutPkts{0};
      if (isFrontPanel) {
        frontPanelPort = ecmpHelper.ecmpPortDescriptorAt(1).phyPortID();
        std::tie(beforeFrontPanelOutPkts, beforeFrontPanelOutBytes) =
            getPortOutPktsBytes(*frontPanelPort);
      }
      auto beforeRecyclePkts = getRecyclePortPkts();
      auto beforeSwitchDropStats = getAggregatedSwitchDropStats();
      auto txPacketSize = sendPacket(ecmpHelper.ip(kPort), frontPanelPort);

      auto [maxRetryCount, sleepTimeMsecs] =
          utility::getRetryCountAndDelay(utility::getFirstAsic(this->getSw()));
      WITH_RETRIES_N_TIMED(
          maxRetryCount, std::chrono::milliseconds(sleepTimeMsecs), {
            if (isSupportedOnAllAsics(HwAsic::Feature::L3_QOS)) {
              auto afterAllQueueOut = getAllQueueOutPktsBytes();
              afterQueueOutPkts = afterAllQueueOut.first.at(kDefaultQueue);
              afterQueueOutBytes = afterAllQueueOut.second.at(kDefaultQueue);
              auto afterAllVoQOutBytes = getAllVoQOutBytes();
              afterVoQOutBytes = afterAllVoQOutBytes.at(kDefaultQueue);
              printQueueStats("After VoQ Out", "Bytes", afterAllVoQOutBytes);
            }
            auto afterAclPkts =
                isSupportedOnAllAsics(HwAsic::Feature::ACL_TABLE_GROUP)
                ? getAclPackets()
                : 0;
            auto portOutPktsAndBytes = getPortOutPktsBytes(kPort.phyPortID());
            auto afterOutPkts = portOutPktsAndBytes.first;
            auto afterOutBytes = portOutPktsAndBytes.second;
            uint64_t afterFrontPanelOutBytes{0}, afterFrontPanelOutPkts{0};
            if (isFrontPanel) {
              std::tie(afterFrontPanelOutPkts, afterFrontPanelOutBytes) =
                  getPortOutPktsBytes(*frontPanelPort);
            }
            auto afterRecyclePkts = getRecyclePortPkts();
            XLOG(DBG2) << "Verifying: "
                       << (isFrontPanel ? "Send Packet from Front Panel Port"
                                        : "Send Packet from CPU Port")
                       << " Stats:: beforeOutPkts: " << beforeOutPkts
                       << " beforeOutBytes: " << beforeOutBytes
                       << " beforeQueueOutPkts: " << beforeQueueOutPkts
                       << " beforeQueueOutBytes: " << beforeQueueOutBytes
                       << " beforeVoQOutBytes: " << beforeVoQOutBytes
                       << " beforeFrontPanelPkts: " << beforeFrontPanelOutPkts
                       << " beforeFrontPanelBytes: " << beforeFrontPanelOutBytes
                       << " beforeRecyclePkts: " << beforeRecyclePkts
                       << " txPacketSize: " << txPacketSize
                       << " afterOutPkts: " << afterOutPkts
                       << " afterOutBytes: " << afterOutBytes
                       << " afterQueueOutPkts: " << afterQueueOutPkts
                       << " afterQueueOutBytes: " << afterQueueOutBytes
                       << " afterVoQOutBytes: " << afterVoQOutBytes
                       << " afterAclPkts: " << afterAclPkts
                       << " afterFrontPanelPkts: " << afterFrontPanelOutPkts
                       << " afterFrontPanelBytes: " << afterFrontPanelOutBytes
                       << " afterRecyclePkts: " << afterRecyclePkts;

            EXPECT_EVENTUALLY_EQ(afterOutPkts - 1, beforeOutPkts);
            int extraByteOffset = 0;
            auto asicType = utility::getFirstAsic(this->getSw())->getAsicType();
            auto asicMode = utility::getFirstAsic(this->getSw())->getAsicMode();
            if (asicMode != HwAsic::AsicMode::ASIC_MODE_SIM &&
                (asicType == cfg::AsicType::ASIC_TYPE_JERICHO2 ||
                 asicType == cfg::AsicType::ASIC_TYPE_JERICHO3)) {
              // Account for Ethernet FCS being counted in TX out bytes.
              extraByteOffset = utility::EthFrame::FCS_SIZE;
            }
            EXPECT_EVENTUALLY_EQ(
                afterOutBytes - txPacketSize - extraByteOffset, beforeOutBytes);
            if (isSupportedOnAllAsics(HwAsic::Feature::L3_QOS)) {
              EXPECT_EVENTUALLY_EQ(afterQueueOutPkts - 1, beforeQueueOutPkts);
              // CS00012267635: debug why queue counter is 310, when
              // txPacketSize is 322
              EXPECT_EVENTUALLY_GE(afterQueueOutBytes, beforeQueueOutBytes);
            }
            if (isSupportedOnAllAsics(HwAsic::Feature::ACL_TABLE_GROUP)) {
              EXPECT_EVENTUALLY_GT(afterAclPkts, beforeAclPkts);
            }
            if (isSupportedOnAllAsics(HwAsic::Feature::VOQ)) {
              EXPECT_EVENTUALLY_GT(afterVoQOutBytes, beforeVoQOutBytes);
            }
            if (frontPanelPort) {
              EXPECT_EVENTUALLY_GT(
                  afterFrontPanelOutBytes, beforeFrontPanelOutBytes);

              EXPECT_EVENTUALLY_GT(
                  afterFrontPanelOutPkts, beforeFrontPanelOutPkts);
            } else if (asicMode != HwAsic::AsicMode::ASIC_MODE_SIM) {
              EXPECT_EVENTUALLY_EQ(beforeRecyclePkts + 1, afterRecyclePkts);
            }
            auto afterSwitchDropStats = getAggregatedSwitchDropStats();
            if (asicMode != HwAsic::AsicMode::ASIC_MODE_SIM &&
                asicType == cfg::AsicType::ASIC_TYPE_JERICHO3) {
              XLOG(DBG2) << " Queue resolution drops, before: "
                         << *beforeSwitchDropStats.queueResolutionDrops()
                         << " after: "
                         << *afterSwitchDropStats.queueResolutionDrops();
              EXPECT_EVENTUALLY_EQ(
                  *afterSwitchDropStats.queueResolutionDrops(),
                  *beforeSwitchDropStats.queueResolutionDrops() + 1);
            }
          });
    };

    sendPacketCpuFrontPanelHelper(false /* cpu */);
    sendPacketCpuFrontPanelHelper(true /* front panel*/);
  };

  verifyAcrossWarmBoots(setup, verify);
}
} // namespace facebook::fboss
