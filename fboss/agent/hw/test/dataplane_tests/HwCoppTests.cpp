/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "common/time/Time.h"
#include "fboss/agent/LacpTypes.h"
#include "fboss/agent/Platform.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwLinkStateDependentTest.h"
#include "fboss/agent/hw/test/HwTestCoppUtils.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"

#include <folly/IPAddress.h>
#include <folly/container/Array.h>

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
} // unnamed namespace

namespace facebook {
namespace fboss {

class HwCoppTest : public HwLinkStateDependentTest {
 protected:
  cfg::SwitchConfig initialConfig() const override {
    auto cfg = utility::oneL3IntfConfig(
        getHwSwitch(), masterLogicalPortIds()[0], cfg::PortLoopbackMode::MAC);
    utility::setDefaultCpuTrafficPolicyConfig(
        cfg, getPlatform()->getLocalMac());
    utility::addCpuQueueConfig(cfg);
    return cfg;
  }

  void sendUdpPkts(
      int numPktsToSend,
      const folly::IPAddress& dstIpAddress,
      int l4SrcPort,
      int l4DstPort,
      const std::optional<folly::MacAddress>& dstMac = std::nullopt,
      uint8_t trafficClass = 0) {
    auto vlanId = VlanID(initialConfig().vlanPorts[0].vlanID);
    auto intfMac = utility::getInterfaceMac(getProgrammedState(), vlanId);

    for (int i = 0; i < numPktsToSend; i++) {
      auto txPacket = utility::makeUDPTxPacket(
          getHwSwitch(),
          vlanId,
          intfMac,
          dstMac ? *dstMac : intfMac,
          folly::IPAddress(dstIpAddress.isV4() ? "1.1.1.2" : "1::1"),
          dstIpAddress,
          l4SrcPort,
          l4DstPort,
          dstIpAddress.isV4()
              ? trafficClass
              : trafficClass << 2 // v6 header takes entire TC byte with
                                  // trailing 2 bits for ECN. V4 header OTOH
                                  // expects only dscp value.
      );

      getHwSwitch()->sendPacketOutOfPortSync(
          std::move(txPacket), PortID(masterLogicalPortIds()[0]));
    }
  }

  uint64_t getQueueOutPacketsWithRetry(
      int queueId,
      int retryTimes,
      uint64_t expectedNumPkts) {
    uint64_t outPkts = 0;
    do {
      outPkts = utility::getCpuQueueOutPackets(getHwSwitch(), queueId);
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

  void sendPktAndVerifyCpuQueue(
      int queueId,
      const folly::IPAddress& dstIpAddress,
      const int l4SrcPort,
      const int l4DstPort,
      const std::optional<folly::MacAddress>& dstMac = std::nullopt,
      uint8_t trafficClass = 0,
      const int numPktsToSend = 1,
      const int expectedPktDelta = 1) {
    auto beforeOutPkts = getQueueOutPacketsWithRetry(
        queueId, 0 /* retryTimes */, 0 /* expectedNumPkts */);
    sendUdpPkts(
        numPktsToSend,
        dstIpAddress,
        l4SrcPort,
        l4DstPort,
        dstMac,
        trafficClass);
    auto afterOutPkts = getQueueOutPacketsWithRetry(
        queueId, kGetQueueOutPktsRetryTimes, beforeOutPkts + 1);
    XLOG(DBG0) << "Packet of DstIp=" << dstIpAddress.str()
               << ", from port=" << l4SrcPort << " to port=" << l4DstPort
               << ", dstMac="
               << (dstMac ? (*dstMac).toString()
                          : getPlatform()->getLocalMac().toString())
               << ". Queue=" << queueId << ", before pkts:" << beforeOutPkts
               << ", after pkts:" << afterOutPkts;
    EXPECT_EQ(expectedPktDelta, afterOutPkts - beforeOutPkts);
  }

 private:
  uint32_t featuresDesired() const override {
    return (HwSwitch::LINKSCAN_DESIRED | HwSwitch::PACKET_RX_DESIRED);
  }
};

TEST_F(HwCoppTest, VerifyCoppPpsLowPri) {
  auto setup = [=]() {
    // COPP is part of initial config already
  };

  auto verify = [=]() {
    auto kNumPktsToSend = 60000;
    auto kMinDurationInSecs = 12;
    const double kVariance = 0.12; // i.e. + or -12%

    auto beforeSecs = WallClockUtil::NowInSecFast();
    auto beforeOutPkts = getQueueOutPacketsWithRetry(
        utility::kCoppLowPriQueueId,
        0 /* retryTimes */,
        0 /* expectedNumPkts */);

    auto configIntf = initialConfig().interfaces[0];
    auto ipAddress = configIntf.ipAddresses[0];
    auto dstIP = folly::IPAddress::createNetwork(ipAddress, -1, true).first;
    uint64_t afterSecs;
    /*
     * To avoid noise, the send traffic for at least kMinDurationInSecs.
     * In practice, sending kNumPktsToSend packets takes longer than
     * kMinDurationInSecs. But to be safe, keep sending packets for
     * at least kMinDurationInSecs.
     */
    do {
      sendUdpPkts(
          kNumPktsToSend,
          dstIP,
          utility::kNonSpecialPort1,
          utility::kNonSpecialPort2);
      afterSecs = WallClockUtil::NowInSecFast();
    } while (afterSecs - beforeSecs < kMinDurationInSecs);

    auto afterOutPkts = getQueueOutPacketsWithRetry(
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

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwCoppTest, LocalDstIpBgpPortToHighPriQ) {
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
    for (const auto& configIntf : initialConfig().interfaces) {
      for (const auto& ipAddress : configIntf.ipAddresses) {
        for (int dir = 0; dir <= DST; dir++) {
          sendPktAndVerifyCpuQueue(
              utility::kCoppHighPriQueueId,
              folly::IPAddress::createNetwork(ipAddress, -1, false).first,
              dir == SRC ? utility::kBgpPort : utility::kNonSpecialPort1,
              dir == DST ? utility::kBgpPort : utility::kNonSpecialPort1);
        }
      }
    }
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwCoppTest, LocalDstIpNonBgpPortToMidPriQ) {
  auto setup = [=]() {
    // COPP is part of initial config already
  };

  auto verify = [=]() {
    for (const auto& configIntf : initialConfig().interfaces) {
      for (const auto& ipAddress : configIntf.ipAddresses) {
        sendPktAndVerifyCpuQueue(
            utility::kCoppMidPriQueueId,
            folly::IPAddress::createNetwork(ipAddress, -1, false).first,
            utility::kNonSpecialPort1,
            utility::kNonSpecialPort2);

        // Also high-pri queue should always be 0
        EXPECT_EQ(
            0,
            getQueueOutPacketsWithRetry(
                utility::kCoppHighPriQueueId, kGetQueueOutPktsRetryTimes, 0));
      }
    }
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwCoppTest, Ipv6LinkLocalMcastToMidPriQ) {
  auto setup = [=]() {
    // COPP is part of initial config already
  };

  auto verify = [=]() {
    const auto addresses = folly::make_array(
        kIPv6LinkLocalMcastAbsoluteAddress, kIPv6LinkLocalMcastAddress);
    for (const auto& address : addresses) {
      sendPktAndVerifyCpuQueue(
          utility::kCoppMidPriQueueId,
          address,
          utility::kNonSpecialPort1,
          utility::kNonSpecialPort2);

      // Also high-pri queue should always be 0
      EXPECT_EQ(
          0,
          getQueueOutPacketsWithRetry(
              utility::kCoppHighPriQueueId, kGetQueueOutPktsRetryTimes, 0));
    }
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwCoppTest, Ipv6LinkLocalUcastToMidPriQ) {
  auto setup = [=]() {
    // COPP is part of initial config already
  };

  auto verify = [=]() {
    // Device link local unicast address should use mid-pri queue
    {
      const folly::IPAddressV6 linkLocalAddr = folly::IPAddressV6(
          folly::IPAddressV6::LINK_LOCAL, getPlatform()->getLocalMac());

      sendPktAndVerifyCpuQueue(
          utility::kCoppMidPriQueueId,
          linkLocalAddr,
          utility::kNonSpecialPort1,
          utility::kNonSpecialPort2);

      // Also high-pri queue should always be 0
      EXPECT_EQ(
          0,
          getQueueOutPacketsWithRetry(
              utility::kCoppHighPriQueueId, kGetQueueOutPktsRetryTimes, 0));
    }
    // Non device link local unicast address should also use mid-pri queue
    {
      sendPktAndVerifyCpuQueue(
          utility::kCoppMidPriQueueId,
          kIPv6LinkLocalUcastAddress,
          utility::kNonSpecialPort1,
          utility::kNonSpecialPort2);

      // Also high-pri queue should always be 0
      EXPECT_EQ(
          0,
          getQueueOutPacketsWithRetry(
              utility::kCoppHighPriQueueId, kGetQueueOutPktsRetryTimes, 0));
    }
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwCoppTest, SlowProtocolsMacToHighPriQ) {
  auto setup = [=]() {
    // COPP is part of initial config already
  };

  auto verify = [=]() {
    sendPktAndVerifyCpuQueue(
        utility::kCoppHighPriQueueId,
        folly::IPAddressV6("2::2"),
        utility::kNonSpecialPort1,
        utility::kNonSpecialPort2,
        LACPDU::kSlowProtocolsDstMac());
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwCoppTest, DstIpNetworkControlDscpToHighPriQ) {
  auto setup = [=]() {
    // COPP is part of initial config already
  };

  auto verify = [=]() {
    for (const auto& configIntf : initialConfig().interfaces) {
      for (const auto& ipAddress : configIntf.ipAddresses) {
        sendPktAndVerifyCpuQueue(
            utility::kCoppHighPriQueueId,
            folly::IPAddress::createNetwork(ipAddress, -1, false).first,
            utility::kNonSpecialPort1,
            utility::kNonSpecialPort2,
            std::nullopt,
            kNetworkControlDscp);
      }
      // Non local dst ip with kNetworkControlDscp should not hit high pri queue
      // (since it won't even trap to cpu)
      sendPktAndVerifyCpuQueue(
          utility::kCoppHighPriQueueId,
          folly::IPAddress("2::2"),
          utility::kNonSpecialPort1,
          utility::kNonSpecialPort2,
          std::nullopt,
          kNetworkControlDscp,
          1, /*num pkts to send*/
          0 /*expected delta*/);
    }
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwCoppTest, Ipv6LinkLocalUcastIpNetworkControlDscpToHighPriQ) {
  auto setup = [=]() {
    // COPP is part of initial config already
  };

  auto verify = [=]() {
    // Device link local unicast address + kNetworkControlDscp should use
    // high-pri queue
    {
      const folly::IPAddressV6 linkLocalAddr = folly::IPAddressV6(
          folly::IPAddressV6::LINK_LOCAL, getPlatform()->getLocalMac());

      sendPktAndVerifyCpuQueue(
          utility::kCoppHighPriQueueId,
          linkLocalAddr,
          utility::kNonSpecialPort1,
          utility::kNonSpecialPort2,
          std::nullopt,
          kNetworkControlDscp);
    }
    // Non device link local unicast address + kNetworkControlDscp dscp should
    // also use high-pri queue
    {
      sendPktAndVerifyCpuQueue(
          utility::kCoppHighPriQueueId,
          kIPv6LinkLocalUcastAddress,
          utility::kNonSpecialPort1,
          utility::kNonSpecialPort2,
          std::nullopt,
          kNetworkControlDscp);
    }
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwCoppTest, Ipv6LinkLocalMcastNetworkControlDscpToHighPriQ) {
  auto setup = [=]() {
    // COPP is part of initial config already
  };

  auto verify = [=]() {
    const auto addresses = folly::make_array(
        kIPv6LinkLocalMcastAbsoluteAddress, kIPv6LinkLocalMcastAddress);
    for (const auto& address : addresses) {
      sendPktAndVerifyCpuQueue(
          utility::kCoppHighPriQueueId,
          address,
          utility::kNonSpecialPort1,
          utility::kNonSpecialPort2,
          std::nullopt,
          kNetworkControlDscp);
    }
  };
  verifyAcrossWarmBoots(setup, verify);
}
} // namespace fboss
} // namespace facebook
