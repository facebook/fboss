/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwLinkStateDependentTest.h"
#include "fboss/agent/hw/test/HwTestCoppUtils.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/test/EcmpSetupHelper.h"

#include <folly/IPAddress.h>

namespace facebook::fboss {

class HwSflowTest : public HwLinkStateDependentTest {
 protected:
  cfg::SwitchConfig initialConfig() const override {
    auto cfg = utility::oneL3IntfConfig(
        getHwSwitch(),
        masterLogicalPortIds()[0],
        getAsic()->desiredLoopbackMode());
    utility::setDefaultCpuTrafficPolicyConfig(cfg, getAsic());
    utility::addCpuQueueConfig(cfg, this->getAsic());
    return cfg;
  }

  void sendUdpPkts(int numPktsToSend) {
    auto vlanId = utility::firstVlanID(initialConfig());
    auto intfMac = utility::getFirstInterfaceMac(getProgrammedState());
    for (int i = 0; i < numPktsToSend; i++) {
      auto txPacket = utility::makeUDPTxPacket(
          getHwSwitch(),
          vlanId,
          intfMac,
          intfMac,
          folly::IPAddress("1::1"),
          folly::IPAddress("2::1"),
          10000,
          20000);
      getHwSwitch()->sendPacketSwitchedSync(std::move(txPacket));
    }
  }

  uint64_t getSampledPackets() {
    return std::get<0>(utility::getCpuQueueOutPacketsAndBytes(
        getHwSwitch(), utility::kCoppLowPriQueueId));
  }

  void runTest(bool enableSflow) {
    auto setup = [this, enableSflow]() {
      auto cfg = initialConfig();
      // Setting rate to 100% sampling when sflow is enabled. For small
      // number of packets, the ASIC is not very precise in terms
      // of number of packets it would be sample. So for e.g. if we set
      // a sampling rate to 10% and send a 100 packets. Slightly more than
      // 10 packets will be sampled.
      auto portCfg = utility::findCfgPort(cfg, masterLogicalPortIds()[0]);
      portCfg->sFlowIngressRate() = enableSflow ? 1 : 0;
      if (getHwSwitch()->getPlatform()->getAsic()->isSupported(
              HwAsic::Feature::EGRESS_SFLOW)) {
        portCfg->sFlowEgressRate() = enableSflow ? 1 : 0;
      }
      applyNewConfig(cfg);
      utility::EcmpSetupAnyNPorts6 ecmpHelper(getProgrammedState());
      resolveNeigborAndProgramRoutes(ecmpHelper, 1);
    };

    auto verify = [this, enableSflow]() {
      auto sampledPktsBefore = getSampledPackets();
      sendUdpPkts(10);
      // Since we use loopback ports, each packet should be sampled twice,
      // once on egress, when we send packet out and then on ingress, when
      // the packet loops back in.
      auto expectedSampledPackets = enableSflow ? 10 : 0;
      if (getHwSwitch()->getPlatform()->getAsic()->isSupported(
              HwAsic::Feature::EGRESS_SFLOW)) {
        expectedSampledPackets += enableSflow ? 10 : 0;
      }
      auto sampledPackets = sampledPktsBefore;
      // Post warm boot there is lag in packets getting sampled
      // and counters getting updated. So add retries to avoid flakiness
      auto retriesLeft = 5;
      while (retriesLeft--) {
        sampledPackets = getSampledPackets() - sampledPktsBefore;
        if (sampledPackets == expectedSampledPackets) {
          break;
        }
        XLOG(DBG2) << " Desired number of packets not sampled, retrying";
        sleep(1);
      }
      EXPECT_EQ(expectedSampledPackets, sampledPackets);
    };
    verifyAcrossWarmBoots(setup, verify);
  }

 private:
  HwSwitchEnsemble::Features featuresDesired() const override {
    return {HwSwitchEnsemble::LINKSCAN, HwSwitchEnsemble::PACKET_RX};
  }
}; // namespace facebook::fboss

TEST_F(HwSflowTest, SflowSamplingEnabled) {
  runTest(true);
}
TEST_F(HwSflowTest, SflowSamplingDisabled) {
  runTest(false);
}

} // namespace facebook::fboss
