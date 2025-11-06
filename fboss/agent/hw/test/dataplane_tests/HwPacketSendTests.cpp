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
#include "fboss/agent/test/ResourceLibUtil.h"
#include "fboss/agent/test/TrunkUtils.h"

#include <folly/IPAddress.h>
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
        outPackets = portStats.get_outUnicastPkts_() -
            folly::copy(before.outUnicastPkts_().value());
        outDiscards = portStats.get_outDiscards_() -
            folly::copy(before.outDiscards_().value());
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
    auto cfg = utility::onePortPerInterfaceConfig(
        getHwSwitch(), masterLogicalPortIds());
    utility::setDefaultCpuTrafficPolicyConfig(
        cfg,
        getHwSwitchEnsemble()->getL3Asics(),
        getHwSwitchEnsemble()->isSai());
    utility::addCpuQueueConfig(
        cfg,
        getHwSwitchEnsemble()->getL3Asics(),
        getHwSwitchEnsemble()->isSai());
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
        getHwSwitch()->getPlatform()->getPlatformMapping(),
        getHwSwitch()->getPlatform()->getAsic(),
        masterLogicalPortIds()[0],
        masterLogicalPortIds()[1],
        getHwSwitch()->getPlatform()->supportsAddRemovePort(),
        getAsic()->desiredLoopbackModes());
    utility::setDefaultCpuTrafficPolicyConfig(
        cfg,
        getHwSwitchEnsemble()->getL3Asics(),
        getHwSwitchEnsemble()->isSai());
    utility::addCpuQueueConfig(
        cfg,
        getHwSwitchEnsemble()->getL3Asics(),
        getHwSwitchEnsemble()->isSai());
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
        getHwSwitch()->getPlatform()->getPlatformMapping(),
        getHwSwitch()->getPlatform()->getAsic(),
        getLogicalPortIDs(),
        getHwSwitch()->getPlatform()->supportsAddRemovePort(),
        getAsic()->desiredLoopbackModes());
    utility::setDefaultCpuTrafficPolicyConfig(
        cfg,
        getHwSwitchEnsemble()->getL3Asics(),
        getHwSwitchEnsemble()->isSai());
    utility::addCpuQueueConfig(
        cfg,
        getHwSwitchEnsemble()->getL3Asics(),
        getHwSwitchEnsemble()->isSai());
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
      if (getAsic()->getAsicType() != cfg::AsicType::ASIC_TYPE_EBRO &&
          getAsic()->getAsicType() != cfg::AsicType::ASIC_TYPE_YUBA) {
        if (packetsAfter <= packetsBefore) {
          return false;
        }
      }
    }
    return true;
  }
};

TEST_F(HwPacketSendReceiveLagTest, LacpPacketReceiveSrcPort) {
  auto setup = [=, this]() {
    applyNewState(utility::enableTrunkPorts(getProgrammedState()));
  };
  auto verify = [=, this]() {
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
  auto verify = [=, this]() {
    auto portStatsBefore = getLatestPortStats(masterLogicalPortIds());
    auto vlanId = VlanID(*initialConfig().vlanPorts()[0].vlanID());
    auto intfMac = utility::getInterfaceMac(getProgrammedState(), vlanId);
    auto srcMac = utility::MacAddressGenerator().get(intfMac.u64HBO() + 1);
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
  auto verify = [=, this]() {
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
