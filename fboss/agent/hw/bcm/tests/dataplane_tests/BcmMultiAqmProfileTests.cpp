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
#include "fboss/agent/hw/bcm/tests/BcmTestUtils.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/hw/test/TrafficPolicyUtils.h"
#include "fboss/agent/hw/test/dataplane_tests/HwTestQosUtils.h"
#include "fboss/agent/test/EcmpSetupHelper.h"

#include <folly/IPAddress.h>

namespace facebook::fboss {

class BcmMultiAqmProfileTest : public BcmLinkStateDependentTests {
 protected:
  cfg::SwitchConfig initialConfig() const override {
    auto cfg = utility::oneL3IntfConfig(
        getHwSwitch(), masterLogicalPortIds()[0], cfg::PortLoopbackMode::MAC);

    if (isSupported(HwAsic::Feature::L3_QOS)) {
      auto qosPolicyName = "qp1";
      utility::setDefaultQosPolicy(&cfg, qosPolicyName);
      utility::addDscpQosPolicy(&cfg, qosPolicyName, {{kQueueId(), {kDscp()}}});
      _createMultiAqmProfileQueue(&cfg, masterLogicalPortIds()[0]);
    }

    return cfg;
  }

  uint8_t kDscp() const {
    return 10;
  }

  int kQueueId() const {
    return 1;
  }

  template <typename ECMP_HELPER>
  void setupECMPForwarding(const ECMP_HELPER& ecmpHelper, int ecmpWidth) {
    auto newState = ecmpHelper.setupECMPForwarding(
        ecmpHelper.resolveNextHops(getProgrammedState(), ecmpWidth), ecmpWidth);
    applyNewState(newState);
  }

  void _setup() {
    auto kEcmpWidthForTest = 1;
    utility::EcmpSetupAnyNPorts6 ecmpHelper6{getProgrammedState(),
                                             getPlatform()->getLocalMac()};
    setupECMPForwarding(ecmpHelper6, kEcmpWidthForTest);
    disableTTLDecrements(ecmpHelper6);
  }

  /*
   * For congestion detection queue length of minLength = 128, and maxLength =
   * 128, a packet count of 128 has been enough to cause ECN marking. Inject
   * 128 * 2 packets to avoid test noise.
   */
  void sendPkts(uint8_t trafficClass, int cnt = 256) {
    for (int i = 0; i < cnt; i++) {
      sendUdpPkt(trafficClass);
    }
  }

 private:
  template <typename ECMP_HELPER>
  void disableTTLDecrements(const ECMP_HELPER& ecmpHelper) {
    for (const auto& nextHop : ecmpHelper.getNextHops()) {
      utility::disableTTLDecrements(
          getHwSwitch(), ecmpHelper.getRouterId(), nextHop);
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

  cfg::ActiveQueueManagement kGetWredConfig() const {
    cfg::ActiveQueueManagement wredAQM;
    cfg::LinearQueueCongestionDetection wredLQCD;
    wredLQCD.minimumLength = 208;
    wredLQCD.maximumLength = 208;
    wredAQM.detection.set_linear(wredLQCD);
    wredAQM.behavior = cfg::QueueCongestionBehavior::EARLY_DROP;

    return wredAQM;
  }

  void _createMultiAqmProfileQueue(cfg::SwitchConfig* config, PortID portID)
      const {
    std::vector<cfg::PortQueue> portQueues;

    cfg::PortQueue queue1;
    queue1.id = kQueueId();
    queue1.name_ref() = "ecn1";
    queue1.streamType = cfg::StreamType::UNICAST;
    queue1.scheduling = cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN;
    queue1.aqms_ref() = {};
    queue1.aqms_ref()->push_back(kGetEcnConfig());
    queue1.aqms_ref()->push_back(kGetWredConfig());
    portQueues.push_back(queue1);

    config->portQueueConfigs_ref()["queue_config"] = portQueues;
    auto portCfg = std::find_if(
        config->ports_ref()->begin(),
        config->ports_ref()->end(),
        [&portID](auto& port) {
          return PortID(*port.logicalID_ref()) == portID;
        });
    portCfg->portQueueConfigName_ref() = "queue_config";
  }

  void sendUdpPkt(uint8_t trafficClass) {
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
        trafficClass);

    getHwSwitch()->sendPacketSwitchedSync(std::move(txPacket));
  }
};

TEST_F(BcmMultiAqmProfileTest, verifyWred) {
  if (!isSupported(HwAsic::Feature::L3_QOS)) {
    return;
  }

  auto setup = [this]() { _setup(); };

  auto verify = [this]() {
    // Non-ECT-marked traffic: should use WRED profile
    sendPkts(static_cast<uint8_t>(kDscp() << 2));
    auto portStats = getLatestPortStats(masterLogicalPortIds()[0]);
    XLOG(DBG0) << " ECN counter: " << *portStats.outEcnCounter__ref();
    EXPECT_EQ(*portStats.outEcnCounter__ref(), 0);
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(BcmMultiAqmProfileTest, verifyEcn) {
  if (!isSupported(HwAsic::Feature::L3_QOS)) {
    return;
  }

  auto setup = [this]() { _setup(); };

  auto verify = [this]() {
    // ECT-marked traffic: should use ECN profile
    auto kECT1 = 0x01; // ECN capable transport ECT(1)
    sendPkts(static_cast<uint8_t>((kDscp() << 2) | kECT1));
    auto portStats = getLatestPortStats(masterLogicalPortIds()[0]);
    XLOG(DBG0) << " ECN counter: " << *portStats.outEcnCounter__ref();
    EXPECT_GT(*portStats.outEcnCounter__ref(), 0);
  };

  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
