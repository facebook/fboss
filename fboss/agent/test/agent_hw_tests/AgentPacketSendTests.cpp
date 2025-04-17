// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/AgentHwTest.h"

#include "fboss/agent/TxPacket.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/test/TrunkUtils.h"
#include "fboss/agent/test/utils/CoppTestUtils.h"
#include "fboss/agent/test/utils/MultiPortTrafficTestUtils.h"
#include "fboss/lib/CommonUtils.h"

#include "fboss/agent/test/gen-cpp2/production_features_types.h"

#include <chrono>

using namespace std::chrono;

namespace {
constexpr int kAggId{500};
} // unnamed namespace

namespace facebook::fboss {

class AgentPacketSendTest : public AgentHwTest {
 public:
  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    return {production_features::ProductionFeature::CPU_RX_TX};
  }

  void waitForTxDoneOnPort(
      PortID port,
      uint64_t expectedOutPkts,
      HwPortStats& before) const {
    AgentEnsemble* ensemble = getAgentEnsemble();
    auto waitForExpectedOutPackets = [&](const auto& newStats) {
      uint64_t outPackets{0}, outDiscards{0};
      const auto& portStatsIter = newStats.find(port);
      if (portStatsIter != newStats.end()) {
        const auto& portStats = portStatsIter->second;
        outPackets = portStats.get_outUnicastPkts_() -
            folly::copy(before.outUnicastPkts_().value());
        outDiscards = portStats.get_outDiscards_() -
            folly::copy(before.outDiscards_().value());
      }
      // Check to see if outPackets + outDiscards add up to the expected!
      XLOG(DBG3) << "Port: " << port << ", out packets: " << outPackets
                 << ", discarded packets: " << outDiscards
                 << " vs expectedOutPkts " << expectedOutPkts;
      return outPackets + outDiscards >= expectedOutPkts;
    };

    WITH_RETRIES_N_TIMED(10, std::chrono::seconds(1), {
      EXPECT_EVENTUALLY_TRUE(waitForExpectedOutPackets(
          ensemble->getLatestPortStats(ensemble->masterLogicalPortIds())));
    });
  }

  void enablePortTx(PortID portID, bool enable) {
    getAgentEnsemble()->applyNewState(
        [=](const std::shared_ptr<SwitchState>& state) {
          auto newState = state->clone();
          auto port =
              newState->getPorts()->getNodeIf(portID)->modify(&newState);
          port->setTxEnable(enable);

          return newState;
        },
        enable ? "enable port TX" : "disable port TX");
  }
};

class AgentSwitchedPacketSendTest : public AgentPacketSendTest {
 public:
  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    auto features = AgentPacketSendTest::getProductionFeaturesVerified();
    features.push_back(production_features::ProductionFeature::VLAN);
    return features;
  }
};

TEST_F(AgentPacketSendTest, LldpToFrontPanelOutOfPort) {
  auto setup = [=]() {};
  auto verify = [=, this]() {
    auto portStatsBefore =
        getLatestPortStats(masterLogicalInterfacePortIds()[0]);
    auto vlanId = getVlanIDForTx();
    auto intfMac =
        utility::getMacForFirstInterfaceWithPorts(getProgrammedState());
    auto srcMac = utility::MacAddressGenerator().get(intfMac.u64NBO() + 1);
    auto payLoadSize = 256;
    auto txPacket = utility::makeEthTxPacket(
        getSw(),
        vlanId,
        srcMac,
        folly::MacAddress("01:80:c2:00:00:0e"),
        facebook::fboss::ETHERTYPE::ETHERTYPE_LLDP,
        std::vector<uint8_t>(payLoadSize, 0xff));
    // vlan tag should be removed
    auto pktLengthSent = EthHdr::SIZE + payLoadSize;
    getSw()->sendPacketOutOfPortAsync(
        std::move(txPacket), masterLogicalInterfacePortIds()[0], std::nullopt);
    WITH_RETRIES({
      auto portStatsAfter =
          getLatestPortStats(masterLogicalInterfacePortIds()[0]);
      XLOG(DBG2) << "Lldp Packet:" << " before pkts:"
                 << *portStatsBefore.outMulticastPkts_()
                 << ", after pkts:" << *portStatsAfter.outMulticastPkts_()
                 << ", before bytes:" << *portStatsBefore.outBytes_()
                 << ", after bytes:" << *portStatsAfter.outBytes_();
      // GE as some platforms include FCS in outBytes count
      EXPECT_EVENTUALLY_GE(
          *portStatsAfter.outBytes_() - *portStatsBefore.outBytes_(),
          pktLengthSent);
      auto portSwitchId =
          scopeResolver().scope(masterLogicalPortIds()[0]).switchId();
      auto asicType = getAsic(portSwitchId).getAsicType();
      if (asicType != cfg::AsicType::ASIC_TYPE_EBRO &&
          asicType != cfg::AsicType::ASIC_TYPE_YUBA) {
        EXPECT_EVENTUALLY_EQ(
            1,
            *portStatsAfter.outMulticastPkts_() -
                *portStatsBefore.outMulticastPkts_());
      }
    });
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentPacketSendTest, LldpToFrontPanelOutOfPortWithBufClone) {
  auto setup = [=]() {};
  auto verify = [=, this]() {
    auto portStatsBefore =
        getLatestPortStats(masterLogicalInterfacePortIds()[0]);
    auto vlanId = getVlanIDForTx();
    auto intfMac =
        utility::getMacForFirstInterfaceWithPorts(getProgrammedState());
    auto srcMac = utility::MacAddressGenerator().get(intfMac.u64NBO() + 1);
    auto payLoadSize = 256;
    auto numPkts = 20;
    std::vector<folly::IOBuf*> bufs;
    for (int i = 0; i < numPkts; i++) {
      auto txPacket = utility::makeEthTxPacket(
          getSw(),
          vlanId,
          srcMac,
          folly::MacAddress("01:80:c2:00:00:0e"),
          facebook::fboss::ETHERTYPE::ETHERTYPE_LLDP,
          std::vector<uint8_t>(payLoadSize, 0xff));
      // emulate packet buf clone in PcapPkt, which should make
      // freeTxBuf() get called after txPacket destructor
      auto buf = new folly::IOBuf();
      txPacket->buf()->cloneInto(*buf);
      bufs.push_back(buf);
      getSw()->sendPacketOutOfPortAsync(
          std::move(txPacket),
          masterLogicalInterfacePortIds()[0],
          std::nullopt);
    }
    for (auto buf : bufs) {
      delete buf;
    }
    // vlan tag should be removed
    auto pktLengthSent = (EthHdr::SIZE + payLoadSize) * numPkts;
    WITH_RETRIES({
      auto portStatsAfter =
          getLatestPortStats(masterLogicalInterfacePortIds()[0]);
      XLOG(DBG2) << "Lldp Packet:" << " before pkts:"
                 << *portStatsBefore.outMulticastPkts_()
                 << ", after pkts:" << *portStatsAfter.outMulticastPkts_()
                 << ", before bytes:" << *portStatsBefore.outBytes_()
                 << ", after bytes:" << *portStatsAfter.outBytes_();
      // GE as some platforms include FCS in outBytes count
      EXPECT_EVENTUALLY_GE(
          *portStatsAfter.outBytes_() - *portStatsBefore.outBytes_(),
          pktLengthSent);
      auto portSwitchId =
          scopeResolver().scope(masterLogicalPortIds()[0]).switchId();
      auto asicType = getAsic(portSwitchId).getAsicType();
      if (asicType != cfg::AsicType::ASIC_TYPE_EBRO &&
          asicType != cfg::AsicType::ASIC_TYPE_YUBA) {
        EXPECT_EVENTUALLY_EQ(
            numPkts,
            *portStatsAfter.outMulticastPkts_() -
                *portStatsBefore.outMulticastPkts_());
      }
    });
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentPacketSendTest, PortTxEnableTest) {
  auto setup = [this]() {
    auto config = getSw()->getConfig();
    utility::setTTLZeroCpuConfig(getAgentEnsemble()->getL3Asics(), config);
    applyNewConfig(config);
    utility::setupEcmpDataplaneLoopOnAllPorts(getAgentEnsemble());
  };
  auto verify = [this]() {
    std::vector<PortID> portsUnderTest{
        masterLogicalInterfacePortIds()[0], masterLogicalInterfacePortIds()[1]};
    auto createHighRateTraffic = [=, this]() {
      auto sendPacket = [=, this](
                            AgentEnsemble* ensemble,
                            const folly::IPAddressV6& dstIpv6Addr) {
        auto vlanId = getVlanIDForTx();
        auto intfMac =
            utility::getMacForFirstInterfaceWithPorts(getProgrammedState());
        auto srcMac = utility::MacAddressGenerator().get(intfMac.u64NBO() + 1);
        constexpr auto kPayLoadLen{1000};
        int dscpVal = 0;
        auto kECT1 = 0x01; // ECN capable transport ECT(1)
        auto txPacket = utility::makeTCPTxPacket(
            this->getSw(),
            vlanId,
            srcMac,
            intfMac,
            folly::IPAddressV6("2620:0:1cfe:face:b00c::3"),
            dstIpv6Addr,
            8001,
            8000,
            /*
             * Trailing 2 bits are for ECN, we do not want drops in
             * these queues due to WRED thresholds!
             */
            static_cast<uint8_t>(dscpVal << 2 | kECT1),
            255,
            std::vector<uint8_t>(kPayLoadLen, 0xff));

        ensemble->getSw()->sendPacketSwitchedAsync(std::move(txPacket));
      };
      utility::createTrafficOnMultiplePorts(
          getAgentEnsemble(), 2, sendPacket, 10 /* desiredPct*/
      );
      for (const auto& port : portsUnderTest) {
        // Port rate is at least 1Gbps
        constexpr uint64_t kOneGbps = (uint64_t)10 * 1024 * 1024 * 1024;
        getAgentEnsemble()->waitForSpecificRateOnPort(port, kOneGbps);
      }
    };
    createHighRateTraffic();
    // Disable TX on port
    enablePortTx(portsUnderTest[0], false);
    // wait until paused port gets paused
    auto portStatsT0 = getNextUpdatedPortStats(portsUnderTest);
    auto newStats = portStatsT0;
    WITH_RETRIES({
      auto oldStats = newStats;
      auto pausedPort = portsUnderTest[0];
      auto nonPausedPort = portsUnderTest[1];
      newStats = getNextUpdatedPortStats(portsUnderTest);
      EXPECT_EVENTUALLY_GT(
          *newStats[nonPausedPort].outUnicastPkts_(),
          *oldStats[nonPausedPort].outUnicastPkts_());
      EXPECT_EVENTUALLY_EQ(
          *newStats[pausedPort].outUnicastPkts_(),
          *oldStats[pausedPort].outUnicastPkts_());
    });

    // Enable TX on port
    enablePortTx(portsUnderTest[0], true);
    // Traffic goes back to high rate after we enable TX again
    createHighRateTraffic();
  };
  verifyAcrossWarmBoots(setup, verify);
}

class AgentPacketSendReceiveTest : public AgentHwTest, public PacketObserverIf {
 protected:
  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    return {production_features::ProductionFeature::CPU_RX_TX};
  }

  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto asic = utility::checkSameAndGetAsic(ensemble.getL3Asics());
    auto cfg = AgentHwTest::initialConfig(ensemble);
    utility::setDefaultCpuTrafficPolicyConfig(cfg, {asic}, ensemble.isSai());
    utility::addCpuQueueConfig(cfg, {asic}, ensemble.isSai());
    return cfg;
  }

  void packetReceived(const RxPacket* pkt) noexcept override {
    XLOG(DBG1) << "packet received from port " << pkt->getSrcPort();
    srcPort_ = pkt->getSrcPort();
    numPkts_++;
  }

  int getLastPktSrcPort() {
    return srcPort_;
  }

  bool verifyNumPktsReceived(int expectedNumPkts) {
    return numPkts_ == expectedNumPkts;
  }

  std::atomic<int> srcPort_{-1};
  std::atomic<int> numPkts_{0};
};

TEST_F(AgentPacketSendReceiveTest, LldpPacketReceiveSrcPort) {
  auto setup = [=]() {};
  auto verify = [=, this]() {
    getAgentEnsemble()->getSw()->getPacketObservers()->registerPacketObserver(
        this, "LldpPacketReceiveSrcPort");
    auto vlanId = getVlanIDForTx();
    auto intfMac =
        utility::getMacForFirstInterfaceWithPorts(getProgrammedState());
    auto srcMac = utility::MacAddressGenerator().get(intfMac.u64NBO() + 1);
    auto payLoadSize = 256;
    auto expectedNumPktsReceived = 1;
    for (const auto& port :
         {masterLogicalPortIds()[0], masterLogicalPortIds()[1]}) {
      auto txPacket = utility::makeEthTxPacket(
          getSw(),
          vlanId,
          srcMac,
          folly::MacAddress("01:80:c2:00:00:0e"),
          facebook::fboss::ETHERTYPE::ETHERTYPE_LLDP,
          std::vector<uint8_t>(payLoadSize, 0xff));
      getAgentEnsemble()->ensureSendPacketOutOfPort(std::move(txPacket), port);
      WITH_RETRIES_N_TIMED(5, 20ms, {
        ASSERT_EVENTUALLY_TRUE(
            verifyNumPktsReceived(expectedNumPktsReceived++));
      })
      EXPECT_EQ(port, PortID(getLastPktSrcPort()));
    }
    getAgentEnsemble()->getSw()->getPacketObservers()->unregisterPacketObserver(
        this, "LldpPacketReceiveSrcPort");
  };
  verifyAcrossWarmBoots(setup, verify);
}

class AgentPacketSendReceiveLagTest : public AgentPacketSendReceiveTest {
 protected:
  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    auto prodFeatures =
        AgentPacketSendReceiveTest::getProductionFeaturesVerified();
    prodFeatures.push_back(production_features::ProductionFeature::LAG);
    return prodFeatures;
  }
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto masterLogicalPortIds = ensemble.masterLogicalPortIds();
    auto l3Asics = ensemble.getSw()->getHwAsicTable()->getL3Asics();
    auto asic = utility::checkSameAndGetAsic(l3Asics);
    auto cfg = utility::oneL3IntfTwoPortConfig(
        ensemble.getSw()->getPlatformMapping(),
        asic,
        masterLogicalPortIds[0],
        masterLogicalPortIds[1],
        ensemble.getSw()->getPlatformSupportsAddRemovePort(),
        asic->desiredLoopbackModes());
    utility::setDefaultCpuTrafficPolicyConfig(cfg, l3Asics, ensemble.isSai());
    utility::addCpuQueueConfig(cfg, l3Asics, ensemble.isSai());

    std::vector<int32_t> ports{
        masterLogicalPortIds[0], masterLogicalPortIds[1]};
    utility::addAggPort(kAggId, ports, &cfg, cfg::LacpPortRate::SLOW);
    return cfg;
  }

  void packetReceived(const RxPacket* pkt) noexcept override {
    XLOG(DBG1) << "packet received from port " << pkt->getSrcPort()
               << " lag port " << pkt->getSrcAggregatePort();
    srcPort_ = pkt->getSrcPort();
    srcAggregatePort_ = pkt->getSrcAggregatePort();
    numPkts_++;
  }
  int getLastPktSrcAggregatePort() {
    return srcAggregatePort_;
  }
  std::atomic<int> srcAggregatePort_{-1};
};

TEST_F(AgentPacketSendReceiveLagTest, LacpPacketReceiveSrcPort) {
  auto setup = [=, this]() {
    applyNewState(
        [](const std::shared_ptr<SwitchState>& state) {
          return utility::enableTrunkPorts(state);
        },
        "enable trunk ports");
  };
  auto verify = [=, this]() {
    getAgentEnsemble()->getSw()->getPacketObservers()->registerPacketObserver(
        this, "LacpPacketReceiveSrcPort");
    auto vlanId = getVlanIDForTx();
    auto intfMac =
        utility::getMacForFirstInterfaceWithPorts(getProgrammedState());
    auto payLoadSize = 256;
    static auto payload = std::vector<uint8_t>(payLoadSize, 0xff);
    payload[0] = 0x1; // sub-version of lacp packet
    auto expectedNumPktsReceived = 1;
    for (auto port : {masterLogicalPortIds()[0], masterLogicalPortIds()[1]}) {
      // verify lacp packet properly sent and received from lag
      auto txPacket = utility::makeEthTxPacket(
          getAgentEnsemble()->getSw(),
          vlanId,
          intfMac,
          LACPDU::kSlowProtocolsDstMac(),
          facebook::fboss::ETHERTYPE::ETHERTYPE_SLOW_PROTOCOLS,
          payload);
      getAgentEnsemble()->ensureSendPacketOutOfPort(std::move(txPacket), port);
      WITH_RETRIES_N_TIMED(5, 20ms, {
        ASSERT_EVENTUALLY_TRUE(
            verifyNumPktsReceived(expectedNumPktsReceived++));
      })
      EXPECT_EQ(port, PortID(getLastPktSrcPort()));
      EXPECT_EQ(kAggId, getLastPktSrcAggregatePort());

      // verify lldp packet properly sent and received from lag
      txPacket = utility::makeEthTxPacket(
          getAgentEnsemble()->getSw(),
          vlanId,
          intfMac,
          folly::MacAddress("01:80:c2:00:00:0e"),
          facebook::fboss::ETHERTYPE::ETHERTYPE_LLDP,
          std::vector<uint8_t>(payLoadSize, 0xff));
      getAgentEnsemble()->ensureSendPacketOutOfPort(std::move(txPacket), port);
      ASSERT_TRUE(verifyNumPktsReceived(expectedNumPktsReceived++));
      EXPECT_EQ(port, PortID(getLastPktSrcPort()));
      EXPECT_EQ(kAggId, getLastPktSrcAggregatePort());
    }
    getAgentEnsemble()->getSw()->getPacketObservers()->unregisterPacketObserver(
        this, "LacpPacketReceiveSrcPort");
  };
  verifyAcrossWarmBoots(setup, verify);
}

class AgentPacketFloodTest : public AgentHwTest {
 protected:
  std::vector<PortID> getLogicalPortIDs() const {
    return {masterLogicalPortIds()[0], masterLogicalPortIds()[1]};
  }

  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto l3Asics = ensemble.getSw()->getHwAsicTable()->getL3Asics();
    auto cfg = utility::onePortPerInterfaceConfig(
        ensemble.getSw(),
        ensemble.masterLogicalPortIds(),
        true /*interfaceHasSubnet*/);
    utility::setDefaultCpuTrafficPolicyConfig(cfg, l3Asics, ensemble.isSai());
    utility::addCpuQueueConfig(cfg, l3Asics, ensemble.isSai());
    return cfg;
  }

  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    // PKTIO feature
    return {
        production_features::ProductionFeature::CPU_RX_TX,
        production_features::ProductionFeature::VLAN};
  }

  bool checkPacketFlooding(
      std::map<PortID, HwPortStats>& portStatsBefore,
      bool v6) {
    auto asic = utility::checkSameAndGetAsic(getAgentEnsemble()->getL3Asics());
    auto portStatsAfter = getLatestPortStats(masterLogicalPortIds());
    for (auto portId : getLogicalPortIDs()) {
      auto packetsBefore = v6 ? *portStatsBefore[portId].outMulticastPkts_()
                              : *portStatsBefore[portId].outBroadcastPkts_();
      auto packetsAfter = v6 ? *portStatsAfter[portId].outMulticastPkts_()
                             : *portStatsAfter[portId].outBroadcastPkts_();
      XLOG(DBG2) << "port id: " << portId << " before pkts:" << packetsBefore
                 << ", after pkts:" << packetsAfter
                 << ", before bytes:" << *portStatsBefore[portId].outBytes_()
                 << ", after bytes:" << *portStatsAfter[portId].outBytes_();
      if (*portStatsAfter[portId].outBytes_() ==
          *portStatsBefore[portId].outBytes_()) {
        return false;
      }
      if (asic->getAsicType() != cfg::AsicType::ASIC_TYPE_EBRO &&
          asic->getAsicType() != cfg::AsicType::ASIC_TYPE_YUBA) {
        if (packetsAfter <= packetsBefore) {
          return false;
        }
      }
    }
    return true;
  }
};

TEST_F(AgentPacketFloodTest, ArpRequestFloodTest) {
  auto setup = [=]() {};
  auto verify = [=, this]() {
    auto portStatsBefore = getLatestPortStats(masterLogicalPortIds());
    auto vlanId = getVlanIDForTx();
    auto intfMac =
        utility::getMacForFirstInterfaceWithPorts(getProgrammedState());
    auto srcMac = utility::MacAddressGenerator().get(intfMac.u64NBO() + 1);
    auto randomIP = folly::IPAddressV4("1.1.1.5");
    auto txPacket = utility::makeARPTxPacket(
        getSw(),
        vlanId,
        srcMac,
        folly::MacAddress("ff:ff:ff:ff:ff:ff"),
        folly::IPAddress("1.1.1.2"),
        randomIP,
        ARP_OPER::ARP_OPER_REQUEST,
        std::nullopt);
    getAgentEnsemble()->ensureSendPacketOutOfPort(
        std::move(txPacket), masterLogicalPortIds()[0]);
    EXPECT_TRUE(checkPacketFlooding(portStatsBefore, false));
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentPacketFloodTest, NdpFloodTest) {
  auto setup = [=]() {};
  auto verify = [=, this]() {
    auto retries = 5;
    auto vlanId = getVlanIDForTx();
    auto intfMac =
        utility::getMacForFirstInterfaceWithPorts(getProgrammedState());
    auto suceess = false;
    while (retries--) {
      auto portStatsBefore = getLatestPortStats(masterLogicalPortIds());
      auto txPacket = utility::makeNeighborSolicitation(
          getSw(),
          vlanId,
          intfMac,
          folly::IPAddressV6(folly::IPAddressV6::LINK_LOCAL, intfMac),
          folly::IPAddressV6("1::2"));
      getAgentEnsemble()->ensureSendPacketOutOfPort(
          std::move(txPacket), masterLogicalPortIds()[0]);
      if (checkPacketFlooding(portStatsBefore, true)) {
        suceess = true;
        break;
      }
      std::this_thread::sleep_for(1s);
      XLOG(DBG2) << " Retrying ... ";
    }
    EXPECT_TRUE(suceess);
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentSwitchedPacketSendTest, ArpRequestToFrontPanelPortSwitched) {
  auto setup = [=]() {};
  auto verify = [=, this]() {
    auto portStatsBefore =
        getLatestPortStats(masterLogicalInterfacePortIds()[0]);
    auto vlanId = getVlanIDForTx();
    auto intfMac =
        utility::getMacForFirstInterfaceWithPorts(getProgrammedState());
    auto srcMac = utility::MacAddressGenerator().get(intfMac.u64NBO() + 1);
    auto randomIP = folly::IPAddressV4("1.1.1.5");
    auto txPacket = utility::makeARPTxPacket(
        getSw(),
        vlanId,
        srcMac,
        folly::MacAddress("ff:ff:ff:ff:ff:ff"),
        folly::IPAddress("1.1.1.2"),
        randomIP,
        ARP_OPER::ARP_OPER_REQUEST,
        std::nullopt);
    getSw()->sendPacketSwitchedAsync(std::move(txPacket));
    WITH_RETRIES({
      auto portStatsAfter =
          getLatestPortStats(masterLogicalInterfacePortIds()[0]);
      XLOG(DBG2) << "ARP Packet:" << " before pkts:"
                 << *portStatsBefore.outBroadcastPkts_()
                 << ", after pkts:" << *portStatsAfter.outBroadcastPkts_()
                 << ", before bytes:" << *portStatsBefore.outBytes_()
                 << ", after bytes:" << *portStatsAfter.outBytes_();
      EXPECT_EVENTUALLY_NE(
          0, *portStatsAfter.outBytes_() - *portStatsBefore.outBytes_());
      auto portSwitchId =
          scopeResolver().scope(masterLogicalPortIds()[0]).switchId();
      auto asicType = getAsic(portSwitchId).getAsicType();
      if (asicType != cfg::AsicType::ASIC_TYPE_EBRO &&
          asicType != cfg::AsicType::ASIC_TYPE_YUBA) {
        EXPECT_EVENTUALLY_EQ(
            1,
            *portStatsAfter.outBroadcastPkts_() -
                *portStatsBefore.outBroadcastPkts_());
      }
    });
  };
  verifyAcrossWarmBoots(setup, verify);
}
} // namespace facebook::fboss
