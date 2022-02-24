/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/Platform.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwLinkStateDependentTest.h"
#include "fboss/agent/hw/test/HwTestCoppUtils.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/hw/test/HwTestPortUtils.h"
#include "fboss/agent/packet/EthHdr.h"
#include "fboss/agent/test/ResourceLibUtil.h"
#include "folly/Utility.h"

#include <folly/IPAddress.h>
#include <folly/container/Array.h>
#include <thread>

using namespace std::chrono_literals;

namespace facebook::fboss {

class HwPacketSendTest : public HwLinkStateDependentTest {
 protected:
  cfg::SwitchConfig initialConfig() const override {
    auto cfg = utility::oneL3IntfConfig(
        getHwSwitch(), masterLogicalPortIds()[0], cfg::PortLoopbackMode::MAC);
    utility::setDefaultCpuTrafficPolicyConfig(cfg, getAsic());
    utility::addCpuQueueConfig(cfg, getAsic());
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
        cfg::PortLoopbackMode::MAC);
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

class HwPacketFloodTest : public HwLinkStateDependentTest {
 protected:
  std::vector<PortID> getLogicalPortIDs() const {
    return {masterLogicalPortIds()[0], masterLogicalPortIds()[1]};
  }
  cfg::SwitchConfig initialConfig() const override {
    auto cfg = utility::oneL3IntfNPortConfig(
        getHwSwitch(), getLogicalPortIDs(), cfg::PortLoopbackMode::MAC);
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
      auto packetsBefore = v6
          ? *portStatsBefore[portId].outMulticastPkts__ref()
          : *portStatsBefore[portId].outBroadcastPkts__ref();
      auto packetsAfter = v6 ? *portStatsAfter[portId].outMulticastPkts__ref()
                             : *portStatsAfter[portId].outBroadcastPkts__ref();
      XLOG(INFO) << "port id: " << portId << " before pkts:" << packetsBefore
                 << ", after pkts:" << packetsAfter << ", before bytes:"
                 << *portStatsBefore[portId].outBytes__ref()
                 << ", after bytes:" << *portStatsAfter[portId].outBytes__ref();
      if (*portStatsAfter[portId].outBytes__ref() ==
          *portStatsBefore[portId].outBytes__ref()) {
        return false;
      }
      if (getAsic()->getAsicType() != HwAsic::AsicType::ASIC_TYPE_TAJO) {
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
    auto portStatsBefore = getLatestPortStats(masterLogicalPortIds()[0]);
    auto vlanId = VlanID(*initialConfig().vlanPorts_ref()[0].vlanID_ref());
    auto intfMac = utility::getInterfaceMac(getProgrammedState(), vlanId);
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
        std::move(txPacket), masterLogicalPortIds()[0], std::nullopt);
    auto portStatsAfter = getLatestPortStats(masterLogicalPortIds()[0]);
    XLOG(INFO) << "Lldp Packet:"
               << " before pkts:" << *portStatsBefore.outMulticastPkts__ref()
               << ", after pkts:" << *portStatsAfter.outMulticastPkts__ref()
               << ", before bytes:" << *portStatsBefore.outBytes__ref()
               << ", after bytes:" << *portStatsAfter.outBytes__ref();
    EXPECT_EQ(
        pktLengthSent,
        *portStatsAfter.outBytes__ref() - *portStatsBefore.outBytes__ref());
    if (getAsic()->getAsicType() != HwAsic::AsicType::ASIC_TYPE_TAJO) {
      EXPECT_EQ(
          1,
          *portStatsAfter.outMulticastPkts__ref() -
              *portStatsBefore.outMulticastPkts__ref());
    }
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwPacketSendTest, LldpToFrontPanelWithBufClone) {
  auto setup = [=]() {};
  auto verify = [=]() {
    auto portStatsBefore = getLatestPortStats(masterLogicalPortIds()[0]);
    auto vlanId = VlanID(*initialConfig().vlanPorts_ref()[0].vlanID_ref());
    auto intfMac = utility::getInterfaceMac(getProgrammedState(), vlanId);
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
          std::move(txPacket), masterLogicalPortIds()[0], std::nullopt);
    }
    for (auto buf : bufs) {
      delete buf;
    }
    auto portStatsAfter = getLatestPortStats(masterLogicalPortIds()[0]);
    XLOG(INFO) << "Lldp Packet:"
               << " before pkts:" << *portStatsBefore.outMulticastPkts__ref()
               << ", after pkts:" << *portStatsAfter.outMulticastPkts__ref()
               << ", before bytes:" << *portStatsBefore.outBytes__ref()
               << ", after bytes:" << *portStatsAfter.outBytes__ref();
    auto pktLengthSent = (EthHdr::SIZE + payLoadSize) * numPkts;
    EXPECT_EQ(
        pktLengthSent,
        *portStatsAfter.outBytes__ref() - *portStatsBefore.outBytes__ref());
    if (getAsic()->getAsicType() != HwAsic::AsicType::ASIC_TYPE_TAJO) {
      EXPECT_EQ(
          numPkts,
          *portStatsAfter.outMulticastPkts__ref() -
              *portStatsBefore.outMulticastPkts__ref());
    }
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwPacketSendTest, ArpRequestToFrontPanelPortSwitched) {
  auto setup = [=]() {};
  auto verify = [=]() {
    auto portStatsBefore = getLatestPortStats(masterLogicalPortIds()[0]);
    auto vlanId = VlanID(*initialConfig().vlanPorts_ref()[0].vlanID_ref());
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
    getHwSwitchEnsemble()->ensureSendPacketSwitched(std::move(txPacket));
    auto portStatsAfter = getLatestPortStats(masterLogicalPortIds()[0]);
    XLOG(INFO) << "ARP Packet:"
               << " before pkts:" << *portStatsBefore.outBroadcastPkts__ref()
               << ", after pkts:" << *portStatsAfter.outBroadcastPkts__ref()
               << ", before bytes:" << *portStatsBefore.outBytes__ref()
               << ", after bytes:" << *portStatsAfter.outBytes__ref();
    EXPECT_NE(
        0, *portStatsAfter.outBytes__ref() - *portStatsBefore.outBytes__ref());
    if (getAsic()->getAsicType() != HwAsic::AsicType::ASIC_TYPE_TAJO) {
      EXPECT_EQ(
          1,
          *portStatsAfter.outBroadcastPkts__ref() -
              *portStatsBefore.outBroadcastPkts__ref());
    }
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwPacketSendTest, PortTxEnableTest) {
  auto setup = [=]() {};
  auto verify = [=]() {
    constexpr auto kNumPacketsToSend{100};
    auto vlanId = VlanID(*initialConfig().vlanPorts_ref()[0].vlanID_ref());
    auto intfMac = utility::getInterfaceMac(getProgrammedState(), vlanId);
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
            std::move(txPacket), masterLogicalPortIds()[0]);
      }
    };

    auto getOutPacketDelta = [](auto& after, auto& before) {
      return (
          (*after.outMulticastPkts__ref() + *after.outBroadcastPkts__ref() +
           *after.outUnicastPkts__ref()) -
          (*before.outMulticastPkts__ref() + *before.outBroadcastPkts__ref() +
           *before.outUnicastPkts__ref()));
    };

    // Disable TX on port
    utility::setPortTxEnable(getHwSwitch(), masterLogicalPortIds()[0], false);

    auto portStatsT0 = getLatestPortStats(masterLogicalPortIds()[0]);
    sendTcpPkts(kNumPacketsToSend);
    // We don't know how many packets will get out, wait for atleast 1.
    waitForTxDoneOnPort(masterLogicalPortIds()[0], 1, portStatsT0);

    auto portStatsT1 = getLatestPortStats(masterLogicalPortIds()[0]);
    /*
     * Most platforms would allow some packets to be TXed even after TX
     * disable is set. But after the initial set of packets TX, no further
     * TX happens, verify the same.
     */
    sendTcpPkts(kNumPacketsToSend);
    auto portStatsT2 = getLatestPortStats(masterLogicalPortIds()[0]);

    // Enable TX on port, and wait for a while for packets to TX
    utility::setPortTxEnable(getHwSwitch(), masterLogicalPortIds()[0], true);
    /*
     * For most platforms where TX disable will not drop traffic, will have
     * the out count increment. However, there are implementations like in
     * native TH where the packets are just dropped and TH4 where there is
     * no accounting for these packets at all. Below API would wait for out
     * or drop counts to increment, if neither, return after a timeout.
     */
    waitForTxDoneOnPort(
        masterLogicalPortIds()[0], kNumPacketsToSend * 2, portStatsT0);

    auto portStatsT3 = getLatestPortStats(masterLogicalPortIds()[0]);
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
    auto vlanId = VlanID(*initialConfig().vlanPorts_ref()[0].vlanID_ref());
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

TEST_F(HwPacketFloodTest, ArpRequestFloodTest) {
  auto setup = [=]() {};
  auto verify = [=]() {
    auto portStatsBefore = getLatestPortStats(masterLogicalPortIds());
    auto vlanId = VlanID(*initialConfig().vlanPorts_ref()[0].vlanID_ref());
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
    auto vlanId = VlanID(*initialConfig().vlanPorts_ref()[0].vlanID_ref());
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
      XLOG(INFO) << " Retrying ... ";
    }
    EXPECT_TRUE(suceess);
  };
  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
