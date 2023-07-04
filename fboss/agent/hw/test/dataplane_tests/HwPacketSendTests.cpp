/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/LacpTypes.h"
#include "fboss/agent/Platform.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwLinkStateDependentTest.h"
#include "fboss/agent/hw/test/HwTestCoppUtils.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/hw/test/HwTestPortUtils.h"
#include "fboss/agent/packet/EthHdr.h"
#include "fboss/agent/test/ResourceLibUtil.h"
#include "fboss/agent/test/TrunkUtils.h"
#include "folly/Utility.h"

#include <folly/IPAddress.h>
#include <folly/container/Array.h>
#include <thread>

using namespace std::chrono_literals;

namespace {
constexpr int kAggId{500};
} // unnamed namespace

namespace facebook::fboss {

class HwPacketSendTest : public HwLinkStateDependentTest {
 protected:
  cfg::SwitchConfig initialConfig() const override {
    auto cfg = utility::onePortPerInterfaceConfig(
        getHwSwitch(),
        masterLogicalPortIds(),
        getAsic()->desiredLoopbackModes());
    return cfg;
  }
  HwSwitchEnsemble::Features featuresDesired() const override {
    return {HwSwitchEnsemble::LINKSCAN, HwSwitchEnsemble::PACKET_RX};
  }
  void waitForTxDoneOnPort(
      PortID port,
      uint64_t expectedOutPkts,
      HwPortStats& before) {
    auto waitForExpectedOutPackets = [&](const auto& newStats) {
      uint64_t outPackets{0}, outDiscards{0};
      auto portStatsIter = newStats.find(port);
      if (portStatsIter != newStats.end()) {
        auto portStats = portStatsIter->second;
        outPackets =
            portStats.get_outUnicastPkts_() - before.get_outUnicastPkts_();
        outDiscards = portStats.get_outDiscards_() - before.get_outDiscards_();
      }
      // Check to see if outPackets + outDiscards add up to the expected!
      XLOG(DBG3) << "Port: " << port << ", out packets: " << outPackets
                 << ", discarded packets: " << outDiscards;
      return outPackets + outDiscards >= expectedOutPkts;
    };

    constexpr auto kNumRetries{3};
    getHwSwitchEnsemble()->waitPortStatsCondition(
        waitForExpectedOutPackets,
        kNumRetries,
        std::chrono::milliseconds(std::chrono::seconds(1)));
  }
};

class HwPacketSendReceiveTest : public HwLinkStateDependentTest {
 protected:
  cfg::SwitchConfig initialConfig() const override {
    auto cfg = utility::oneL3IntfTwoPortConfig(
        getHwSwitch(),
        masterLogicalPortIds()[0],
        masterLogicalPortIds().back(),
        getAsic()->desiredLoopbackModes());
    utility::setDefaultCpuTrafficPolicyConfig(cfg, getAsic());
    utility::addCpuQueueConfig(cfg, getAsic());
    return cfg;
  }
  HwSwitchEnsemble::Features featuresDesired() const override {
    return {HwSwitchEnsemble::LINKSCAN, HwSwitchEnsemble::PACKET_RX};
  }
  void packetReceived(RxPacket* pkt) noexcept override {
    XLOG(DBG1) << "packet received from port " << pkt->getSrcPort();
    srcPort_ = pkt->getSrcPort();
    numPkts_++;
  }
  int getLastPktSrcPort() {
    return srcPort_;
  }
  bool verifyNumPktsReceived(int expectedNumPkts) {
    auto retries = 5;
    do {
      if (numPkts_ == expectedNumPkts) {
        return true;
      }
      std::this_thread::sleep_for(20ms);
    } while (retries--);
    return false;
  }
  std::atomic<int> srcPort_{-1};
  std::atomic<int> numPkts_{0};
};

class HwPacketSendReceiveLagTest : public HwPacketSendReceiveTest {
 protected:
  cfg::SwitchConfig initialConfig() const override {
    auto cfg = utility::oneL3IntfTwoPortConfig(
        getHwSwitch(),
        masterLogicalPortIds()[0],
        masterLogicalPortIds()[1],
        getAsic()->desiredLoopbackModes());
    utility::setDefaultCpuTrafficPolicyConfig(cfg, getAsic());
    utility::addCpuQueueConfig(cfg, getAsic());
    std::vector<int32_t> ports{
        masterLogicalPortIds()[0], masterLogicalPortIds()[1]};
    utility::addAggPort(kAggId, ports, &cfg, cfg::LacpPortRate::SLOW);
    return cfg;
  }
  void packetReceived(RxPacket* pkt) noexcept override {
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

class HwPacketFloodTest : public HwLinkStateDependentTest {
 protected:
  std::vector<PortID> getLogicalPortIDs() const {
    return {masterLogicalPortIds()[0], masterLogicalPortIds()[1]};
  }
  cfg::SwitchConfig initialConfig() const override {
    auto cfg = utility::oneL3IntfNPortConfig(
        getHwSwitch(), getLogicalPortIDs(), getAsic()->desiredLoopbackModes());
    utility::setDefaultCpuTrafficPolicyConfig(cfg, getAsic());
    utility::addCpuQueueConfig(cfg, getAsic());
    return cfg;
  }
  HwSwitchEnsemble::Features featuresDesired() const override {
    return {HwSwitchEnsemble::LINKSCAN, HwSwitchEnsemble::PACKET_RX};
  }

  bool checkPacketFlooding(
      std::map<PortID, HwPortStats>& portStatsBefore,
      bool v6) {
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
      if (getAsic()->getAsicType() != cfg::AsicType::ASIC_TYPE_EBRO) {
        if (packetsAfter <= packetsBefore) {
          return false;
        }
      }
    }
    return true;
  }
};

TEST_F(HwPacketSendTest, LldpToFrontPanelOutOfPort) {
  auto setup = [=]() {};
  auto verify = [=]() {
    auto portStatsBefore =
        getLatestPortStats(masterLogicalInterfacePortIds()[0]);
    auto vlanId = utility::firstVlanID(initialConfig());
    auto intfMac = utility::getFirstInterfaceMac(initialConfig());
    auto srcMac = utility::MacAddressGenerator().get(intfMac.u64NBO() + 1);
    auto payLoadSize = 256;
    auto txPacket = utility::makeEthTxPacket(
        getHwSwitch(),
        vlanId,
        srcMac,
        folly::MacAddress("01:80:c2:00:00:0e"),
        facebook::fboss::ETHERTYPE::ETHERTYPE_LLDP,
        std::vector<uint8_t>(payLoadSize, 0xff));
    // vlan tag should be removed
    auto pktLengthSent = EthHdr::SIZE + payLoadSize;
    getHwSwitchEnsemble()->ensureSendPacketOutOfPort(
        std::move(txPacket), masterLogicalInterfacePortIds()[0], std::nullopt);
    auto portStatsAfter =
        getLatestPortStats(masterLogicalInterfacePortIds()[0]);
    XLOG(DBG2) << "Lldp Packet:"
               << " before pkts:" << *portStatsBefore.outMulticastPkts_()
               << ", after pkts:" << *portStatsAfter.outMulticastPkts_()
               << ", before bytes:" << *portStatsBefore.outBytes_()
               << ", after bytes:" << *portStatsAfter.outBytes_();
    EXPECT_EQ(
        pktLengthSent,
        *portStatsAfter.outBytes_() - *portStatsBefore.outBytes_());
    if (getAsic()->getAsicType() != cfg::AsicType::ASIC_TYPE_EBRO) {
      EXPECT_EQ(
          1,
          *portStatsAfter.outMulticastPkts_() -
              *portStatsBefore.outMulticastPkts_());
    }
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwPacketSendTest, LldpToFrontPanelWithBufClone) {
  auto setup = [=]() {};
  auto verify = [=]() {
    auto portStatsBefore =
        getLatestPortStats(masterLogicalInterfacePortIds()[0]);
    auto vlanId = utility::firstVlanID(initialConfig());
    auto intfMac = utility::getFirstInterfaceMac(initialConfig());
    auto srcMac = utility::MacAddressGenerator().get(intfMac.u64NBO() + 1);
    auto payLoadSize = 256;
    auto numPkts = 20;
    std::vector<folly::IOBuf*> bufs;
    for (int i = 0; i < numPkts; i++) {
      auto txPacket = utility::makeEthTxPacket(
          getHwSwitch(),
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
      getHwSwitchEnsemble()->ensureSendPacketOutOfPort(
          std::move(txPacket),
          masterLogicalInterfacePortIds()[0],
          std::nullopt);
    }
    for (auto buf : bufs) {
      delete buf;
    }
    auto portStatsAfter =
        getLatestPortStats(masterLogicalInterfacePortIds()[0]);
    XLOG(DBG2) << "Lldp Packet:"
               << " before pkts:" << *portStatsBefore.outMulticastPkts_()
               << ", after pkts:" << *portStatsAfter.outMulticastPkts_()
               << ", before bytes:" << *portStatsBefore.outBytes_()
               << ", after bytes:" << *portStatsAfter.outBytes_();
    auto pktLengthSent = (EthHdr::SIZE + payLoadSize) * numPkts;
    EXPECT_EQ(
        pktLengthSent,
        *portStatsAfter.outBytes_() - *portStatsBefore.outBytes_());
    if (getAsic()->getAsicType() != cfg::AsicType::ASIC_TYPE_EBRO) {
      EXPECT_EQ(
          numPkts,
          *portStatsAfter.outMulticastPkts_() -
              *portStatsBefore.outMulticastPkts_());
    }
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwPacketSendTest, ArpRequestToFrontPanelPortSwitched) {
  auto setup = [=]() {};
  auto verify = [=]() {
    auto portStatsBefore =
        getLatestPortStats(masterLogicalInterfacePortIds()[0]);
    auto vlanId = utility::firstVlanID(initialConfig());
    auto intfMac = utility::getFirstInterfaceMac(initialConfig());
    auto srcMac = utility::MacAddressGenerator().get(intfMac.u64NBO() + 1);
    auto randomIP = folly::IPAddressV4("1.1.1.5");
    auto txPacket = utility::makeARPTxPacket(
        getHwSwitch(),
        vlanId,
        srcMac,
        folly::MacAddress("ff:ff:ff:ff:ff:ff"),
        folly::IPAddress("1.1.1.2"),
        randomIP,
        ARP_OPER::ARP_OPER_REQUEST,
        std::nullopt);
    getHwSwitchEnsemble()->ensureSendPacketSwitched(std::move(txPacket));
    auto portStatsAfter =
        getLatestPortStats(masterLogicalInterfacePortIds()[0]);
    XLOG(DBG2) << "ARP Packet:"
               << " before pkts:" << *portStatsBefore.outBroadcastPkts_()
               << ", after pkts:" << *portStatsAfter.outBroadcastPkts_()
               << ", before bytes:" << *portStatsBefore.outBytes_()
               << ", after bytes:" << *portStatsAfter.outBytes_();
    EXPECT_NE(0, *portStatsAfter.outBytes_() - *portStatsBefore.outBytes_());
    if (getAsic()->getAsicType() != cfg::AsicType::ASIC_TYPE_EBRO) {
      EXPECT_EQ(
          1,
          *portStatsAfter.outBroadcastPkts_() -
              *portStatsBefore.outBroadcastPkts_());
    }
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwPacketSendTest, PortTxEnableTest) {
  auto setup = [=]() {};
  auto verify = [=]() {
    constexpr auto kNumPacketsToSend{100};
    auto vlanId = utility::firstVlanID(initialConfig());
    auto intfMac = utility::getFirstInterfaceMac(initialConfig());
    auto srcMac = utility::MacAddressGenerator().get(intfMac.u64NBO() + 1);

    auto sendTcpPkts = [=](int numPacketsToSend) {
      int dscpVal = 0;
      for (int i = 0; i < numPacketsToSend; i++) {
        auto kECT1 = 0x01; // ECN capable transport ECT(1)
        constexpr auto kPayLoadLen{200};
        auto txPacket = utility::makeTCPTxPacket(
            getHwSwitch(),
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
        getHwSwitch()->sendPacketOutOfPortSync(
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
    utility::setCreditWatchdogAndPortTx(
        getHwSwitch(), masterLogicalInterfacePortIds()[0], false);

    auto portStatsT0 = getLatestPortStats(masterLogicalInterfacePortIds()[0]);
    sendTcpPkts(kNumPacketsToSend);
    // We don't know how many packets will get out, wait for atleast 1.
    waitForTxDoneOnPort(masterLogicalInterfacePortIds()[0], 1, portStatsT0);

    auto portStatsT1 = getLatestPortStats(masterLogicalInterfacePortIds()[0]);
    /*
     * Most platforms would allow some packets to be TXed even after TX
     * disable is set. But after the initial set of packets TX, no further
     * TX happens, verify the same.
     */
    sendTcpPkts(kNumPacketsToSend);
    auto portStatsT2 = getLatestPortStats(masterLogicalInterfacePortIds()[0]);

    // Enable TX on port, and wait for a while for packets to TX
    utility::setCreditWatchdogAndPortTx(
        getHwSwitch(), masterLogicalInterfacePortIds()[0], true);
    /*
     * For most platforms where TX disable will not drop traffic, will have
     * the out count increment. However, there are implementations like in
     * native TH where the packets are just dropped and TH4 where there is
     * no accounting for these packets at all. Below API would wait for out
     * or drop counts to increment, if neither, return after a timeout.
     */
    waitForTxDoneOnPort(
        masterLogicalInterfacePortIds()[0], kNumPacketsToSend * 2, portStatsT0);

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

TEST_F(HwPacketSendReceiveTest, LldpPacketReceiveSrcPort) {
  auto setup = [=]() {};
  auto verify = [=]() {
    if (!isSupported(HwAsic::Feature::PKTIO)) {
      return;
    }
    auto vlanId = VlanID(*initialConfig().vlanPorts()[0].vlanID());
    auto intfMac = utility::getInterfaceMac(getProgrammedState(), vlanId);
    auto srcMac = utility::MacAddressGenerator().get(intfMac.u64NBO() + 1);
    auto payLoadSize = 256;
    auto expectedNumPktsReceived = 1;
    for (auto port :
         {masterLogicalPortIds()[0], masterLogicalPortIds().back()}) {
      auto txPacket = utility::makeEthTxPacket(
          getHwSwitch(),
          vlanId,
          srcMac,
          folly::MacAddress("01:80:c2:00:00:0e"),
          facebook::fboss::ETHERTYPE::ETHERTYPE_LLDP,
          std::vector<uint8_t>(payLoadSize, 0xff));
      getHwSwitchEnsemble()->ensureSendPacketOutOfPort(
          std::move(txPacket), port, std::nullopt);
      ASSERT_TRUE(verifyNumPktsReceived(expectedNumPktsReceived++));
      EXPECT_EQ(port, PortID(getLastPktSrcPort()));
    }
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwPacketSendReceiveLagTest, LacpPacketReceiveSrcPort) {
  auto setup = [=]() {
    applyNewState(utility::enableTrunkPorts(getProgrammedState()));
  };
  auto verify = [=]() {
    auto vlanId = VlanID(*initialConfig().vlanPorts()[0].vlanID());
    auto intfMac = utility::getInterfaceMac(getProgrammedState(), vlanId);
    auto payLoadSize = 256;
    static auto payload = std::vector<uint8_t>(payLoadSize, 0xff);
    payload[0] = 0x1; // sub-version of lacp packet
    auto expectedNumPktsReceived = 1;
    for (auto port : {masterLogicalPortIds()[0], masterLogicalPortIds()[1]}) {
      // verify lacp packet properly sent and received from lag
      auto txPacket = utility::makeEthTxPacket(
          getHwSwitch(),
          vlanId,
          intfMac,
          LACPDU::kSlowProtocolsDstMac(),
          facebook::fboss::ETHERTYPE::ETHERTYPE_SLOW_PROTOCOLS,
          payload);
      getHwSwitchEnsemble()->ensureSendPacketOutOfPort(
          std::move(txPacket), port, std::nullopt);
      ASSERT_TRUE(verifyNumPktsReceived(expectedNumPktsReceived++));
      EXPECT_EQ(port, PortID(getLastPktSrcPort()));
      EXPECT_EQ(kAggId, getLastPktSrcAggregatePort());

      // verify lldp packet properly sent and received from lag
      txPacket = utility::makeEthTxPacket(
          getHwSwitch(),
          vlanId,
          intfMac,
          folly::MacAddress("01:80:c2:00:00:0e"),
          facebook::fboss::ETHERTYPE::ETHERTYPE_LLDP,
          std::vector<uint8_t>(payLoadSize, 0xff));
      getHwSwitchEnsemble()->ensureSendPacketOutOfPort(
          std::move(txPacket), port, std::nullopt);
      ASSERT_TRUE(verifyNumPktsReceived(expectedNumPktsReceived++));
      EXPECT_EQ(port, PortID(getLastPktSrcPort()));
      EXPECT_EQ(kAggId, getLastPktSrcAggregatePort());
    }
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwPacketFloodTest, ArpRequestFloodTest) {
  auto setup = [=]() {};
  auto verify = [=]() {
    auto portStatsBefore = getLatestPortStats(masterLogicalPortIds());
    auto vlanId = VlanID(*initialConfig().vlanPorts()[0].vlanID());
    auto intfMac = utility::getInterfaceMac(getProgrammedState(), vlanId);
    auto srcMac = utility::MacAddressGenerator().get(intfMac.u64NBO() + 1);
    auto randomIP = folly::IPAddressV4("1.1.1.5");
    auto txPacket = utility::makeARPTxPacket(
        getHwSwitch(),
        vlanId,
        srcMac,
        folly::MacAddress("ff:ff:ff:ff:ff:ff"),
        folly::IPAddress("1.1.1.2"),
        randomIP,
        ARP_OPER::ARP_OPER_REQUEST,
        std::nullopt);
    getHwSwitchEnsemble()->ensureSendPacketOutOfPort(
        std::move(txPacket), masterLogicalPortIds()[0], std::nullopt);
    EXPECT_TRUE(checkPacketFlooding(portStatsBefore, false));
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwPacketFloodTest, NdpFloodTest) {
  auto setup = [=]() {};
  auto verify = [=]() {
    auto retries = 5;
    auto vlanId = VlanID(*initialConfig().vlanPorts()[0].vlanID());
    auto intfMac = utility::getInterfaceMac(getProgrammedState(), vlanId);
    auto suceess = false;
    while (retries--) {
      auto portStatsBefore = getLatestPortStats(masterLogicalPortIds());
      auto txPacket = utility::makeNeighborSolicitation(
          getHwSwitch(),
          vlanId,
          intfMac,
          folly::IPAddressV6(folly::IPAddressV6::LINK_LOCAL, intfMac),
          folly::IPAddressV6("1::2"));
      getHwSwitchEnsemble()->ensureSendPacketOutOfPort(
          std::move(txPacket), masterLogicalPortIds()[0], std::nullopt);
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

} // namespace facebook::fboss
