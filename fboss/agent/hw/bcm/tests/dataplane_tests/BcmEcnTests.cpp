/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/bcm/BcmAclTable.h"
#include "fboss/agent/hw/bcm/tests/BcmLinkStateDependentTests.h"
#include "fboss/agent/hw/bcm/tests/BcmTestTrafficPolicyUtils.h"
#include "fboss/agent/hw/bcm/tests/BcmTestUtils.h"
#include "fboss/agent/hw/bcm/tests/dataplane_tests/BcmQosUtils.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/test/EcmpSetupHelper.h"

#include <folly/IPAddress.h>

namespace facebook::fboss {

class BcmEcnTest : public BcmLinkStateDependentTests {
 protected:
  cfg::SwitchConfig initialConfig() const override {
    auto cfg = utility::oneL3IntfConfig(
        getHwSwitch(), masterLogicalPortIds()[0], cfg::PortLoopbackMode::MAC);

    if (isSupported(HwAsic::Feature::L3_QOS)) {
      utility::addDscpAclToCfg(&cfg, kAclName(), kEcnDscp());
      utility::addQueueMatcher(&cfg, kAclName(), kEcnQueueId());
      _createEcnQueue(&cfg);
    }

    return cfg;
  }

  uint8_t kEcnDscp() const {
    return 5;
  }

  int kEcnQueueId() const {
    return 2;
  }

  std::string kAclName() const {
    return "acl1";
  }

  template <typename ECMP_HELPER>
  void setupECMPForwarding(const ECMP_HELPER& ecmpHelper, int ecmpWidth) {
    auto newState = ecmpHelper.setupECMPForwarding(
        ecmpHelper.resolveNextHops(getProgrammedState(), ecmpWidth), ecmpWidth);
    applyNewState(newState);
  }

  template <typename ECMP_HELPER>
  void disableTTLDecrements(const ECMP_HELPER& ecmpHelper) {
    for (const auto& nextHopIp : ecmpHelper.getNextHops()) {
      utility::disableTTLDecrements(
          getHwSwitch(),
          ecmpHelper.getRouterId(),
          folly::IPAddress(nextHopIp.ip));
    }
  }

  cfg::ActiveQueueManagement kGetEcnConfig() const {
    cfg::ActiveQueueManagement ecnAQM;
    cfg::LinearQueueCongestionDetection ecnLQCD;
    ecnLQCD.minimumLength = 208;
    ecnLQCD.maximumLength = 208;
    ecnAQM.detection.set_linear(ecnLQCD);
    ecnAQM.behavior = cfg::QueueCongestionBehavior::ECN;
    return ecnAQM;
  }

  void _createEcnQueue(cfg::SwitchConfig* config) const {
    std::vector<cfg::PortQueue> portQueues;

    cfg::PortQueue queue0;
    queue0.id = 0;
    queue0.name_ref() = "low-pri";
    queue0.streamType = cfg::StreamType::UNICAST;
    queue0.scheduling = cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN;
    portQueues.push_back(queue0);

    cfg::PortQueue queue2;
    queue2.id = kEcnQueueId();
    queue2.name_ref() = "ecn1";
    queue2.streamType = cfg::StreamType::UNICAST;
    queue2.scheduling = cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN;
    queue2.aqms_ref() = {};
    queue2.aqms_ref()->push_back(kGetEcnConfig());
    portQueues.push_back(queue2);

    config->portQueueConfigs["queue_config"] = portQueues;
    config->ports[0].portQueueConfigName_ref() = "queue_config";
  }

  void sendEcnCapableUdpPkt(uint8_t dscpVal) {
    auto kECT1 = 0x01; // ECN capable transport ECT(1)
    auto vlanId = utility::firstVlanID(initialConfig());
    auto intfMac = utility::getInterfaceMac(getProgrammedState(), vlanId);

    auto txPacket = utility::makeUDPTxPacket(
        getHwSwitch(),
        vlanId,
        intfMac,
        intfMac,
        folly::IPAddressV6("2620:0:1cfe:face:b00c::3"),
        folly::IPAddressV6("2620:0:1cfe:face:b00c::4"),
        8000,
        8001,
        static_cast<uint8_t>((dscpVal << 2) | kECT1));

    getHwSwitch()->sendPacketSwitchedSync(std::move(txPacket));
  }

  /*
   * For congestion detection queue length of minLength = 128, and maxLength =
   * 128, a packet count of 128 has been enough to cause ECN marking. Inject
   * 128 * 2 packets to avoid test noise.
   */
  void sendEcnCapableUdpPkts(uint8_t dscpVal, int cnt = 256) {
    for (int i = 0; i < cnt; i++) {
      sendEcnCapableUdpPkt(dscpVal);
    }
  }
};

TEST_F(BcmEcnTest, verifyEcn) {
  if (!isSupported(HwAsic::Feature::L3_QOS)) {
    return;
  }

  auto setup = [=]() {
    auto kEcmpWidthForTest = 1;
    utility::EcmpSetupAnyNPorts6 ecmpHelper6{getProgrammedState(),
                                             getPlatform()->getLocalMac()};
    setupECMPForwarding(ecmpHelper6, kEcmpWidthForTest);
    disableTTLDecrements(ecmpHelper6);
  };

  auto verify = [=]() {
    sendEcnCapableUdpPkts(kEcnDscp());
    auto portStats = getLatestPortStats(masterLogicalPortIds()[0]);
    XLOG(DBG0) << " ECN counter: " << portStats.outEcnCounter_;
    EXPECT_GT(portStats.outEcnCounter_, 0);
  };

  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
