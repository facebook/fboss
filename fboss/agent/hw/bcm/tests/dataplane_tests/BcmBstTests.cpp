/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/bcm/tests/BcmLinkStateDependentTests.h"
#include "fboss/agent/hw/bcm/tests/BcmTestTrafficPolicyUtils.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/hw/test/HwTestWatermarkUtils.h"
#include "fboss/agent/test/EcmpSetupHelper.h"

#include <folly/IPAddress.h>

namespace facebook::fboss {

class BcmBstTest : public BcmLinkStateDependentTests {
 private:
  cfg::SwitchConfig initialConfig() const override {
    auto cfg = utility::oneL3IntfConfig(
        getHwSwitch(), masterLogicalPortIds()[0], cfg::PortLoopbackMode::MAC);
    return cfg;
  }

  MacAddress kDstMac() const {
    return getPlatform()->getLocalMac();
  }

  template <typename ECMP_HELPER>
  std::shared_ptr<SwitchState> setupECMPForwarding(
      const ECMP_HELPER& ecmpHelper) {
    auto kEcmpWidthForTest = 1;
    auto newState = ecmpHelper.setupECMPForwarding(
        ecmpHelper.resolveNextHops(getProgrammedState(), kEcmpWidthForTest),
        kEcmpWidthForTest);
    applyNewState(newState);
    return getProgrammedState();
  }

  void sendUdpPkt(uint8_t dscpVal) {
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
        // Trailing 2 bits are for ECN
        static_cast<uint8_t>(dscpVal << 2),
        255,
        std::vector<uint8_t>(6000, 0xff));
    getHwSwitch()->sendPacketSwitchedSync(std::move(txPacket));
  }

 protected:
  /*
   * In practice, sending single packet usually (but not always) returned BST
   * value > 0 (usually 2, but  other times it was 0). Thus, send a large
   * number of packets to try to avoid the risk of flakiness.
   */
  void sendUdpPkts(uint8_t dscpVal, int cnt = 100) {
    for (int i = 0; i < cnt; i++) {
      sendUdpPkt(dscpVal);
    }
  }
  void _createAcl(uint8_t dscpVal, int queueId, const std::string& aclName) {
    auto newCfg{initialConfig()};
    utility::addDscpAclToCfg(&newCfg, aclName, dscpVal);
    utility::addQueueMatcher(&newCfg, aclName, queueId);
    applyNewConfig(newCfg);
  }

  void _setup(uint8_t kDscp) {
    utility::EcmpSetupAnyNPorts6 ecmpHelper6{getProgrammedState()};
    setupECMPForwarding(ecmpHelper6);
  }
  void assertWatermarks(PortID port, const std::vector<int>& queueIds) const {
    auto queueWaterMarks = utility::getQueueWaterMarks(
        getHwSwitch(),
        port,
        *(std::max_element(queueIds.begin(), queueIds.end())));
    for (auto queueId : queueIds) {
      XLOG(DBG0) << "queueId: " << queueId
                 << " Watermark: " << queueWaterMarks[queueId];
      EXPECT_GT(queueWaterMarks[queueId], 0);
    }
  }
};

TEST_F(BcmBstTest, VerifyDefaultQueue) {
  uint8_t kDscp = 10;
  int kDefQueueId = 0;

  auto setup = [=]() { _setup(kDscp); };
  auto verify = [=]() {
    sendUdpPkts(kDscp);
    assertWatermarks(masterLogicalPortIds()[0], {kDefQueueId});
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(BcmBstTest, VerifyNonDefaultQueue) {
  if (!isSupported(HwAsic::Feature::L3_QOS)) {
    return;
  }

  uint8_t kDscp = 10;
  int kNonDefQueueId = 1;
  auto kAclName = "acl1";

  auto setup = [=]() {
    _createAcl(kDscp, kNonDefQueueId, kAclName);
    _setup(kDscp);
  };
  auto verify = [=]() {
    sendUdpPkts(kDscp);
    assertWatermarks(masterLogicalPortIds()[0], {kNonDefQueueId});
  };

  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
