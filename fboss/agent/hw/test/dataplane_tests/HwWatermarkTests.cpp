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
#include "fboss/agent/hw/test/HwTestCoppUtils.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/hw/test/HwTestPortUtils.h"
#include "fboss/agent/hw/test/dataplane_tests/HwTestAqmUtils.h"
#include "fboss/agent/hw/test/dataplane_tests/HwTestQosUtils.h"
#include "fboss/agent/packet/EthHdr.h"
#include "fboss/agent/packet/IPv6Hdr.h"
#include "fboss/agent/packet/UDPHeader.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/utils/OlympicTestUtils.h"

#include "fboss/lib/CommonUtils.h"

#include <folly/IPAddress.h>

namespace facebook::fboss {

class HwWatermarkTest : public HwLinkStateDependentTest {
 private:
  cfg::SwitchConfig initialConfig() const override {
    auto cfg = utility::onePortPerInterfaceConfig(
        getHwSwitch(),
        masterLogicalPortIds(),
        getAsic()->desiredLoopbackModes());
    if (isSupported(HwAsic::Feature::L3_QOS)) {
      utility::addOlympicQueueConfig(&cfg, getHwSwitchEnsemble()->getL3Asics());
      utility::addOlympicQosMaps(cfg, getHwSwitchEnsemble()->getL3Asics());
    }
    utility::setTTLZeroCpuConfig(getHwSwitchEnsemble()->getL3Asics(), cfg);
    return cfg;
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
      auto sysPortId = getSystemPortID(
          portId,
          utility::getFirstNodeIf(getProgrammedState()->getSwitchSettings())
              ->getSwitchIdToSwitchInfo(),
          getPlatform()->getHwSwitch()->getSwitchID());
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
        getHwSwitchEnsemble(), ecmpHelper.getRouterId(), nextHop);
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
};

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
    auto kQueueId =
        utility::getOlympicQueueId(utility::OlympicQueueType::SILVER);
    constexpr auto kTxPacketPayloadLen{200};
    constexpr auto kNumberOfPacketsToSend{100};
    const bool isVoq =
        getPlatform()->getAsic()->getSwitchType() == cfg::SwitchType::VOQ;
    auto txPacketLen = kTxPacketPayloadLen + EthHdr::SIZE + IPv6Hdr::size() +
        UDPHeader::size();
    // Clear any watermark stats
    getQueueWatermarks(masterLogicalInterfacePortIds()[0], isVoq);

    auto sendPackets = [=, this](PortID /*port*/, int numPacketsToSend) {
      // Send packets out on port1, so that it gets looped back, and
      // forwarded in the pipeline to egress port0 where the watermark
      // will be validated.
      sendUdpPkts(
          utility::kOlympicQueueToDscp().at(kQueueId).front(),
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
    uint64_t roundedWatermarkBytes;
    if (getPlatform()->getAsic()->getAsicType() ==
        cfg::AsicType::ASIC_TYPE_JERICHO2) {
      // For Jericho2, there is a limitation that one packet less than
      // expected watermark shows up in watermark.
      expectedWatermarkBytes =
          utility::getEffectiveBytesPerPacket(getHwSwitch(), txPacketLen) *
          (kNumberOfPacketsToSend - 1);
      // Watermarks read in are accurate, no rounding needed
      roundedWatermarkBytes = expectedWatermarkBytes;
    } else if (
        getPlatform()->getAsic()->getAsicType() ==
        cfg::AsicType::ASIC_TYPE_YUBA) {
      // For Yuba, watermark counter is accurate to the number of buffers
      expectedWatermarkBytes =
          utility::getEffectiveBytesPerPacket(getHwSwitch(), txPacketLen) *
          kNumberOfPacketsToSend;
      roundedWatermarkBytes = expectedWatermarkBytes;
    } else {
      expectedWatermarkBytes =
          utility::getEffectiveBytesPerPacket(getHwSwitch(), txPacketLen) *
          kNumberOfPacketsToSend;
      roundedWatermarkBytes = utility::getRoundedBufferThreshold(
          getHwSwitch(), expectedWatermarkBytes);
    }
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
