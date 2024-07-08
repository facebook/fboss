// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/TxPacket.h"
#include "fboss/agent/hw/HwResourceStatsPublisher.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/packet/EthFrame.h"
#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/utils/AclTestUtils.h"
#include "fboss/agent/test/utils/CoppTestUtils.h"
#include "fboss/agent/test/utils/FabricTestUtils.h"
#include "fboss/agent/test/utils/LoadBalancerTestUtils.h"
#include "fboss/agent/test/utils/PacketSnooper.h"
#include "fboss/agent/test/utils/PortTestUtils.h"
#include "fboss/agent/test/utils/TrapPacketUtils.h"
#include "fboss/agent/test/utils/VoqTestUtils.h"
#include "fboss/lib/CommonUtils.h"

DECLARE_bool(disable_looped_fabric_ports);
DECLARE_bool(enable_stats_update_thread);
DECLARE_int32(ecmp_resource_percentage);

namespace {
constexpr uint8_t kDefaultQueue = 0;
}

using namespace facebook::fb303;
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
        auto l3Asics = ensemble.getSw()->getHwAsicTable()->getL3Asics();
        addCpuTrafficPolicy(config, l3Asics);
        utility::addCpuQueueConfig(config, l3Asics, ensemble.isSai());
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
  void
  rxPacketToCpuHelper(uint16_t l4SrcPort, uint16_t l4DstPort, uint8_t queueId) {
    utility::EcmpSetupAnyNPorts6 ecmpHelper(getProgrammedState());
    auto kPort = ecmpHelper.ecmpPortDescriptorAt(0);

    auto verify = [this, ecmpHelper, kPort, l4SrcPort, l4DstPort, queueId]() {
      // TODO(skhare)
      // Send to only one IPv6 address for ease of debugging.
      // Once SAI implementation bugs are fixed, send to ALL interface
      // addresses.
      auto ipAddrs =
          *(initialConfig(*getAgentEnsemble()).interfaces()[0].ipAddresses());
      auto ipv6Addr =
          std::find_if(ipAddrs.begin(), ipAddrs.end(), [](const auto& ipAddr) {
            auto ip = folly::IPAddress::createNetwork(ipAddr, -1, false).first;
            return ip.isV6();
          });

      auto dstIp =
          folly::IPAddress::createNetwork(*ipv6Addr, -1, false).first.asV6();
      folly::IPAddressV6 kSrcIp("1::1");
      const auto srcMac = folly::MacAddress("00:00:01:02:03:04");
      const auto dstMac = utility::kLocalCpuMac();

      auto createTxPacket =
          [this, srcMac, dstMac, kSrcIp, dstIp, l4SrcPort, l4DstPort]() {
            return utility::makeTCPTxPacket(
                getSw(),
                std::nullopt, // vlanID
                srcMac,
                dstMac,
                kSrcIp,
                dstIp,
                l4SrcPort,
                l4DstPort);
          };

      const PortID port = ecmpHelper.ecmpPortDescriptorAt(1).phyPortID();
      auto switchId =
          scopeResolver().scope(getProgrammedState(), port).switchId();
      auto [beforeQueueOutPkts, beforeQueueOutBytes] =
          utility::getCpuQueueOutPacketsAndBytes(getSw(), queueId, switchId);

      auto txPacket = createTxPacket();
      size_t txPacketSize = txPacket->buf()->length();

      getSw()->sendPacketOutOfPortAsync(std::move(txPacket), port);
      utility::SwSwitchPacketSnooper snooper(getSw(), "snoop");
      txPacket = createTxPacket();
      std::unique_ptr<folly::IOBuf> rxBuf;
      WITH_RETRIES({
        auto frameRx = snooper.waitForPacket(1);
        ASSERT_EVENTUALLY_TRUE(frameRx.has_value());
        CHECK(frameRx.has_value());
        rxBuf = std::move(*frameRx);
      });
      WITH_RETRIES({
        XLOG(DBG3) << "TX Packet Dump::" << std::endl
                   << folly::hexDump(
                          txPacket->buf()->data(), txPacket->buf()->length());
        XLOG(DBG3) << "RX Packet Dump::" << std::endl
                   << folly::hexDump(rxBuf->data(), rxBuf->length());

        XLOG(DBG2) << "TX Packet Length: " << txPacket->buf()->length()
                   << " RX Packet Length: " << rxBuf->length();
        EXPECT_EVENTUALLY_EQ(txPacket->buf()->length(), rxBuf->length());
        EXPECT_EVENTUALLY_EQ(
            0, memcmp(txPacket->buf()->data(), rxBuf->data(), rxBuf->length()));

        auto [afterQueueOutPkts, afterQueueOutBytes] =
            utility::getCpuQueueOutPacketsAndBytes(getSw(), queueId, switchId);

        XLOG(DBG2) << "Stats:: queueId: " << static_cast<int>(queueId)
                   << " beforeQueueOutPkts: " << beforeQueueOutPkts
                   << " beforeQueueOutBytes: " << beforeQueueOutBytes
                   << " txPacketSize: " << txPacketSize
                   << " afterQueueOutPkts: " << afterQueueOutPkts
                   << " afterQueueOutBytes: " << afterQueueOutBytes;

        EXPECT_EVENTUALLY_EQ(afterQueueOutPkts - 1, beforeQueueOutPkts);
        // CS00012267635: debug why queue counter is 362, when txPacketSize is
        // 322
        EXPECT_EVENTUALLY_GE(afterQueueOutBytes, beforeQueueOutBytes);

        for (auto i = 0; i <
             utility::getCoppHighPriQueueId(getAgentEnsemble()->getL3Asics());
             i++) {
          auto [outPkts, outBytes] =
              utility::getCpuQueueOutPacketsAndBytes(getSw(), i, switchId);
          XLOG(DBG2) << "QueueID: " << i << " outPkts: " << outPkts
                     << " outBytes: " << outBytes;
        }
      });
    };

    verifyAcrossWarmBoots([] {}, verify);
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
        utility::getAclCounterTypes(getAgentEnsemble()->getL3Asics()));
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
  void addCpuTrafficPolicy(
      cfg::SwitchConfig& cfg,
      std::vector<const HwAsic*>& l3Asics) const {
    cfg::CPUTrafficPolicyConfig cpuConfig;
    std::vector<cfg::PacketRxReasonToQueue> rxReasonToQueues;
    std::vector<std::pair<cfg::PacketRxReason, uint16_t>>
        rxReasonToQueueMappings = {
            std::pair(
                cfg::PacketRxReason::BGP,
                utility::getCoppHighPriQueueId(l3Asics)),
            std::pair(
                cfg::PacketRxReason::BGPV6,
                utility::getCoppHighPriQueueId(l3Asics)),
            std::pair(
                cfg::PacketRxReason::CPU_IS_NHOP,
                utility::getCoppMidPriQueueId(l3Asics)),
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
  void assertPortAndDrainState(cfg::SwitchDrainState expectDrainState) const {
    bool expectDrained =
        expectDrainState == cfg::SwitchDrainState::DRAINED ? true : false;
    for (const auto& switchId : getSw()->getHwAsicTable()->getSwitchIDs()) {
      // reachability should always be there regardless of drain state
      utility::checkFabricConnectivity(getAgentEnsemble(), switchId);
      HwSwitchMatcher matcher(std::unordered_set<SwitchID>({switchId}));
      const auto& switchSettings =
          getProgrammedState()->getSwitchSettings()->getSwitchSettings(matcher);
      EXPECT_EQ(switchSettings->isSwitchDrained(), expectDrained);
    }
    // Drained - expect inactive
    // Undrained - expect active
    utility::checkFabricPortsActiveState(
        getAgentEnsemble(), masterLogicalFabricPortIds(), !expectDrained);
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
  void setCmdLineFlagOverrides() const override {
    AgentHwTest::setCmdLineFlagOverrides();
    FLAGS_hide_fabric_ports = false;
    // Allow disabling of looped ports. This should
    // be a noop for VOQ switches
    FLAGS_disable_looped_fabric_ports = true;
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

TEST_F(AgentVoqSwitchWithFabricPortsTest, checkFabricConnectivity) {
  auto verify = [this]() {
    for (const auto& switchId : getSw()->getHwAsicTable()->getSwitchIDs()) {
      utility::checkFabricConnectivity(getAgentEnsemble(), switchId);
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
      std::vector<PortID> fabricPortIds({static_cast<PortID>(fabricPortId)});
      utility::checkFabricPortsActiveState(
          getAgentEnsemble(), fabricPortIds, expectActive);
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
          PortLedExternalState::CABLING_ERROR_LOOP_DETECTED);
    });
  };
  verifyAcrossWarmBoots([]() {}, verify);
}

TEST_F(AgentVoqSwitchWithFabricPortsTest, switchIsolate) {
  auto setup = [=, this]() {
    // Before drain all fabric ports should be active
    utility::checkFabricPortsActiveState(
        getAgentEnsemble(),
        masterLogicalFabricPortIds(),
        true /*expectActive*/);
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
          utility::getRetryCountAndDelay(getSw()->getHwAsicTable());
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
            auto asic =
                utility::checkSameAndGetAsic(getAgentEnsemble()->getL3Asics());
            const auto asicMode = asic->getAsicMode();
            const auto asicType = asic->getAsicType();
            if (asic->getAsicMode() != HwAsic::AsicMode::ASIC_MODE_SIM) {
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

TEST_F(AgentVoqSwitchTest, trapPktsOnPort) {
  utility::EcmpSetupAnyNPorts6 ecmpHelper(getProgrammedState());
  const auto kPort = ecmpHelper.ecmpPortDescriptorAt(0);
  auto setup = [this, kPort, &ecmpHelper]() {
    auto cfg = initialConfig(*getAgentEnsemble());
    utility::addTrapPacketAcl(&cfg, kPort.phyPortID());
    applyNewConfig(cfg);
    applyNewState([=](const std::shared_ptr<SwitchState>& in) {
      return ecmpHelper.resolveNextHops(in, {kPort});
    });
  };
  auto verify = [this, kPort, &ecmpHelper]() {
    utility::SwSwitchPacketSnooper snooper(getSw(), "snoop");
    auto frontPanelPort = ecmpHelper.ecmpPortDescriptorAt(1).phyPortID();
    sendPacket(ecmpHelper.ip(kPort), frontPanelPort);
    WITH_RETRIES({
      auto frameRx = snooper.waitForPacket(1);
      EXPECT_EVENTUALLY_TRUE(frameRx.has_value());
    });
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentVoqSwitchTest, rxPacketToCpu) {
  rxPacketToCpuHelper(
      utility::kNonSpecialPort1,
      utility::kNonSpecialPort2,
      utility::getCoppMidPriQueueId(getAgentEnsemble()->getL3Asics()));
}

TEST_F(AgentVoqSwitchTest, rxPacketToCpuBgpDstPort) {
  rxPacketToCpuHelper(
      utility::kNonSpecialPort1,
      utility::kBgpPort,
      utility::getCoppHighPriQueueId(getAgentEnsemble()->getL3Asics()));
}

TEST_F(AgentVoqSwitchTest, rxPacketToCpuBgpSrcPort) {
  rxPacketToCpuHelper(
      utility::kBgpPort,
      utility::kNonSpecialPort1,
      utility::getCoppHighPriQueueId(getAgentEnsemble()->getL3Asics()));
}

TEST_F(AgentVoqSwitchTest, localForwardingPostIsolate) {
  utility::EcmpSetupAnyNPorts6 ecmpHelper(getProgrammedState());
  const auto kPort = ecmpHelper.ecmpPortDescriptorAt(0);
  auto setup = [this, kPort]() {
    auto newCfg = initialConfig(*getAgentEnsemble());
    *newCfg.switchSettings()->switchDrainState() =
        cfg::SwitchDrainState::DRAINED;
    applyNewConfig(newCfg);
    addRemoveNeighbor(kPort, true /* add neighbor*/);
  };

  auto verify = [this, kPort, &ecmpHelper]() {
    auto sendPktAndVerify = [&](std::optional<PortID> portToSendFrom) {
      auto beforePkts =
          getLatestPortStats(kPort.phyPortID()).get_outUnicastPkts_();
      sendPacket(ecmpHelper.ip(kPort), portToSendFrom);
      WITH_RETRIES({
        auto afterPkts =
            getLatestPortStats(kPort.phyPortID()).get_outUnicastPkts_();
        XLOG(DBG2) << "Before pkts: " << beforePkts
                   << " After pkts: " << afterPkts;
        EXPECT_EVENTUALLY_GE(afterPkts, beforePkts + 1);
      });
    };
    // CPU send
    sendPktAndVerify(std::nullopt);
    // Front panel send
    sendPktAndVerify(ecmpHelper.ecmpPortDescriptorAt(1).phyPortID());
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentVoqSwitchTest, stressLocalForwardingPostIsolate) {
  utility::EcmpSetupAnyNPorts6 ecmpHelper(getProgrammedState());
  const auto kPort = ecmpHelper.ecmpPortDescriptorAt(0);
  auto setup = [this, kPort]() {
    auto newCfg = initialConfig(*getAgentEnsemble());
    *newCfg.switchSettings()->switchDrainState() =
        cfg::SwitchDrainState::DRAINED;
    applyNewConfig(newCfg);
    addRemoveNeighbor(kPort, true /* add neighbor*/);
  };

  auto verify = [this, kPort, &ecmpHelper]() {
    auto beforePkts =
        getLatestPortStats(kPort.phyPortID()).get_outUnicastPkts_();
    for (auto i = 0; i < 10000; ++i) {
      // CPU send
      sendPacket(ecmpHelper.ip(kPort), std::nullopt);
      // Front panel send
      sendPacket(
          ecmpHelper.ip(kPort), ecmpHelper.ecmpPortDescriptorAt(1).phyPortID());
    }
    WITH_RETRIES({
      auto afterPkts =
          getLatestPortStats(kPort.phyPortID()).get_outUnicastPkts_();
      XLOG(DBG2) << "Before pkts: " << beforePkts
                 << " After pkts: " << afterPkts;
      EXPECT_EVENTUALLY_GE(afterPkts, beforePkts + 20000);
    });
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentVoqSwitchTest, localSystemPortEcmp) {
  auto setup = [this]() {
    utility::EcmpSetupTargetedPorts6 ecmpHelper(getProgrammedState());
    auto prefix = RoutePrefixV6{folly::IPAddressV6("1::1"), 128};
    flat_set<PortDescriptor> localSysPorts;
    for (auto& systemPortMap :
         std::as_const(*getProgrammedState()->getSystemPorts())) {
      for (auto& [_, localSysPort] : std::as_const(*systemPortMap.second)) {
        localSysPorts.insert(PortDescriptor(localSysPort->getID()));
      }
    }
    applyNewState([=](const std::shared_ptr<SwitchState>& in) {
      return ecmpHelper.resolveNextHops(in, localSysPorts);
    });
    auto wrapper = getSw()->getRouteUpdater();
    ecmpHelper.programRoutes(&wrapper, localSysPorts, {prefix});
  };
  verifyAcrossWarmBoots(setup, [] {});
}

TEST_F(AgentVoqSwitchTest, packetIntegrityError) {
  utility::EcmpSetupAnyNPorts6 ecmpHelper(getProgrammedState());
  auto port = ecmpHelper.ecmpPortDescriptorAt(0);
  auto setup = [=, this]() { addRemoveNeighbor(port, true /*add*/); };
  auto verify = [=, this]() {
    const auto dstIp = ecmpHelper.ip(port);
    auto switchId = scopeResolver().scope(port.phyPortID()).switchId();
    auto switchAsic = getSw()->getHwAsicTable()->getHwAsic(switchId);
    std::string out;
    if (switchAsic->getAsicType() == cfg::AsicType::ASIC_TYPE_JERICHO2) {
      getAgentEnsemble()->runDiagCommand(
          "m SPB_FORCE_CRC_ERROR FORCE_CRC_ERROR_ON_DATA=1 FORCE_CRC_ERROR_ON_CRC=1\n",
          out);
    } else if (switchAsic->getAsicType() == cfg::AsicType::ASIC_TYPE_JERICHO3) {
      getAgentEnsemble()->runDiagCommand(
          "m IRE_FORCE_CRC_ERROR FORCE_CRC_ERROR_ON_CRC=1\n", out);
    } else {
      throw FbossError(
          "Unsupported ASIC type: ",
          apache::thrift::util::enumNameSafe(switchAsic->getAsicType()));
    }
    getAgentEnsemble()->runDiagCommand("quit\n", out);
    sendPacket(dstIp, std::nullopt, std::vector<uint8_t>(1024, 0xff));
    WITH_RETRIES({
      auto switchIndex =
          getSw()->getSwitchInfoTable().getSwitchIndexFromSwitchId(switchId);
      auto switchStats = getSw()->getHwSwitchStatsExpensive()[switchIndex];
      auto pktIntegrityDrops =
          switchStats.switchDropStats()->packetIntegrityDrops().value_or(0);
      XLOG(INFO) << " Packet integrity drops: " << pktIntegrityDrops;
      EXPECT_EVENTUALLY_GT(pktIntegrityDrops, 0);
    });
    // Assert that packet Integrity drops don't continuously increment.
    // Packet integrity drop counter is clear on read from HW. So we
    // accumulate its value in memory. If HW/SDK ever changed this to
    // not be clear on read, but cumulative, then our approach would
    // yeild constantly increasing values. Assert against that.
    checkNoStatsChange(30);
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentVoqSwitchTest, dramEnqueueDequeueBytes) {
  utility::EcmpSetupAnyNPorts6 ecmpHelper(getProgrammedState());
  const auto kPort = ecmpHelper.ecmpPortDescriptorAt(0);
  auto setup = [this, kPort]() {
    addRemoveNeighbor(kPort, true /* add neighbor*/);
  };

  auto verify = [this, kPort, &ecmpHelper]() {
    // Disable both port TX and credit watchdog
    utility::setCreditWatchdogAndPortTx(
        getAgentEnsemble(), kPort.phyPortID(), false);
    auto sendPkts = [this, kPort, &ecmpHelper]() {
      for (auto i = 0; i < 1000; ++i) {
        sendPacket(ecmpHelper.ip(kPort), std::nullopt);
      }
    };
    int64_t dramEnqueuedBytes = 0;
    auto switchId = scopeResolver().scope(kPort.phyPortID()).switchId();
    auto switchIndex =
        getSw()->getSwitchInfoTable().getSwitchIndexFromSwitchId(switchId);
    WITH_RETRIES({
      sendPkts();
      auto switchStats = getSw()->getHwSwitchStatsExpensive()[switchIndex];
      dramEnqueuedBytes =
          *switchStats.fb303GlobalStats()->dram_enqueued_bytes();
      XLOG(DBG2) << "Dram enqueued bytes : " << dramEnqueuedBytes;
      EXPECT_EVENTUALLY_GT(dramEnqueuedBytes, 0);
    });
    // Enable port TX
    utility::setPortTx(getAgentEnsemble(), kPort.phyPortID(), true);
    WITH_RETRIES({
      auto switchStats = getSw()->getHwSwitchStatsExpensive()[switchIndex];
      auto dramDequeuedBytes =
          *switchStats.fb303GlobalStats()->dram_dequeued_bytes();
      XLOG(DBG2) << "Dram dequeued bytes : " << dramDequeuedBytes;
      EXPECT_EVENTUALLY_GT(dramDequeuedBytes, 0);
    });
    // Assert that Dram enqueue/dequeue bytes don't continuously increment
    // Eventually all pkts should be dequeued and we should stop getting
    // increments
    checkNoStatsChange(60);
  };
  verifyAcrossWarmBoots(setup, verify);
}

class AgentVoqSwitchWithMultipleDsfNodesTest : public AgentVoqSwitchTest {
 public:
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto cfg = AgentVoqSwitchTest::initialConfig(ensemble);
    cfg.dsfNodes() = *overrideDsfNodes(*cfg.dsfNodes());
    return cfg;
  }

  std::optional<std::map<int64_t, cfg::DsfNode>> overrideDsfNodes(
      const std::map<int64_t, cfg::DsfNode>& curDsfNodes) const {
    return utility::addRemoteDsfNodeCfg(curDsfNodes, 1);
  }
  SwitchID getRemoteVoqSwitchId() const {
    auto dsfNodes = getSw()->getConfig().dsfNodes();
    // We added remote switch Id at the end
    auto [switchId, remoteNode] = *dsfNodes->rbegin();
    CHECK(*remoteNode.type() == cfg::DsfNodeType::INTERFACE_NODE);
    return SwitchID(switchId);
  }
};

TEST_F(AgentVoqSwitchWithMultipleDsfNodesTest, twoDsfNodes) {
  verifyAcrossWarmBoots([] {}, [] {});
}

TEST_F(AgentVoqSwitchWithMultipleDsfNodesTest, remoteSystemPort) {
  auto setup = [this]() {
    // in addRemoteDsfNodeCfg, we use numCores to calculate the remoteSwitchId
    // keeping remote switch id passed below in sync with it
    int numCores =
        utility::checkSameAndGetAsic(getAgentEnsemble()->getL3Asics())
            ->getNumCores();
    auto getStats = [] {
      return std::make_tuple(
          fbData->getCounter(kSystemPortsFree), fbData->getCounter(kVoqsFree));
    };
    auto [beforeSysPortsFree, beforeVoqsFree] = getStats();
    applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      return utility::addRemoteSysPort(
          in,
          scopeResolver(),
          SystemPortID(401),
          static_cast<SwitchID>(numCores));
    });
    WITH_RETRIES({
      auto [afterSysPortsFree, afterVoqsFree] = getStats();
      XLOG(INFO) << " Before sysPortsFree: " << beforeSysPortsFree
                 << " voqsFree: " << beforeVoqsFree
                 << " after sysPortsFree: " << afterSysPortsFree
                 << " voqsFree: " << afterVoqsFree;
      EXPECT_EVENTUALLY_EQ(beforeSysPortsFree - 1, afterSysPortsFree);
      // 8 VOQs allocated per sys port
      EXPECT_EVENTUALLY_EQ(beforeVoqsFree - 8, afterVoqsFree);
    });
  };
  verifyAcrossWarmBoots(setup, [] {});
}

TEST_F(AgentVoqSwitchWithMultipleDsfNodesTest, remoteRouterInterface) {
  auto setup = [this]() {
    // in addRemoteDsfNodeCfg, we use numCores to calculate the remoteSwitchId
    // keeping remote switch id passed below in sync with it
    int numCores =
        utility::checkSameAndGetAsic(getAgentEnsemble()->getL3Asics())
            ->getNumCores();
    auto constexpr remotePortId = 401;
    applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      return utility::addRemoteSysPort(
          in,
          scopeResolver(),
          SystemPortID(remotePortId),
          static_cast<SwitchID>(numCores));
    });

    applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      return utility::addRemoteInterface(
          in,
          scopeResolver(),
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
    });
  };
  verifyAcrossWarmBoots(setup, [] {});
}

TEST_F(AgentVoqSwitchWithMultipleDsfNodesTest, addRemoveRemoteNeighbor) {
  auto setup = [this]() {
    // in addRemoteDsfNodeCfg, we use numCores to calculate the remoteSwitchId
    // keeping remote switch id passed below in sync with it
    int numCores =
        utility::checkSameAndGetAsic(getAgentEnsemble()->getL3Asics())
            ->getNumCores();
    auto constexpr remotePortId = 401;
    const SystemPortID kRemoteSysPortId(remotePortId);
    applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      return utility::addRemoteSysPort(
          in,
          scopeResolver(),
          kRemoteSysPortId,
          static_cast<SwitchID>(numCores));
    });
    const InterfaceID kIntfId(remotePortId);
    applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      return utility::addRemoteInterface(
          in,
          scopeResolver(),
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
    });
    folly::IPAddressV6 kNeighborIp("100::2");
    PortDescriptor kPort(kRemoteSysPortId);
    // Add neighbor
    applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      return utility::addRemoveRemoteNeighbor(
          in,
          scopeResolver(),
          kNeighborIp,
          kIntfId,
          kPort,
          true,
          utility::getDummyEncapIndex(getAgentEnsemble()));
    });

    // Remove neighbor
    applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      return utility::addRemoveRemoteNeighbor(
          in,
          scopeResolver(),
          kNeighborIp,
          kIntfId,
          kPort,
          false,
          utility::getDummyEncapIndex(getAgentEnsemble()));
    });
  };
  verifyAcrossWarmBoots(setup, [] {});
}

TEST_F(AgentVoqSwitchWithMultipleDsfNodesTest, voqDelete) {
  auto constexpr remotePortId = 401;
  const SystemPortID kRemoteSysPortId(remotePortId);
  folly::IPAddressV6 kNeighborIp("100::2");
  auto setup = [=, this]() {
    // in addRemoteDsfNodeCfg, we use numCores to calculate the remoteSwitchId
    // keeping remote switch id passed below in sync with it
    int numCores =
        utility::checkSameAndGetAsic(getAgentEnsemble()->getL3Asics())
            ->getNumCores();
    applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      return utility::addRemoteSysPort(
          in,
          scopeResolver(),
          kRemoteSysPortId,
          static_cast<SwitchID>(numCores));
    });
    const InterfaceID kIntfId(remotePortId);
    applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      return utility::addRemoteInterface(
          in,
          scopeResolver(),
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
    });
    PortDescriptor kPort(kRemoteSysPortId);
    // Add neighbor
    applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      return utility::addRemoveRemoteNeighbor(
          in,
          scopeResolver(),
          kNeighborIp,
          kIntfId,
          kPort,
          true,
          utility::getDummyEncapIndex(getAgentEnsemble()));
    });
  };
  auto verify = [=, this]() {
    auto getVoQDeletedPkts = [=, this]() {
      if (!isSupportedOnAllAsics(HwAsic::Feature::VOQ_DELETE_COUNTER)) {
        return 0L;
      }
      return getLatestSysPortStats(kRemoteSysPortId)
          .get_queueCreditWatchdogDeletedPackets_()
          .at(kDefaultQueue);
    };

    auto voqDeletedPktsBefore = getVoQDeletedPkts();
    utility::EcmpSetupAnyNPorts6 ecmpHelper(getProgrammedState());
    auto frontPanelPort = ecmpHelper.ecmpPortDescriptorAt(1).phyPortID();
    for (auto i = 0; i < 100; ++i) {
      // Send pkts via front panel
      sendPacket(kNeighborIp, frontPanelPort, std::vector<uint8_t>(1024, 0xff));
    }
    WITH_RETRIES({
      auto voqDeletedPktsAfter = getVoQDeletedPkts();
      XLOG(INFO) << "Voq deleted pkts, before: " << voqDeletedPktsBefore
                 << " after: " << voqDeletedPktsAfter;
      EXPECT_EVENTUALLY_EQ(voqDeletedPktsBefore + 100, voqDeletedPktsAfter);
    });
  };
  verifyAcrossWarmBoots(setup, verify);
}

class AgentVoqSwitchFullScaleDsfNodesTest : public AgentVoqSwitchTest {
 public:
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto cfg = AgentVoqSwitchTest::initialConfig(ensemble);
    cfg.dsfNodes() = *overrideDsfNodes(*cfg.dsfNodes());
    cfg.loadBalancers()->push_back(
        utility::getEcmpFullHashConfig(ensemble.getL3Asics()));
    return cfg;
  }

  std::optional<std::map<int64_t, cfg::DsfNode>> overrideDsfNodes(
      const std::map<int64_t, cfg::DsfNode>& curDsfNodes) const {
    return utility::addRemoteDsfNodeCfg(curDsfNodes);
  }

 protected:
  int getMaxEcmpWidth() const {
    // J2 and J3 only supports variable width
    return utility::checkSameAndGetAsic(getAgentEnsemble()->getL3Asics())
        ->getMaxVariableWidthEcmpSize();
  }

  int getMaxEcmpGroup() const {
    return 64;
  }

  // Resolve and return list of local nhops (only NIF ports)
  std::vector<PortDescriptor> resolveLocalNhops(
      utility::EcmpSetupTargetedPorts6& ecmpHelper) {
    auto ports = getProgrammedState()->getPorts()->getAllNodes();
    std::vector<PortDescriptor> portDescs;
    std::for_each(
        ports->begin(),
        ports->end(),
        [this, &portDescs](const auto& idAndPort) {
          const auto port = idAndPort.second;
          if (port->getPortType() == cfg::PortType::INTERFACE_PORT) {
            portDescs.push_back(
                PortDescriptor(getSystemPortID(PortDescriptor(port->getID()))));
          }
        });

    applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      auto out = in->clone();
      for (const auto& portDesc : portDescs) {
        out = ecmpHelper.resolveNextHops(out, {portDesc});
      }
      return out;
    });
    return portDescs;
  }

 private:
  void setCmdLineFlagOverrides() const override {
    AgentVoqSwitchTest::setCmdLineFlagOverrides();
    // Disable stats update to improve performance
    FLAGS_enable_stats_update_thread = false;
    // Allow 100% ECMP resource usage
    FLAGS_ecmp_resource_percentage = 100;
  }
};

TEST_F(AgentVoqSwitchFullScaleDsfNodesTest, systemPortScaleTest) {
  auto setup = [this]() {
    applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      return utility::setupRemoteIntfAndSysPorts(
          in,
          scopeResolver(),
          getSw()->getConfig(),
          isSupportedOnAllAsics(HwAsic::Feature::RESERVED_ENCAP_INDEX_RANGE));
    });
  };
  verifyAcrossWarmBoots(setup, [] {});
}

TEST_F(AgentVoqSwitchFullScaleDsfNodesTest, remoteNeighborWithEcmpGroup) {
  const auto kEcmpWidth = getMaxEcmpWidth();
  const auto kMaxDeviation = 25;
  FLAGS_ecmp_width = kEcmpWidth;
  boost::container::flat_set<PortDescriptor> sysPortDescs;
  auto setup = [&]() {
    applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      return utility::setupRemoteIntfAndSysPorts(
          in,
          scopeResolver(),
          getSw()->getConfig(),
          isSupportedOnAllAsics(HwAsic::Feature::RESERVED_ENCAP_INDEX_RANGE));
    });
    utility::EcmpSetupTargetedPorts6 ecmpHelper(getProgrammedState());
    // Trigger config apply to add remote interface routes as directly connected
    // in RIB. This is to resolve ECMP members pointing to remote nexthops.
    applyNewConfig(getSw()->getConfig());

    // Resolve remote nhops and get a list of remote sysPort descriptors
    sysPortDescs = utility::resolveRemoteNhops(getAgentEnsemble(), ecmpHelper);

    for (int i = 0; i < getMaxEcmpGroup(); i++) {
      auto prefix = RoutePrefixV6{
          folly::IPAddressV6(folly::to<std::string>(i, "::", i)),
          static_cast<uint8_t>(i == 0 ? 0 : 128)};
      auto routeUpdater = getSw()->getRouteUpdater();
      ecmpHelper.programRoutes(
          &routeUpdater,
          flat_set<PortDescriptor>(
              std::make_move_iterator(sysPortDescs.begin() + i),
              std::make_move_iterator(sysPortDescs.begin() + i + kEcmpWidth)),
          {prefix});
    }
  };
  auto verify = [&]() {
    // Send and verify packets across voq drops.
    auto defaultRouteSysPorts = std::vector<PortDescriptor>(
        sysPortDescs.begin(), sysPortDescs.begin() + kEcmpWidth);
    std::function<std::map<SystemPortID, HwSysPortStats>(
        const std::vector<SystemPortID>&)>
        getSysPortStatsFn = [&](const std::vector<SystemPortID>& portIds) {
          getSw()->updateStats();
          return getLatestSysPortStats(portIds);
        };
    utility::pumpTrafficAndVerifyLoadBalanced(
        [&]() {
          utility::pumpTraffic(
              true, /* isV6 */
              utility::getAllocatePktFn(getAgentEnsemble()),
              utility::getSendPktFunc(getAgentEnsemble()),
              utility::kLocalCpuMac(), /* dstMac */
              std::nullopt, /* vlan */
              std::nullopt, /* frontPanelPortToLoopTraffic */
              255, /* hopLimit */
              1000000 /* numPackets */);
        },
        [&]() {
          auto ports = std::make_unique<std::vector<int32_t>>();
          for (auto sysPortDecs : defaultRouteSysPorts) {
            ports->push_back(static_cast<int32_t>(sysPortDecs.sysPortID()));
          }
          getSw()->clearPortStats(ports);
        },
        [&]() {
          WITH_RETRIES(EXPECT_EVENTUALLY_TRUE(utility::isLoadBalanced(
              defaultRouteSysPorts,
              {},
              getSysPortStatsFn,
              kMaxDeviation,
              false)));
          return true;
        });
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentVoqSwitchFullScaleDsfNodesTest, remoteAndLocalLoadBalance) {
  const auto kEcmpWidth = 16;
  const auto kMaxDeviation = 25;
  FLAGS_ecmp_width = kEcmpWidth;
  std::vector<PortDescriptor> sysPortDescs;
  auto setup = [&]() {
    applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      return utility::setupRemoteIntfAndSysPorts(
          in,
          scopeResolver(),
          getSw()->getConfig(),
          isSupportedOnAllAsics(HwAsic::Feature::RESERVED_ENCAP_INDEX_RANGE));
    });
    utility::EcmpSetupTargetedPorts6 ecmpHelper(getProgrammedState());
    // Trigger config apply to add remote interface routes as directly connected
    // in RIB. This is to resolve ECMP members pointing to remote nexthops.
    applyNewConfig(getSw()->getConfig());

    // Resolve remote and local nhops and get a list of sysPort descriptors
    auto remoteSysPortDescs =
        utility::resolveRemoteNhops(getAgentEnsemble(), ecmpHelper);
    auto localSysPortDescs = resolveLocalNhops(ecmpHelper);

    sysPortDescs.insert(
        sysPortDescs.end(),
        remoteSysPortDescs.begin(),
        remoteSysPortDescs.begin() + kEcmpWidth / 2);
    sysPortDescs.insert(
        sysPortDescs.end(),
        localSysPortDescs.begin(),
        localSysPortDescs.begin() + kEcmpWidth / 2);

    auto prefix = RoutePrefixV6{folly::IPAddressV6("0::0"), 0};
    auto routeUpdater = getSw()->getRouteUpdater();
    ecmpHelper.programRoutes(
        &routeUpdater,
        flat_set<PortDescriptor>(
            std::make_move_iterator(sysPortDescs.begin()),
            std::make_move_iterator(sysPortDescs.end())),
        {prefix});
  };
  auto verify = [&]() {
    // Send and verify packets across voq drops.
    std::function<std::map<SystemPortID, HwSysPortStats>(
        const std::vector<SystemPortID>&)>
        getSysPortStatsFn = [&](const std::vector<SystemPortID>& portIds) {
          getSw()->updateStats();
          return getLatestSysPortStats(portIds);
        };
    utility::pumpTrafficAndVerifyLoadBalanced(
        [&]() {
          utility::pumpTraffic(
              true, /* isV6 */
              utility::getAllocatePktFn(getAgentEnsemble()),
              utility::getSendPktFunc(getAgentEnsemble()),
              utility::kLocalCpuMac(), /* dstMac */
              std::nullopt, /* vlan */
              std::nullopt, /* frontPanelPortToLoopTraffic */
              255, /* hopLimit */
              10000 /* numPackets */);
        },
        [&]() {
          auto ports = std::make_unique<std::vector<int32_t>>();
          for (auto sysPortDecs : sysPortDescs) {
            ports->push_back(static_cast<int32_t>(sysPortDecs.sysPortID()));
          }
          getSw()->clearPortStats(ports);
        },
        [&]() {
          WITH_RETRIES(EXPECT_EVENTUALLY_TRUE(utility::isLoadBalanced(
              sysPortDescs, {}, getSysPortStatsFn, kMaxDeviation, false)));
          return true;
        });
  };
  verifyAcrossWarmBoots(setup, verify);
};

TEST_F(AgentVoqSwitchFullScaleDsfNodesTest, stressProgramEcmpRoutes) {
  auto kEcmpWidth = getMaxEcmpWidth();
  FLAGS_ecmp_width = kEcmpWidth;
  // Stress add/delete 40 iterations of 5 routes with ECMP width.
  // 40 iterations take ~17 mins on j3.
  const auto routeScale = 5;
  const auto numIterations = 40;
  auto setup = [&]() {
    applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      return utility::setupRemoteIntfAndSysPorts(
          in,
          scopeResolver(),
          getSw()->getConfig(),
          isSupportedOnAllAsics(HwAsic::Feature::RESERVED_ENCAP_INDEX_RANGE));
    });
    utility::EcmpSetupTargetedPorts6 ecmpHelper(getProgrammedState());
    // Trigger config apply to add remote interface routes as directly connected
    // in RIB. This is to resolve ECMP members pointing to remote nexthops.
    applyNewConfig(getSw()->getConfig());

    // Resolve remote nhops and get a list of remote sysPort descriptors
    auto sysPortDescs =
        utility::resolveRemoteNhops(getAgentEnsemble(), ecmpHelper);

    for (int iter = 0; iter < numIterations; iter++) {
      std::vector<RoutePrefixV6> routes;
      for (int i = 0; i < routeScale; i++) {
        auto prefix = RoutePrefixV6{
            folly::IPAddressV6(folly::to<std::string>(i + 1, "::", i + 1)),
            128};
        auto routeUpdater = getSw()->getRouteUpdater();
        ecmpHelper.programRoutes(
            &routeUpdater,
            flat_set<PortDescriptor>(
                std::make_move_iterator(sysPortDescs.begin() + i),
                std::make_move_iterator(sysPortDescs.begin() + i + kEcmpWidth)),
            {prefix});
        routes.push_back(prefix);
      }
      auto routeUpdater = getSw()->getRouteUpdater();
      ecmpHelper.unprogramRoutes(&routeUpdater, routes);
    }
  };
  verifyAcrossWarmBoots(setup, [] {});
}

} // namespace facebook::fboss
