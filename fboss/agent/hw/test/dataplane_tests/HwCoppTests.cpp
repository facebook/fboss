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
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/ResourceLibUtil.h"
#include "folly/Utility.h"

#include <folly/IPAddress.h>
#include <folly/container/Array.h>

#include <gtest/gtest.h>

namespace {
constexpr auto kGetQueueOutPktsRetryTimes = 5;
/**
 * Link-local multicast network
 */
const auto kIPv6LinkLocalMcastAbsoluteAddress = folly::IPAddressV6("ff02::0");
const auto kIPv6LinkLocalMcastAddress = folly::IPAddressV6("ff02::5");
// Link-local unicast network
const auto kIPv6LinkLocalUcastAddress = folly::IPAddressV6("fe80::2");
constexpr uint8_t kNetworkControlDscp = 48;

const auto kMcastMacAddress = folly::MacAddress("01:05:0E:01:02:03");

static time_t getCurrentTime() {
  return std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
}

using TestTypes = ::testing::Types<facebook::fboss::PortID>;
} // unnamed namespace

namespace facebook::fboss {

template <typename TestType>
class HwCoppTest : public HwLinkStateDependentTest {
 protected:
  cfg::SwitchConfig initialConfig() const override {
    auto cfg = utility::oneL3IntfConfig(
        getHwSwitch(), masterLogicalPortIds()[0], cfg::PortLoopbackMode::MAC);
    utility::setDefaultCpuTrafficPolicyConfig(cfg, getAsic());
    utility::addCpuQueueConfig(cfg, getAsic());
    return cfg;
  }

  void sendUdpPkts(
      int numPktsToSend,
      const folly::IPAddress& dstIpAddress,
      int l4SrcPort,
      int l4DstPort) {
    auto vlanId = VlanID(*initialConfig().vlanPorts_ref()[0].vlanID_ref());
    auto intfMac = utility::getInterfaceMac(getProgrammedState(), vlanId);
    // arbit
    const auto srcIp =
        folly::IPAddress(dstIpAddress.isV4() ? "1.1.1.2" : "1::10");
    auto srcMac = utility::MacAddressGenerator().get(intfMac.u64NBO() + 1);
    for (int i = 0; i < numPktsToSend; i++) {
      auto txPacket = utility::makeUDPTxPacket(
          getHwSwitch(),
          vlanId,
          srcMac,
          intfMac,
          srcIp,
          dstIpAddress,
          l4SrcPort,
          l4DstPort);
      getHwSwitch()->sendPacketSwitchedSync(std::move(txPacket));
    }
  }

  void sendEthPkts(
      int numPktsToSend,
      facebook::fboss::ETHERTYPE etherType,
      const std::optional<folly::MacAddress>& dstMac = std::nullopt,
      std::optional<std::vector<uint8_t>> payload = std::nullopt) {
    auto vlanId = VlanID(*initialConfig().vlanPorts_ref()[0].vlanID_ref());
    auto intfMac = utility::getInterfaceMac(getProgrammedState(), vlanId);
    for (int i = 0; i < numPktsToSend; i++) {
      auto txPacket = utility::makeEthTxPacket(
          getHwSwitch(),
          vlanId,
          intfMac,
          dstMac ? *dstMac : intfMac,
          etherType,
          payload);
      getHwSwitch()->sendPacketOutOfPortSync(
          std::move(txPacket), PortID(masterLogicalPortIds()[0]));
    }
  }

  void sendArpPkts(
      int numPktsToSend,
      const folly::IPAddress& dstIpAddress,
      ARP_OPER arpType,
      bool outOfPort) {
    auto vlanId = VlanID(*initialConfig().vlanPorts_ref()[0].vlanID_ref());
    auto intfMac = utility::getInterfaceMac(getProgrammedState(), vlanId);
    auto srcMac = utility::MacAddressGenerator().get(intfMac.u64NBO() + 1);
    for (int i = 0; i < numPktsToSend; i++) {
      auto txPacket = utility::makeARPTxPacket(
          getHwSwitch(),
          vlanId,
          srcMac,
          arpType == ARP_OPER::ARP_OPER_REQUEST
              ? folly::MacAddress("ff:ff:ff:ff:ff:ff")
              : intfMac,
          folly::IPAddress("1.1.1.2"),
          dstIpAddress,
          arpType);
      if (outOfPort) {
        getHwSwitch()->sendPacketOutOfPortSync(
            std::move(txPacket), PortID(masterLogicalPortIds()[0]));
      } else {
        getHwSwitch()->sendPacketSwitchedSync(std::move(txPacket));
      }
    }
  }

  uint64_t getQueueOutPacketsWithRetry(
      int queueId,
      int retryTimes,
      uint64_t expectedNumPkts) {
    uint64_t outPkts = 0, outBytes = 0;
    do {
      std::tie(outPkts, outBytes) =
          utility::getCpuQueueOutPacketsAndBytes(getHwSwitch(), queueId);
      if (retryTimes == 0 || outPkts == expectedNumPkts) {
        break;
      }

      /*
       * Post warmboot, the packet always gets processed by the right CPU
       * queue (as per ACL/rxreason etc.) but sometimes it is delayed.
       * Retrying a few times to avoid test noise.
       */
      XLOG(DBG0) << "Retry...";
      /* sleep override */
      sleep(1);
    } while (retryTimes-- > 0);

    return outPkts;
  }

  void sendUdpPktAndVerify(
      int queueId,
      const folly::IPAddress& dstIpAddress,
      const int l4SrcPort,
      const int l4DstPort,
      const int numPktsToSend = 1,
      const int expectedPktDelta = 0) {
    auto beforeOutPkts = getQueueOutPacketsWithRetry(
        queueId, 0 /* retryTimes */, 0 /* expectedNumPkts */);
    sendUdpPkts(numPktsToSend, dstIpAddress, l4SrcPort, l4DstPort);
    auto afterOutPkts = getQueueOutPacketsWithRetry(
        queueId, kGetQueueOutPktsRetryTimes, beforeOutPkts + 1);
    XLOG(DBG0) << "Queue=" << queueId << ", before pkts:" << beforeOutPkts
               << ", after pkts:" << afterOutPkts;
    EXPECT_EQ(expectedPktDelta, afterOutPkts - beforeOutPkts);
  }

  void sendTcpPkts(
      int numPktsToSend,
      const folly::IPAddress& dstIpAddress,
      int l4SrcPort,
      int l4DstPort,
      const std::optional<folly::MacAddress>& dstMac = std::nullopt,
      uint8_t trafficClass = 0,
      std::optional<std::vector<uint8_t>> payload = std::nullopt) {
    auto vlanId = VlanID(*initialConfig().vlanPorts_ref()[0].vlanID_ref());
    auto intfMac = utility::getInterfaceMac(getProgrammedState(), vlanId);
    utility::sendTcpPkts(
        getHwSwitch(),
        numPktsToSend,
        vlanId,
        dstMac ? *dstMac : intfMac,
        dstIpAddress,
        l4SrcPort,
        l4DstPort,
        masterLogicalPortIds()[0],
        trafficClass,
        payload);
  }

  void sendPktAndVerifyCpuQueue(
      int queueId,
      const folly::IPAddress& dstIpAddress,
      const int l4SrcPort,
      const int l4DstPort,
      const std::optional<folly::MacAddress>& dstMac = std::nullopt,
      uint8_t trafficClass = 0,
      const int numPktsToSend = 1,
      const int expectedPktDelta = 1,
      std::optional<std::vector<uint8_t>> payload = std::nullopt) {
    auto sendPkts = [=]() {
      sendTcpPkts(
          numPktsToSend,
          dstIpAddress,
          l4SrcPort,
          l4DstPort,
          dstMac,
          trafficClass,
          payload);
    };
    utility::sendPktAndVerifyCpuQueue(
        getHwSwitch(), queueId, sendPkts, expectedPktDelta);
  }

  void sendPktAndVerifyEthPacketsCpuQueue(
      int queueId,
      facebook::fboss::ETHERTYPE etherType,
      const std::optional<folly::MacAddress>& dstMac = std::nullopt,
      const int numPktsToSend = 1,
      const int expectedPktDelta = 1) {
    auto beforeOutPkts = getQueueOutPacketsWithRetry(
        queueId, 0 /* retryTimes */, 0 /* expectedNumPkts */);
    static auto payload = std::vector<uint8_t>(256, 0xff);
    payload[0] = 0x1; // sub-version of lacp packet
    sendEthPkts(numPktsToSend, etherType, dstMac, payload);
    auto afterOutPkts = getQueueOutPacketsWithRetry(
        queueId, kGetQueueOutPktsRetryTimes, beforeOutPkts + 1);
    XLOG(DBG0) << "Packet of dstMac="
               << (dstMac ? (*dstMac).toString()
                          : getPlatform()->getLocalMac().toString())
               << ". Ethertype=" << std::hex << int(etherType)
               << ". Queue=" << queueId << ", before pkts:" << beforeOutPkts
               << ", after pkts:" << afterOutPkts;
    EXPECT_EQ(expectedPktDelta, afterOutPkts - beforeOutPkts);
  }

  void sendPktAndVerifyArpPacketsCpuQueue(
      int queueId,
      const folly::IPAddress& dstIpAddress,
      ARP_OPER arpType,
      bool outOfPort = true,
      const int numPktsToSend = 1,
      const int expectedPktDelta = 1) {
    auto beforeOutPkts = getQueueOutPacketsWithRetry(
        queueId, 0 /* retryTimes */, 0 /* expectedNumPkts */);
    sendArpPkts(numPktsToSend, dstIpAddress, arpType, outOfPort);
    auto afterOutPkts = getQueueOutPacketsWithRetry(
        queueId, kGetQueueOutPktsRetryTimes, beforeOutPkts + 1);
    XLOG(DBG0) << "Packet of DstIp=" << dstIpAddress.str() << ", dstMac="
               << ". Queue=" << queueId << ", before pkts:" << beforeOutPkts
               << ", after pkts:" << afterOutPkts;
    EXPECT_EQ(expectedPktDelta, afterOutPkts - beforeOutPkts);
  }

  folly::IPAddress getInSubnetNonSwitchIP() const {
    auto configIntf = initialConfig().interfaces_ref()[0];
    auto ipAddress = configIntf.ipAddresses_ref()[0];
    return folly::IPAddress::createNetwork(ipAddress, -1, true).first;
  }

 private:
  HwSwitchEnsemble::Features featuresDesired() const override {
    return {HwSwitchEnsemble::LINKSCAN, HwSwitchEnsemble::PACKET_RX};
  }
};

TYPED_TEST_SUITE(HwCoppTest, TestTypes);

TYPED_TEST(HwCoppTest, VerifyCoppPpsLowPri) {
  auto setup = [=]() {
    // COPP is part of initial config already
  };

  auto verify = [=]() {
    auto kNumPktsToSend = 60000;
    auto kMinDurationInSecs = 12;
    const double kVariance = 0.30; // i.e. + or -30%

    auto beforeSecs = getCurrentTime();
    auto beforeOutPkts = this->getQueueOutPacketsWithRetry(
        utility::kCoppLowPriQueueId,
        0 /* retryTimes */,
        0 /* expectedNumPkts */);

    auto dstIP = this->getInSubnetNonSwitchIP();
    uint64_t afterSecs;
    /*
     * To avoid noise, the send traffic for at least kMinDurationInSecs.
     * In practice, sending kNumPktsToSend packets takes longer than
     * kMinDurationInSecs. But to be safe, keep sending packets for
     * at least kMinDurationInSecs.
     */
    do {
      this->sendTcpPkts(
          kNumPktsToSend,
          dstIP,
          utility::kNonSpecialPort1,
          utility::kNonSpecialPort2);
      afterSecs = getCurrentTime();
    } while (afterSecs - beforeSecs < kMinDurationInSecs);

    auto afterOutPkts = this->getQueueOutPacketsWithRetry(
        utility::kCoppLowPriQueueId,
        0 /* retryTimes */,
        0 /* expectedNumPkts */);
    auto totalRecvdPkts = afterOutPkts - beforeOutPkts;
    auto duration = afterSecs - beforeSecs;
    auto currPktsPerSec = totalRecvdPkts / duration;
    auto lowPktsPerSec = utility::kCoppLowPriPktsPerSec * (1 - kVariance);
    auto highPktsPerSec = utility::kCoppLowPriPktsPerSec * (1 + kVariance);

    XLOG(DBG0) << "Before pkts: " << beforeOutPkts
               << " after pkts: " << afterOutPkts
               << " totalRecvdPkts: " << totalRecvdPkts
               << " duration: " << duration
               << " currPktsPerSec: " << currPktsPerSec
               << " low pktsPerSec: " << lowPktsPerSec
               << " high pktsPerSec: " << highPktsPerSec;

    /*
     * In practice, if no pps is configured, using the above method, the
     * packets are received at a rate > 2500 per second.
     */
    EXPECT_TRUE(
        lowPktsPerSec <= currPktsPerSec && currPktsPerSec <= highPktsPerSec);
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwCoppTest, LocalDstIpBgpPortToHighPriQ) {
  auto setup = [=]() {
    // COPP is part of initial config already
  };

  auto verify = [=]() {
    /*
     * XXX BCM_GPORT_LOCAL_CPU (335544320) does not work as it is different
     * from * portGport (134217728)?
     */
    // Make sure all dstip(=interfaces local ips) + BGP port packets send to
    // cpu high priority queue
    enum SRC_DST { SRC, DST };
    for (const auto& configIntf :
         folly::copy(*(this->initialConfig()).interfaces_ref())) {
      for (const auto& ipAddress : *configIntf.ipAddresses_ref()) {
        for (int dir = 0; dir <= DST; dir++) {
          this->sendPktAndVerifyCpuQueue(
              utility::getCoppHighPriQueueId(this->getAsic()),
              folly::IPAddress::createNetwork(ipAddress, -1, false).first,
              dir == SRC ? utility::kBgpPort : utility::kNonSpecialPort1,
              dir == DST ? utility::kBgpPort : utility::kNonSpecialPort1);
        }
      }
    }
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwCoppTest, LocalDstIpNonBgpPortToMidPriQ) {
  auto setup = [=]() {
    // COPP is part of initial config already
  };

  auto verify = [=]() {
    for (const auto& configIntf :
         folly::copy(*(this->initialConfig()).interfaces_ref())) {
      for (const auto& ipAddress : *configIntf.ipAddresses_ref()) {
        this->sendPktAndVerifyCpuQueue(
            utility::kCoppMidPriQueueId,
            folly::IPAddress::createNetwork(ipAddress, -1, false).first,
            utility::kNonSpecialPort1,
            utility::kNonSpecialPort2);

        // Also high-pri queue should always be 0
        EXPECT_EQ(
            0,
            this->getQueueOutPacketsWithRetry(
                utility::getCoppHighPriQueueId(this->getAsic()),
                kGetQueueOutPktsRetryTimes,
                0));
      }
    }
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwCoppTest, Ipv6LinkLocalMcastToMidPriQ) {
  auto setup = [=]() {
    // COPP is part of initial config already
  };

  auto verify = [=]() {
    const auto addresses = folly::make_array(
        kIPv6LinkLocalMcastAbsoluteAddress, kIPv6LinkLocalMcastAddress);
    for (const auto& address : addresses) {
      this->sendPktAndVerifyCpuQueue(
          utility::kCoppMidPriQueueId,
          address,
          utility::kNonSpecialPort1,
          utility::kNonSpecialPort2,
          kMcastMacAddress);

      // Also high-pri queue should always be 0
      EXPECT_EQ(
          0,
          this->getQueueOutPacketsWithRetry(
              utility::getCoppHighPriQueueId(this->getAsic()),
              kGetQueueOutPktsRetryTimes,
              0));
    }
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwCoppTest, Ipv6LinkLocalMcastTxFromCpu) {
  auto setup = [=]() {
    // COPP is part of initial config already
  };

  auto verify = [=]() {
    // Intent of this test is to verify that
    // link local ipv6 address is not looped back when sent from CPU
    this->sendUdpPktAndVerify(
        utility::kCoppMidPriQueueId,
        folly::IPAddressV6("ff02::1"),
        utility::kNonSpecialPort1,
        utility::kNonSpecialPort2,
        1 /* send pkt count */,
        0 /* expected rx count */);
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwCoppTest, Ipv6LinkLocalUcastToMidPriQ) {
  auto setup = [=]() {
    // COPP is part of initial config already
  };

  auto verify = [=]() {
    // Device link local unicast address should use mid-pri queue
    {
      const folly::IPAddressV6 linkLocalAddr = folly::IPAddressV6(
          folly::IPAddressV6::LINK_LOCAL, this->getPlatform()->getLocalMac());

      this->sendPktAndVerifyCpuQueue(
          utility::kCoppMidPriQueueId,
          linkLocalAddr,
          utility::kNonSpecialPort1,
          utility::kNonSpecialPort2);

      // Also high-pri queue should always be 0
      EXPECT_EQ(
          0,
          this->getQueueOutPacketsWithRetry(
              utility::getCoppHighPriQueueId(this->getAsic()),
              kGetQueueOutPktsRetryTimes,
              0));
    }
    // Non device link local unicast address should also use mid-pri queue
    {
      this->sendPktAndVerifyCpuQueue(
          utility::kCoppMidPriQueueId,
          kIPv6LinkLocalUcastAddress,
          utility::kNonSpecialPort1,
          utility::kNonSpecialPort2);

      // Also high-pri queue should always be 0
      EXPECT_EQ(
          0,
          this->getQueueOutPacketsWithRetry(
              utility::getCoppHighPriQueueId(this->getAsic()),
              kGetQueueOutPktsRetryTimes,
              0));
    }
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwCoppTest, SlowProtocolsMacToHighPriQ) {
  auto setup = [=]() {
    // COPP is part of initial config already
  };

  auto verify = [=]() {
    this->sendPktAndVerifyEthPacketsCpuQueue(
        utility::getCoppHighPriQueueId(this->getAsic()),
        facebook::fboss::ETHERTYPE::ETHERTYPE_SLOW_PROTOCOLS,
        LACPDU::kSlowProtocolsDstMac());
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwCoppTest, DstIpNetworkControlDscpToHighPriQ) {
  auto setup = [=]() {
    // COPP is part of initial config already
  };

  auto verify = [=]() {
    for (const auto& configIntf :
         folly::copy(*(this->initialConfig()).interfaces_ref())) {
      for (const auto& ipAddress : *configIntf.ipAddresses_ref()) {
        this->sendPktAndVerifyCpuQueue(
            utility::getCoppHighPriQueueId(this->getAsic()),
            folly::IPAddress::createNetwork(ipAddress, -1, false).first,
            utility::kNonSpecialPort1,
            utility::kNonSpecialPort2,
            std::nullopt,
            kNetworkControlDscp);
      }
      // Non local dst ip with kNetworkControlDscp should not hit high pri queue
      // (since it won't even trap to cpu)
      this->sendPktAndVerifyCpuQueue(
          utility::getCoppHighPriQueueId(this->getAsic()),
          folly::IPAddress("2::2"),
          utility::kNonSpecialPort1,
          utility::kNonSpecialPort2,
          std::nullopt,
          kNetworkControlDscp,
          1, /*num pkts to send*/
          0 /*expected delta*/);
    }
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwCoppTest, Ipv6LinkLocalUcastIpNetworkControlDscpToHighPriQ) {
  auto setup = [=]() {
    // COPP is part of initial config already
  };

  auto verify = [=]() {
    // Device link local unicast address + kNetworkControlDscp should use
    // high-pri queue
    {
      const folly::IPAddressV6 linkLocalAddr = folly::IPAddressV6(
          folly::IPAddressV6::LINK_LOCAL, this->getPlatform()->getLocalMac());

      this->sendPktAndVerifyCpuQueue(
          utility::getCoppHighPriQueueId(this->getAsic()),
          linkLocalAddr,
          utility::kNonSpecialPort1,
          utility::kNonSpecialPort2,
          std::nullopt,
          kNetworkControlDscp);
    }
    // Non device link local unicast address + kNetworkControlDscp dscp should
    // also use high-pri queue
    {
      this->sendPktAndVerifyCpuQueue(
          utility::getCoppHighPriQueueId(this->getAsic()),
          kIPv6LinkLocalUcastAddress,
          utility::kNonSpecialPort1,
          utility::kNonSpecialPort2,
          std::nullopt,
          kNetworkControlDscp);
    }
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwCoppTest, Ipv6LinkLocalMcastNetworkControlDscpToHighPriQ) {
  auto setup = [=]() {
    // COPP is part of initial config already
  };

  auto verify = [=]() {
    const auto addresses = folly::make_array(
        kIPv6LinkLocalMcastAbsoluteAddress, kIPv6LinkLocalMcastAddress);
    for (const auto& address : addresses) {
      this->sendPktAndVerifyCpuQueue(
          utility::getCoppHighPriQueueId(this->getAsic()),
          address,
          utility::kNonSpecialPort1,
          utility::kNonSpecialPort2,
          kMcastMacAddress,
          kNetworkControlDscp);
    }
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwCoppTest, L3MTUErrorToLowPriQ) {
  auto setup = [=]() {
    // COPP is part of initial config already
    utility::EcmpSetupAnyNPorts6 ecmpHelper(
        this->getProgrammedState(), this->getPlatform()->getLocalMac());
    this->resolveNeigborAndProgramRoutes(ecmpHelper, 1);
  };
  auto verify = [=]() {
    // Make sure all packets packet with large payload (> MTU)
    // are sent to cpu low priority queue.
    // Port Max Frame size is set to 9412 and L3 MTU is set as 9000
    // Thus sending a packet sized between 9000 and 9412 to cause the trap.
    auto randomIP = folly::IPAddressV6("2::2");
    this->sendPktAndVerifyCpuQueue(
        utility::kCoppLowPriQueueId,
        randomIP,
        utility::kNonSpecialPort1,
        utility::kNonSpecialPort2,
        std::nullopt,
        0,
        1, /* num pkts to send */
        1, /* num pkts to excepted to be captured */
        std::vector<uint8_t>(9200, 0xff));
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwCoppTest, ArpRequestAndReplyToHighPriQ) {
  auto setup = [=]() {};
  auto verify = [=]() {
    this->sendPktAndVerifyArpPacketsCpuQueue(
        utility::getCoppHighPriQueueId(this->getAsic()),
        folly::IPAddressV4("1.1.1.5"),
        ARP_OPER::ARP_OPER_REQUEST);
    this->sendPktAndVerifyArpPacketsCpuQueue(
        utility::getCoppHighPriQueueId(this->getAsic()),
        folly::IPAddressV4("1.1.1.5"),
        ARP_OPER::ARP_OPER_REPLY);
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwCoppTest, JumboFramesToQueues) {
  auto setup = [=]() {
    // COPP is part of initial config already
  };

  auto verify = [=]() {
    std::vector<uint8_t> jumboPayload(7000, 0xff);
    for (const auto& configIntf :
         folly::copy(*(this->initialConfig()).interfaces_ref())) {
      for (const auto& ipAddress : *configIntf.ipAddresses_ref()) {
        // High pri queue
        this->sendPktAndVerifyCpuQueue(
            utility::getCoppHighPriQueueId(this->getAsic()),
            folly::IPAddress::createNetwork(ipAddress, -1, false).first,
            utility::kBgpPort,
            utility::kNonSpecialPort2,
            std::nullopt, /*mac*/
            0, /* traffic class*/
            1, /* pkts to send*/
            1, /* expected delta*/
            jumboPayload);
        // Mid pri queue
        this->sendPktAndVerifyCpuQueue(
            utility::kCoppMidPriQueueId,
            folly::IPAddress::createNetwork(ipAddress, -1, false).first,
            utility::kNonSpecialPort1,
            utility::kNonSpecialPort2,
            std::nullopt, /*mac*/
            0, /* traffic class*/
            1, /* pkts to send*/
            1, /* expected delta*/
            jumboPayload);
      }
    }
    this->sendPktAndVerifyCpuQueue(
        utility::kCoppLowPriQueueId,
        this->getInSubnetNonSwitchIP(),
        utility::kNonSpecialPort1,
        utility::kNonSpecialPort2,
        std::nullopt, /*mac*/
        0, /* traffic class*/
        1, /* pkts to send*/
        1, /* expected delta*/
        jumboPayload);
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
