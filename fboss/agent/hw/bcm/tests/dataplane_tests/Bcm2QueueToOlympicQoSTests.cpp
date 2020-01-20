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
#include "fboss/agent/hw/bcm/tests/BcmPortUtils.h"
#include "fboss/agent/hw/bcm/tests/BcmTestTrafficPolicyUtils.h"
#include "fboss/agent/hw/bcm/tests/dataplane_tests/BcmQosUtils.h"
#include "fboss/agent/hw/bcm/tests/dataplane_tests/BcmTest2QueueUtils.h"
#include "fboss/agent/hw/bcm/tests/dataplane_tests/BcmTestOlympicUtils.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/test/EcmpSetupHelper.h"

#include "fboss/agent/hw/test/ConfigFactory.h"

#include <folly/IPAddress.h>

namespace facebook::fboss {

class Bcm2QueueToOlympicQoSTest : public BcmLinkStateDependentTests {
 protected:
  cfg::SwitchConfig initialConfig() const override {
    auto cfg = utility::oneL3IntfConfig(
        getHwSwitch(), masterLogicalPortIds()[0], cfg::PortLoopbackMode::MAC);
    return cfg;
  }

  template <typename ECMP_HELPER>
  void setupECMPForwarding(const ECMP_HELPER& ecmpHelper) {
    auto kEcmpWidthForTest = 1;
    auto newState = ecmpHelper.setupECMPForwarding(
        ecmpHelper.resolveNextHops(getProgrammedState(), kEcmpWidthForTest),
        kEcmpWidthForTest);
    applyNewState(newState);
  }

  std::unique_ptr<facebook::fboss::TxPacket> createUdpPkt(
      uint8_t dscpVal) const {
    auto cpuMac = getPlatform()->getLocalMac();

    return utility::makeUDPTxPacket(
        getHwSwitch(),
        VlanID(initialConfig().vlanPorts[0].vlanID),
        cpuMac,
        cpuMac,
        folly::IPAddressV6("2620:0:1cfe:face:b00c::3"),
        folly::IPAddressV6("2620:0:1cfe:face:b00c::4"),
        8000,
        8001,
        static_cast<uint8_t>(dscpVal << 2)); // Trailing 2 bits are for ECN
  }

  void sendUdpPkt(int hopLimit = 64) {
    getHwSwitch()->sendPacketSwitchedSync(createUdpPkt(hopLimit));
  }

  void _verifyDscpQueueMappingHelper(
      const std::map<int, std::vector<uint8_t>>& queueToDscp) {
    for (const auto& entry : queueToDscp) {
      auto queueId = entry.first;
      auto dscpVals = entry.second;

      for (const auto& dscpVal : dscpVals) {
        auto beforeQueueOutPkts = getLatestPortStats(masterLogicalPortIds()[0])
                                      .get_queueOutPackets_()
                                      .at(queueId);
        sendUdpPkt(dscpVal);
        auto afterQueueOutPkts = getLatestPortStats(masterLogicalPortIds()[0])
                                     .get_queueOutPackets_()
                                     .at(queueId);

        EXPECT_EQ(1, afterQueueOutPkts - beforeQueueOutPkts);
      }
    }
  }
};

TEST_F(Bcm2QueueToOlympicQoSTest, VerifyDscpQueueMapping) {
  if (!getPlatform()->isCosSupported()) {
    return;
  }

  auto setup = [=]() {
    utility::EcmpSetupAnyNPorts6 ecmpHelper6{getProgrammedState()};
    setupECMPForwarding(ecmpHelper6);

    auto newCfg{initialConfig()};
    utility::add2QueueConfig(&newCfg);
    utility::add2QueueAcls(&newCfg);
    applyNewConfig(newCfg);
  };

  auto verify = [=]() {
    _verifyDscpQueueMappingHelper(utility::k2QueueToDscp());
  };

  auto setupPostWarmboot = [=]() {
    auto newCfg{initialConfig()};
    utility::addOlympicQueueConfig(&newCfg);
    utility::addOlympicAcls(&newCfg);
    applyNewConfig(newCfg);
  };

  auto verifyPostWarmboot = [=]() {
    _verifyDscpQueueMappingHelper(utility::kOlympicQueueToDscp());
  };

  verifyAcrossWarmBoots(setup, verify, setupPostWarmboot, verifyPostWarmboot);
}

} // namespace facebook::fboss
