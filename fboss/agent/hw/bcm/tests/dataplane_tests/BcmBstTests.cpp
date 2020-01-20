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
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/tests/BcmLinkStateDependentTests.h"
#include "fboss/agent/hw/bcm/tests/BcmTestStatUtils.h"
#include "fboss/agent/hw/bcm/tests/BcmTestTrafficPolicyUtils.h"
#include "fboss/agent/hw/bcm/tests/dataplane_tests/BcmQosUtils.h"
#include "fboss/agent/hw/bcm/tests/dataplane_tests/BcmTestBstUtils.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/test/EcmpSetupHelper.h"

#include "fboss/agent/hw/test/ConfigFactory.h"

#include <folly/IPAddress.h>

namespace facebook::fboss {

class BcmBstTest : public BcmLinkStateDependentTests {
 protected:
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

  template <typename ECMP_HELPER>
  void disableTTLDecrements(const ECMP_HELPER& ecmpHelper) {
    for (const auto& nextHopIp : ecmpHelper.getNextHops()) {
      utility::disableTTLDecrements(
          getHwSwitch(),
          ecmpHelper.getRouterId(),
          folly::IPAddress(nextHopIp.ip));
    }
  }

  void _createAcl(uint8_t dscpVal, int queueId, const std::string& aclName) {
    auto newCfg{initialConfig()};
    utility::addDscpAclToCfg(&newCfg, aclName, dscpVal);
    utility::addQueueMatcher(&newCfg, aclName, queueId);
    applyNewConfig(newCfg);
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
        static_cast<uint8_t>(dscpVal << 2)); // Trailing 2 bits are for ECN
    getHwSwitch()->sendPacketSwitchedSync(std::move(txPacket));
  }

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

  void _setup(uint8_t kDscp) {
    utility::EcmpSetupAnyNPorts6 ecmpHelper6{getProgrammedState(),
                                             getPlatform()->getLocalMac()};
    setupECMPForwarding(ecmpHelper6);
    disableTTLDecrements(ecmpHelper6);
    sendUdpPkts(kDscp);
  }
};

TEST_F(BcmBstTest, VerifyDefaultQueue) {
  // NOTE: failures in this test often suggest unexpected port flaps
  // on warm boot.
  uint8_t kDscp = 10;
  int kDefQueueId = 0;

  auto setup = [=]() { _setup(kDscp); };
  auto verify = [=]() {
    utility::verifyBstStatsReported(
        getHwSwitch(),
        masterLogicalPortIds()[0],
        std::vector<int>{kDefQueueId});
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(BcmBstTest, VerifyNonDefaultQueue) {
  // NOTE: failures in this test often suggest unexpected port flaps
  // on warm boot.
  if (!getPlatform()->isCosSupported()) {
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
    utility::verifyBstStatsReported(
        getHwSwitch(),
        masterLogicalPortIds()[0],
        std::vector<int>{kNonDefQueueId});
  };

  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
