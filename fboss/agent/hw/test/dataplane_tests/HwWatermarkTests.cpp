/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <fb303/ServiceData.h>
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwLinkStateDependentTest.h"
#include "fboss/agent/hw/test/HwSwitchEnsemble.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/hw/test/HwTestPortUtils.h"
#include "fboss/agent/hw/test/dataplane_tests/HwTestAqmUtils.h"
#include "fboss/agent/hw/test/dataplane_tests/HwTestOlympicUtils.h"
#include "fboss/agent/hw/test/dataplane_tests/HwTestQosUtils.h"
#include "fboss/agent/packet/EthHdr.h"
#include "fboss/agent/packet/IPv6Hdr.h"
#include "fboss/agent/packet/UDPHeader.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/test/EcmpSetupHelper.h"

#include "fboss/lib/CommonUtils.h"

#include <folly/IPAddress.h>

namespace facebook::fboss {

class HwWatermarkTest : public HwLinkStateDependentTest {
 private:
  cfg::SwitchConfig initialConfig() const override {
    auto cfg = utility::onePortPerInterfaceConfig(
        getHwSwitch(),
        masterLogicalPortIds(),
        getAsic()->desiredLoopbackMode());
    if (isSupported(HwAsic::Feature::L3_QOS)) {
      auto streamType =
          *(getPlatform()
                ->getAsic()
                ->getQueueStreamTypes(cfg::PortType::INTERFACE_PORT)
                .begin());
      utility::addOlympicQueueConfig(
          &cfg, streamType, getPlatform()->getAsic());
      utility::addOlympicQosMaps(cfg, getPlatform()->getAsic());
    }
    return cfg;
  }

  MacAddress kDstMac() const {
    return getPlatform()->getLocalMac();
  }

  void sendUdpPkt(
      uint8_t dscpVal,
      const folly::IPAddressV6& dst,
      int payloadSize,
      std::optional<PortID> port) {
    auto vlanId = utility::firstVlanID(initialConfig());
    auto intfMac = utility::getFirstInterfaceMac(initialConfig());
    auto srcMac = utility::MacAddressGenerator().get(intfMac.u64NBO() + 1);

    auto kECT1 = 0x01; // ECN capable transport ECT(1)
    auto txPacket = utility::makeUDPTxPacket(
        getHwSwitch(),
        vlanId,
        srcMac,
        intfMac,
        folly::IPAddressV6("2620:0:1cfe:face:b00c::3"),
        dst,
        8000,
        8001,
        // Trailing 2 bits are for ECN, we do not want drops in
        // these queues due to any configured thresholds!
        static_cast<uint8_t>(dscpVal << 2 | kECT1),
        255,
        std::vector<uint8_t>(payloadSize, 0xff));
    if (port.has_value()) {
      getHwSwitch()->sendPacketOutOfPortSync(std::move(txPacket), *port);
    } else {
      getHwSwitch()->sendPacketSwitchedSync(std::move(txPacket));
    }
  }

  void checkFb303BufferWatermarkUcast(
      const PortID& portId,
      const int queueId,
      bool isVoq) {
    std::string portName;
    if (!isVoq) {
      portName = getProgrammedState()->getPorts()->getPort(portId)->getName();
    } else {
      auto systemPortId = getSystemPortID(portId, getProgrammedState());
      portName = getProgrammedState()
                     ->getMultiSwitchSystemPorts()
                     ->getNodeIf(systemPortId)
                     ->getPortName();
    }

    auto counters = fb303::fbData->getRegexCounters({folly::sformat(
        "buffer_watermark_ucast.{}.queue{}.*.p100.60", portName, queueId)});
    // Unfortunately since  we use quantile stats, which compute
    // a MAX over a period, we can't really assert on the exact
    // value, just on its presence
    EXPECT_EQ(1, counters.size());
  }

  void
  assertWatermark(PortID port, int queueId, bool expectZero, int retries = 1) {
    EXPECT_TRUE(
        gotExpectedWatermark(port, queueId, expectZero, retries, false));
    if (getPlatform()->getAsic()->getSwitchType() == cfg::SwitchType::VOQ) {
      EXPECT_TRUE(
          gotExpectedWatermark(port, queueId, expectZero, retries, true));
    }
  }

  bool gotExpectedWatermark(
      const PortID& port,
      int queueId,
      bool expectZero,
      int retries,
      bool isVoq) {
    std::map<int16_t, int64_t> queueWaterMarks;
    auto portName = getProgrammedState()->getPorts()->getPort(port)->getName();
    auto queueTypeStr = isVoq ? " voq queueId: " : " queueId: ";
    auto watermarkStatsCheck = [&]() {
      XLOG(DBG2) << "Port: " << portName << queueTypeStr << queueId
                 << " Watermark: " << queueWaterMarks[queueId];
      if ((expectZero && !queueWaterMarks[queueId]) ||
          (!expectZero && queueWaterMarks[queueId])) {
        return true;
      }
      return false;
    };

    auto updatePortOrSysportStats = [&]() {
      queueWaterMarks = getQueueWatermarks(port, isVoq);
    };
    auto statsConditionMet = getHwSwitchEnsemble()->waitStatsCondition(
        watermarkStatsCheck,
        updatePortOrSysportStats,
        retries,
        std::chrono::milliseconds(1000));
    if (!statsConditionMet) {
      auto expectation = expectZero ? "zero" : "non-zero";
      XLOG(DBG2) << " Did not get expected " << expectation
                 << " watermark value!";
    } else {
      XLOG(DBG2) << "Port: " << portName << queueTypeStr << queueId
                 << " got expected watermarks of " << queueWaterMarks[queueId];
    }
    // Check fb303
    checkFb303BufferWatermarkUcast(port, queueId, isVoq);
    return statsConditionMet;
  }

  uint64_t getMinDeviceWatermarkValue() {
    uint64_t minDeviceWatermarkBytes{0};
    if (getAsic()->getAsicType() == cfg::AsicType::ASIC_TYPE_EBRO) {
      /*
       * Ebro will always have some internal buffer utilization even
       * when there is no traffic in the ASIC. The recommendation is
       * to consider atleast 100 buffers, translating to 100 x 384B
       * as steady state device watermark.
       */
      constexpr auto kEbroMinDeviceWatermarkBytes = 38400;
      minDeviceWatermarkBytes = kEbroMinDeviceWatermarkBytes;
    }
    return minDeviceWatermarkBytes;
  }

  bool gotExpectedDeviceWatermark(bool expectZero, int retries) {
    XLOG(DBG0) << "Expect zero watermark: " << std::boolalpha << expectZero;
    do {
      getQueueWatermarks(masterLogicalInterfacePortIds()[0], false /*isVoq*/);
      auto deviceWatermarkBytes =
          getHwSwitchEnsemble()->getHwSwitch()->getDeviceWatermarkBytes();
      XLOG(DBG0) << "Device watermark bytes: " << deviceWatermarkBytes;
      auto watermarkAsExpected =
          (expectZero &&
           (deviceWatermarkBytes <= getMinDeviceWatermarkValue())) ||
          (!expectZero &&
           (deviceWatermarkBytes > getMinDeviceWatermarkValue()));
      if (watermarkAsExpected) {
        return true;
      }
      XLOG(DBG0) << " Retry ...";
      sleep(1);
    } while (--retries > 0);

    XLOG(DBG2) << " Did not get expected device watermark value";
    return false;
  }
  std::map<PortID, folly::IPAddressV6> getPort2DstIp() const {
    return {
        {masterLogicalInterfacePortIds()[0], kDestIp1()},
        {masterLogicalInterfacePortIds()[1], kDestIp2()},
    };
  }

 protected:
  folly::IPAddressV6 kDestIp1() const {
    return folly::IPAddressV6("2620:0:1cfe:face:b00c::4");
  }
  folly::IPAddressV6 kDestIp2() const {
    return folly::IPAddressV6("2620:0:1cfe:face:b00c::5");
  }

  // isVoq applies only to VoQ systems and will be used to collect
  // VoQ stats for VoQ switches.
  const std::map<int16_t, int64_t> getQueueWatermarks(
      const PortID& portId,
      bool isVoq) {
    if (getPlatform()->getAsic()->getSwitchType() == cfg::SwitchType::VOQ &&
        isVoq) {
      auto sysPortId = getSystemPortID(portId, getProgrammedState());
      return *getHwSwitchEnsemble()
                  ->getLatestSysPortStats(sysPortId)
                  .queueWatermarkBytes_();
    } else {
      return *getHwSwitchEnsemble()
                  ->getLatestPortStats(portId)
                  .queueWatermarkBytes_();
    }
  }

  /*
   * In practice, sending single packet usually (but not always) returned BST
   * value > 0 (usually 2, but  other times it was 0). Thus, send a large
   * number of packets to try to avoid the risk of flakiness.
   */
  void sendUdpPkts(
      uint8_t dscpVal,
      const folly::IPAddressV6& dstIp,
      int cnt = 100,
      int payloadSize = 6000,
      std::optional<PortID> port = std::nullopt) {
    for (int i = 0; i < cnt; i++) {
      sendUdpPkt(dscpVal, dstIp, payloadSize, port);
    }
  }

  void disableTTLDecrements(
      const utility::EcmpSetupTargetedPorts6& ecmpHelper,
      const PortDescriptor& portDesc) {
    const auto& nextHop = ecmpHelper.nhop(portDesc);
    utility::ttlDecrementHandlingForLoopbackTraffic(
        getHwSwitch(), ecmpHelper.getRouterId(), nextHop);
  }

  void _setup(bool needTrafficLoop = false) {
    auto intfMac = utility::getFirstInterfaceMac(initialConfig());
    std::optional<folly::MacAddress> macAddr{};
    if (needTrafficLoop) {
      macAddr = intfMac;
    }
    utility::EcmpSetupTargetedPorts6 ecmpHelper6{getProgrammedState(), macAddr};
    for (auto portAndIp : getPort2DstIp()) {
      auto portDesc = PortDescriptor(portAndIp.first);
      applyNewState(
          ecmpHelper6.resolveNextHops(getProgrammedState(), {portDesc}));
      ecmpHelper6.programRoutes(
          getRouteUpdater(),
          {portDesc},
          {Route<folly::IPAddressV6>::Prefix{portAndIp.second, 128}});
      if (needTrafficLoop) {
        disableTTLDecrements(ecmpHelper6, portDesc);
      }
    }
  }

  void assertDeviceWatermark(bool expectZero, int retries = 1) {
    EXPECT_TRUE(gotExpectedDeviceWatermark(expectZero, retries));
  }

  void stopTraffic(const PortID& portId) {
    // Toggle the link to break traffic loop
    bringDownPort(portId);
    bringUpPort(portId);
  }

  void runTest(int queueId) {
    if (!isSupported(HwAsic::Feature::L3_QOS)) {
      return;
    }

    auto setup = [this]() {
      // Needs a traffic loop to accommodate some ASICs like Jericho
      _setup(true /*needTrafficLoop*/);
    };
    auto verify = [this, queueId]() {
      auto dscpsForQueue =
          utility::kOlympicQueueToDscp(getAsic()).find(queueId)->second;
      for (auto portAndIp : getPort2DstIp()) {
        // Assert zero watermark
        assertWatermark(portAndIp.first, queueId, true /*expectZero*/, 2);
        // Send traffic
        sendUdpPkts(dscpsForQueue[0], portAndIp.second);
        // Assert non zero watermark
        assertWatermark(portAndIp.first, queueId, false /*expectZero*/, 5);
        // Stop traffic
        stopTraffic(portAndIp.first);
        // Assert zero watermark
        assertWatermark(portAndIp.first, queueId, true /*expectZero*/, 5);
      }
    };
    verifyAcrossWarmBoots(setup, verify);
  }
};

TEST_F(HwWatermarkTest, VerifyDefaultQueue) {
  runTest(getOlympicQueueId(getAsic(), utility::OlympicQueueType::SILVER));
}

TEST_F(HwWatermarkTest, VerifyNonDefaultQueue) {
  runTest(getOlympicQueueId(getAsic(), utility::OlympicQueueType::GOLD));
}

// TODO - merge device watermark checking into the tests
// above once all platforms support device watermarks
TEST_F(HwWatermarkTest, VerifyDeviceWatermark) {
  auto setup = [this]() { _setup(true); };
  auto verify = [this]() {
    auto minPktsForLineRate = getHwSwitchEnsemble()->getMinPktsForLineRate(
        masterLogicalInterfacePortIds()[0]);
    sendUdpPkts(0, kDestIp1(), minPktsForLineRate);
    getHwSwitchEnsemble()->waitForLineRateOnPort(
        masterLogicalInterfacePortIds()[0]);
    // Assert non zero watermark
    assertDeviceWatermark(false);
    auto counters =
        fb303::fbData->getSelectedCounters({"buffer_watermark_device.p100.60"});
    // Unfortunately since  we use quantile stats, which compute
    // a MAX over a period, we can't really assert on the exact
    // value, just on its presence
    EXPECT_EQ(1, counters.size());

    // Now, break the loop to make sure traffic goes to zero!
    bringDownPort(masterLogicalInterfacePortIds()[0]);
    bringUpPort(masterLogicalInterfacePortIds()[0]);

    // Assert zero watermark
    assertDeviceWatermark(true, 5);
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwWatermarkTest, VerifyDeviceWatermarkHigherThanQueueWatermark) {
  auto setup = [this]() {
    _setup(true);
    auto minPktsForLineRate = getHwSwitchEnsemble()->getMinPktsForLineRate(
        masterLogicalInterfacePortIds()[0]);
    // Sending traffic on 2 queues
    sendUdpPkts(
        utility::kOlympicQueueToDscp(getAsic())
            .at(utility::getOlympicQueueId(
                getAsic(), utility::OlympicQueueType::SILVER))
            .front(),
        kDestIp1(),
        minPktsForLineRate / 2);
    sendUdpPkts(
        utility::kOlympicQueueToDscp(getAsic())
            .at(utility::getOlympicQueueId(
                getAsic(), utility::OlympicQueueType::GOLD))
            .front(),
        kDestIp1(),
        minPktsForLineRate / 2);
    getHwSwitchEnsemble()->waitForLineRateOnPort(
        masterLogicalInterfacePortIds()[0]);
  };
  auto verify = [this]() {
    if (!isSupported(HwAsic::Feature::L3_QOS)) {
      return;
    }
    auto queueIdGold =
        utility::getOlympicQueueId(getAsic(), utility::OlympicQueueType::GOLD);
    auto queueIdSilver = utility::getOlympicQueueId(
        getAsic(), utility::OlympicQueueType::SILVER);
    auto queueWatermarkNonZero =
        [queueIdGold,
         queueIdSilver](std::map<int16_t, int64_t>& queueWaterMarks) {
          if (queueWaterMarks.at(queueIdSilver) ||
              queueWaterMarks.at(queueIdGold)) {
            return true;
          }
          return false;
        };

    /*
     * Now we are at line rate on port, make sure that queue watermark
     * is non-zero! As of now, the assumption is as below:
     *
     * 1. VoQ switches has device watermarks being reported from ingress
     *    buffers, hence compare it with VoQ watermarks
     * 2. Non-voq switches just has egress queue watermarks which is from
     *    the same buffer as the device watermark is reported.
     *
     * In case of new platforms, make sure that the assumption holds.
     */
    std::map<int16_t, int64_t> queueWaterMarks;
    WITH_RETRIES_N_TIMED(5, std::chrono::milliseconds(1000), {
      queueWaterMarks = getQueueWatermarks(
          masterLogicalInterfacePortIds()[0],
          getPlatform()->getAsic()->getSwitchType() ==
              cfg::SwitchType::VOQ /*isVoq*/);
      EXPECT_EVENTUALLY_TRUE(queueWatermarkNonZero(queueWaterMarks));
    });

    // Get device watermark now, so that it is > highest queue watermark!
    auto deviceWaterMark =
        getHwSwitchEnsemble()->getHwSwitch()->getDeviceWatermarkBytes();
    XLOG(DBG2) << "For port: " << masterLogicalInterfacePortIds()[0]
               << ", Queue" << queueIdSilver
               << " watermark: " << queueWaterMarks.at(queueIdSilver)
               << ", Queue" << queueIdGold
               << " watermark: " << queueWaterMarks.at(queueIdGold)
               << ", Device watermark: " << deviceWaterMark;

    // Make sure that device watermark is > highest queue watermark
    EXPECT_GT(
        deviceWaterMark,
        std::max(
            queueWaterMarks.at(queueIdSilver),
            queueWaterMarks.at(queueIdGold)));
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwWatermarkTest, VerifyQueueWatermarkAccuracy) {
  auto setup = [this]() { _setup(false); };
  auto verify = [this]() {
    if (!isSupported(HwAsic::Feature::L3_QOS)) {
#if defined(GTEST_SKIP)
      GTEST_SKIP();
#endif
      return;
    }

    /*
     * This test ensures that the queue watermark reported is accurate.
     * For this, we start by clearing any watermark from previous tests.
     * We need to know the exact number of bytes a packet will need while
     * it is in the ASIC queue. This size includes the headers + overhead
     * used in the ASIC for each packet. Once we know the number of bytes
     * that will be used per packet, with TX disable function, we ensure
     * that all the packets to be TXed is queued up, resulting in a high
     * watermark which is predictable. Then compare and make sure that
     * the watermark reported is as expected by the computed watermark
     * based on the number of bytes we expect in the queue.
     */
    auto kQueueId = utility::getOlympicQueueId(
        getAsic(), utility::OlympicQueueType::SILVER);
    constexpr auto kTxPacketPayloadLen{200};
    constexpr auto kNumberOfPacketsToSend{100};
    const bool isVoq =
        getPlatform()->getAsic()->getSwitchType() == cfg::SwitchType::VOQ;
    auto txPacketLen = kTxPacketPayloadLen + EthHdr::SIZE + IPv6Hdr::size() +
        UDPHeader::size();
    // Clear any watermark stats
    getQueueWatermarks(masterLogicalInterfacePortIds()[0], isVoq);

    auto sendPackets = [=](PortID /*port*/, int numPacketsToSend) {
      // Send packets out on port1, so that it gets looped back, and
      // forwarded in the pipeline to egress port0 where the watermark
      // will be validated.
      sendUdpPkts(
          utility::kOlympicQueueToDscp(getAsic()).at(kQueueId).front(),
          kDestIp1(),
          numPacketsToSend,
          kTxPacketPayloadLen,
          masterLogicalInterfacePortIds()[1]);
    };

    utility::sendPacketsWithQueueBuildup(
        sendPackets,
        getHwSwitchEnsemble(),
        masterLogicalInterfacePortIds()[0],
        kNumberOfPacketsToSend);

    uint64_t expectedWatermarkBytes;
    if (getPlatform()->getAsic()->getAsicType() ==
        cfg::AsicType::ASIC_TYPE_JERICHO2) {
      // For Jericho2, there is a limitation that one packet less than
      // expected watermark shows up in watermark.
      expectedWatermarkBytes =
          utility::getEffectiveBytesPerPacket(getHwSwitch(), txPacketLen) *
          (kNumberOfPacketsToSend - 1);
    } else {
      expectedWatermarkBytes =
          utility::getEffectiveBytesPerPacket(getHwSwitch(), txPacketLen) *
          kNumberOfPacketsToSend;
    }
    auto roundedWatermarkBytes = utility::getRoundedBufferThreshold(
        getHwSwitch(), expectedWatermarkBytes);
    std::map<int16_t, int64_t> queueWaterMarks;
    int64_t maxWatermarks = 0;
    WITH_RETRIES_N_TIMED(5, std::chrono::milliseconds(1000), {
      queueWaterMarks =
          getQueueWatermarks(masterLogicalInterfacePortIds()[0], isVoq);
      if (queueWaterMarks.at(kQueueId) > maxWatermarks) {
        maxWatermarks = queueWaterMarks.at(kQueueId);
      }
      EXPECT_EVENTUALLY_EQ(maxWatermarks, roundedWatermarkBytes);
    });

    XLOG(DBG0) << "Expected rounded watermark bytes: " << roundedWatermarkBytes
               << ", reported watermark bytes: " << maxWatermarks
               << ", pkts TXed: " << kNumberOfPacketsToSend
               << ", pktLen: " << txPacketLen << ", effectiveBytesPerPacket: "
               << utility::getEffectiveBytesPerPacket(
                      getHwSwitch(), txPacketLen);
  };
  verifyAcrossWarmBoots(setup, verify);
}
} // namespace facebook::fboss
