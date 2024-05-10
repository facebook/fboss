// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/TxPacket.h"
#include "fboss/agent/Utils.h"
#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/agent/test/utils/CoppTestUtils.h"
#include "fboss/agent/test/utils/OlympicTestUtils.h"
#include "fboss/agent/test/utils/QosTestUtils.h"
#include "fboss/lib/CommonUtils.h"

namespace facebook::fboss {

class AgentWatermarkTest : public AgentHwTest {
 protected:
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto config = AgentHwTest::initialConfig(ensemble);
    utility::addOlympicQueueConfig(&config, ensemble.getL3Asics());
    utility::addOlympicQosMaps(config, ensemble.getL3Asics());
    utility::setTTLZeroCpuConfig(ensemble.getL3Asics(), config);
    return config;
  }

  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    return {production_features::ProductionFeature::L3_QOS};
  }

  folly::IPAddressV6 kDestIp1() const {
    return folly::IPAddressV6("2620:0:1cfe:face:b00c::4");
  }
  folly::IPAddressV6 kDestIp2() const {
    return folly::IPAddressV6("2620:0:1cfe:face:b00c::5");
  }

  std::map<PortID, folly::IPAddressV6> getPort2DstIp() const {
    return {
        {getAgentEnsemble()->masterLogicalInterfacePortIds()[0], kDestIp1()},
        {getAgentEnsemble()->masterLogicalInterfacePortIds()[1], kDestIp2()},
    };
  }

  void stopTraffic(const PortID& portId) {
    // Toggle the link to break traffic loop
    getAgentEnsemble()->getLinkToggler()->bringDownPorts({portId});
    getAgentEnsemble()->getLinkToggler()->bringUpPorts({portId});
  }

  void sendUdpPkt(
      uint8_t dscpVal,
      const folly::IPAddressV6& dst,
      int payloadSize,
      std::optional<PortID> port) {
    auto vlanId = utility::firstVlanID(getProgrammedState());
    auto intfMac = utility::getFirstInterfaceMac(getProgrammedState());
    auto srcMac = utility::MacAddressGenerator().get(intfMac.u64NBO() + 1);

    auto kECT1 = 0x01; // ECN capable transport ECT(1)
    auto txPacket = utility::makeUDPTxPacket(
        getSw(),
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
      getSw()->sendPacketOutOfPortAsync(std::move(txPacket), *port);
    } else {
      getSw()->sendPacketSwitchedAsync(std::move(txPacket));
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

  void checkFb303BufferWatermarkUcast(
      const PortID& portId,
      const int queueId,
      bool isVoq) {
    std::string portName;
    if (!isVoq) {
      portName = getProgrammedState()->getPorts()->getNodeIf(portId)->getName();

    } else {
      auto systemPortId = getSystemPortID(
          portId,
          utility::getFirstNodeIf(getProgrammedState()->getSwitchSettings())
              ->getSwitchIdToSwitchInfo(),
          scopeResolver().scope(portId).switchId());
      portName = getProgrammedState()
                     ->getSystemPorts()
                     ->getNodeIf(systemPortId)
                     ->getPortName();
    }

    if (!FLAGS_multi_switch) {
      auto counters = fb303::fbData->getRegexCounters({folly::sformat(
          "buffer_watermark_ucast.{}.*.queue{}.*.p100.60", portName, queueId)});
      // Unfortunately since  we use quantile stats, which compute
      // a MAX over a period, we can't really assert on the exact
      // value, just on its presence
      WITH_RETRIES({ EXPECT_EVENTUALLY_EQ(1, counters.size()); });
    } else {
      // TODO: Check the fb303 counter on HwAgent
    }
  }

  const std::map<int16_t, int64_t> getQueueWatermarks(
      const PortID& portId,
      bool isVoq) {
    if (isVoq) {
      auto sysPortId = getSystemPortID(
          portId,
          utility::getFirstNodeIf(getProgrammedState()->getSwitchSettings())
              ->getSwitchIdToSwitchInfo(),
          switchIdForPort(portId));
      return *getLatestSysPortStats(sysPortId).queueWatermarkBytes_();
    } else {
      return *getLatestPortStats(portId).queueWatermarkBytes_();
    }
  }

  uint64_t getMinDeviceWatermarkValue(SwitchID switchId) {
    uint64_t minDeviceWatermarkBytes{0};
    if (getSw()->getHwAsicTable()->getHwAsic(switchId)->getAsicType() ==
        cfg::AsicType::ASIC_TYPE_EBRO) {
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

  bool gotExpectedDeviceWatermark(bool expectZero, SwitchID switchId) {
    XLOG(DBG0) << "Expect zero watermark: " << std::boolalpha << expectZero;
    bool watermarkAsExpected = false;
    WITH_RETRIES({
      auto switchWatermarkStats = getAllSwitchWatermarkStats();
      if (switchWatermarkStats.find(switchId) == switchWatermarkStats.end()) {
        continue;
      }
      auto deviceWatermarkBytes =
          *switchWatermarkStats.at(switchId).deviceWatermarkBytes();
      XLOG(DBG0) << "Device watermark bytes: " << deviceWatermarkBytes;
      watermarkAsExpected |=
          (expectZero &&
           (deviceWatermarkBytes <= getMinDeviceWatermarkValue(switchId))) ||
          (!expectZero &&
           (deviceWatermarkBytes > getMinDeviceWatermarkValue(switchId)));
      EXPECT_EVENTUALLY_TRUE(watermarkAsExpected);
    });
    if (!watermarkAsExpected) {
      XLOG(DBG2) << "Did not get expected device watermark value";
    }
    return watermarkAsExpected;
  }

  bool gotExpectedWatermark(
      const PortID& port,
      int queueId,
      bool expectZero,
      bool isVoq) {
    std::map<int16_t, int64_t> queueWaterMarks;
    auto queueTypeStr = isVoq ? " voq queueId: " : " queueId: ";
    auto portName =
        getProgrammedState()->getPorts()->getNodeIf(port)->getName();
    bool statsConditionMet = false;
    WITH_RETRIES({
      queueWaterMarks = getQueueWatermarks(port, isVoq);
      XLOG(DBG2) << "Port: " << portName << queueTypeStr << queueId
                 << " Watermark: " << queueWaterMarks[queueId];
      statsConditionMet = (expectZero && !queueWaterMarks[queueId]) ||
          (!expectZero && queueWaterMarks[queueId]);
      EXPECT_EVENTUALLY_TRUE(statsConditionMet);
    });

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

  void assertWatermark(
      PortID port,
      const HwAsic* asic,
      int queueId,
      bool expectZero) {
    EXPECT_TRUE(gotExpectedWatermark(
        port,
        queueId,
        expectZero,
        asic->getSwitchType() == cfg::SwitchType::VOQ));
  }

  void _setup(bool needTrafficLoop = false) {
    auto intfMac = utility::getFirstInterfaceMac(getProgrammedState());
    std::optional<folly::MacAddress> macAddr{};
    if (needTrafficLoop) {
      macAddr = intfMac;
    }
    utility::EcmpSetupTargetedPorts6 ecmpHelper6{getProgrammedState(), macAddr};
    for (const auto& portAndIp : getPort2DstIp()) {
      auto portDesc = PortDescriptor(portAndIp.first);
      applyNewState([&](const std::shared_ptr<SwitchState>& in) {
        return ecmpHelper6.resolveNextHops(in, {portDesc});
      });
      auto routeUpdater = getSw()->getRouteUpdater();
      ecmpHelper6.programRoutes(
          &routeUpdater,
          {portDesc},
          {Route<folly::IPAddressV6>::Prefix{portAndIp.second, 128}});
      if (needTrafficLoop) {
        const auto& nextHop = ecmpHelper6.nhop(portDesc);
        utility::ttlDecrementHandlingForLoopbackTraffic(
            getAgentEnsemble(), ecmpHelper6.getRouterId(), nextHop);
      }
    }
  }

  void runTest(int queueId) {
    auto setup = [this]() {
      // Needs a traffic loop to accommodate some ASICs like Jericho
      _setup(true /*needTrafficLoop*/);
    };
    auto verify = [this, queueId]() {
      for (const auto& portAndIp : getPort2DstIp()) {
        const auto asic = getSw()->getHwAsicTable()->getHwAsic(
            scopeResolver().scope(portAndIp.first).switchId());
        auto dscpsForQueue =
            utility::kOlympicQueueToDscp().find(queueId)->second;
        // Assert zero watermark
        assertWatermark(portAndIp.first, asic, queueId, true /*expectZero*/);
        // Send traffic
        sendUdpPkts(dscpsForQueue[0], portAndIp.second);
        // Assert non zero watermark
        assertWatermark(portAndIp.first, asic, queueId, false /*expectZero*/);
        // Stop traffic
        stopTraffic(portAndIp.first);
        // Assert zero watermark
        assertWatermark(portAndIp.first, asic, queueId, true /*expectZero*/);
      }
    };
    verifyAcrossWarmBoots(setup, verify);
  }
};

TEST_F(AgentWatermarkTest, VerifyDefaultQueue) {
  runTest(getOlympicQueueId(utility::OlympicQueueType::SILVER));
}

TEST_F(AgentWatermarkTest, VerifyNonDefaultQueue) {
  runTest(getOlympicQueueId(utility::OlympicQueueType::GOLD));
}

// TODO - merge device watermark checking into the tests
// above once all platforms support device watermarks
TEST_F(AgentWatermarkTest, VerifyDeviceWatermark) {
  auto setup = [this]() { _setup(true); };
  auto verify = [this]() {
    for (const auto& switchId : getSw()->getSwitchInfoTable().getSwitchIDs()) {
      auto portToSendTraffic =
          getAgentEnsemble()->masterLogicalInterfacePortIds(switchId)[0];
      auto minPktsForLineRate =
          getAgentEnsemble()->getMinPktsForLineRate(portToSendTraffic);
      sendUdpPkts(0, kDestIp1(), minPktsForLineRate);
      getAgentEnsemble()->waitForLineRateOnPort(portToSendTraffic);
      // Assert non zero watermark
      EXPECT_TRUE(gotExpectedDeviceWatermark(false, switchId));

      // Now, break the loop to make sure traffic goes to zero!
      getAgentEnsemble()->getLinkToggler()->bringDownPorts({portToSendTraffic});
      getAgentEnsemble()->getLinkToggler()->bringUpPorts({portToSendTraffic});

      // Assert zero watermark
      EXPECT_TRUE(gotExpectedDeviceWatermark(true, switchId));
    }
  };
  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
