// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <chrono>
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/utils/CoppTestUtils.h"
#include "fboss/lib/CommonUtils.h"

#include "fboss/agent/test/gen-cpp2/production_features_types.h"

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
        outPackets =
            portStats.get_outUnicastPkts_() - before.get_outUnicastPkts_();
        outDiscards = portStats.get_outDiscards_() - before.get_outDiscards_();
      }
      // Check to see if outPackets + outDiscards add up to the expected!
      XLOG(DBG3) << "Port: " << port << ", out packets: " << outPackets
                 << ", discarded packets: " << outDiscards
                 << " vs expectedOutPkts " << expectedOutPkts;
      return outPackets + outDiscards >= expectedOutPkts;
    };

    WITH_RETRIES_N_TIMED(3, std::chrono::seconds(1), {
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

TEST_F(AgentPacketSendTest, LldpToFrontPanelOutOfPort) {
  auto setup = [=]() {};
  auto verify = [=, this]() {
    auto portStatsBefore =
        getLatestPortStats(masterLogicalInterfacePortIds()[0]);
    auto vlanId = utility::firstVlanID(getProgrammedState());
    auto intfMac = utility::getFirstInterfaceMac(getProgrammedState());
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

TEST_F(AgentPacketSendTest, ArpRequestToFrontPanelPortSwitched) {
  auto setup = [=]() {};
  auto verify = [=, this]() {
    auto portStatsBefore =
        getLatestPortStats(masterLogicalInterfacePortIds()[0]);
    auto vlanId = utility::firstVlanID(getProgrammedState());
    auto intfMac = utility::getFirstInterfaceMac(getProgrammedState());
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

TEST_F(AgentPacketSendTest, PortTxEnableTest) {
  auto setup = [=]() {};
  auto verify = [=, this]() {
    constexpr auto kNumPacketsToSend{100};
    auto vlanId = utility::firstVlanID(getProgrammedState());
    auto intfMac = utility::getFirstInterfaceMac(getProgrammedState());
    auto srcMac = utility::MacAddressGenerator().get(intfMac.u64NBO() + 1);

    auto sendTcpPkts = [=, this](int numPacketsToSend) {
      int dscpVal = 0;
      for (int i = 0; i < numPacketsToSend; i++) {
        auto kECT1 = 0x01; // ECN capable transport ECT(1)
        constexpr auto kPayLoadLen{200};
        auto txPacket = utility::makeTCPTxPacket(
            this->getSw(),
            vlanId,
            srcMac,
            intfMac,
            folly::IPAddressV6("2620:0:1cfe:face:b00c::3"),
            folly::IPAddressV6("2620:0:1cfe:face:b00c::4"),
            8001,
            8000,
            /*
             * Trailing 2 bits are for ECN, we do not want drops in
             * these queues due to WRED thresholds!
             */
            static_cast<uint8_t>(dscpVal << 2 | kECT1),
            255,
            std::vector<uint8_t>(kPayLoadLen, 0xff));

        getSw()->sendPacketOutOfPortAsync(
            std::move(txPacket), masterLogicalInterfacePortIds()[0]);
      }
    };

    auto getOutPacketDelta = [](auto& after, auto& before) {
      return (
          (*after.outMulticastPkts_() + *after.outBroadcastPkts_() +
           *after.outUnicastPkts_()) -
          (*before.outMulticastPkts_() + *before.outBroadcastPkts_() +
           *before.outUnicastPkts_()));
    };

    // Disable TX on port
    enablePortTx(masterLogicalInterfacePortIds()[0], false);

    /* wait until port stats stop incrementing */
    auto portStatsT0 = getLatestPortStats(masterLogicalInterfacePortIds()[0]);
    sendTcpPkts(kNumPacketsToSend);
    auto newStats = portStatsT0;
    WITH_RETRIES_N_TIMED(10, std::chrono::milliseconds(500), {
      sendTcpPkts(1);
      auto oldStats = newStats;
      while (*newStats.timestamp_() <= *oldStats.timestamp_()) {
        newStats = getLatestPortStats(masterLogicalInterfacePortIds()[0]);
      }
      EXPECT_EVENTUALLY_TRUE(
          *newStats.outUnicastPkts_() == *oldStats.outUnicastPkts_());
    });
    // tx is fully disabled now, stats no longer increment.
    auto portStatsT1 = getLatestPortStats(masterLogicalInterfacePortIds()[0]);
    /*
     * Most platforms would allow some packets to be TXed even after TX
     * disable is set. But after the initial set of packets TX, no further
     * TX happens, verify the same.
     */
    sendTcpPkts(kNumPacketsToSend);
    auto portStatsT2 = getLatestPortStats(masterLogicalInterfacePortIds()[0]);

    // Enable TX on port, and wait for a while for packets to TX
    enablePortTx(masterLogicalInterfacePortIds()[0], true);

    /*
     * For most platforms where TX disable will not drop traffic, will have
     * the out count increment. However, there are implementations like in
     * native TH where the packets are just dropped and TH4 where there is
     * no accounting for these packets at all. Below API would wait for out
     * or drop counts to increment, if neither, return after a timeout.
     */
    waitForTxDoneOnPort(
        masterLogicalInterfacePortIds()[0], kNumPacketsToSend, portStatsT1);

    auto portStatsT3 = getLatestPortStats(masterLogicalInterfacePortIds()[0]);
    XLOG(DBG0) << "Expected number of packets to be TXed: "
               << kNumPacketsToSend * 2;
    XLOG(DBG0) << "Delta packets during test, T0:T1 -> "
               << getOutPacketDelta(portStatsT1, portStatsT0) << ", T1:T2 -> "
               << getOutPacketDelta(portStatsT2, portStatsT1) << ", T2:T3 -> "
               << getOutPacketDelta(portStatsT3, portStatsT2);

    // TX disable works if no TX is seen between T1 and T2
    EXPECT_EQ(0, getOutPacketDelta(portStatsT2, portStatsT1));
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
    auto vlanId = utility::firstVlanID(getProgrammedState());
    auto intfMac = utility::getFirstInterfaceMac(getProgrammedState());
    auto srcMac = utility::MacAddressGenerator().get(intfMac.u64NBO() + 1);
    auto payLoadSize = 256;
    auto expectedNumPktsReceived = 1;
    for (const auto& port :
         {masterLogicalPortIds()[0], masterLogicalPortIds().back()}) {
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

} // namespace facebook::fboss
