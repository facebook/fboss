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
#include "fboss/agent/hw/test/HwPortUtils.h"
#include "fboss/agent/hw/test/HwTestAclUtils.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/hw/test/TrafficPolicyUtils.h"
#include "fboss/agent/test/EcmpSetupHelper.h"

#include <folly/IPAddress.h>

#include "fboss/agent/gen-cpp2/switch_config_types.h"

namespace facebook::fboss {

class HwDscpQueueMappingTest : public HwLinkStateDependentTest {
 protected:
  cfg::SwitchConfig initialConfig() const override {
    auto cfg = utility::oneL3IntfConfig(
        getHwSwitch(), masterLogicalPortIds()[0], cfg::PortLoopbackMode::MAC);
    return cfg;
  }

  int kEcmpWidthForTest() const {
    return 1;
  }

  int16_t kDscp() const {
    return 48;
  }

  int kQueueId() const {
    return 7;
  }

  std::string kCounterName() const {
    return folly::to<std::string>("dscp", kQueueId(), "_counter");
  }

  MacAddress kDstMac() const {
    return getPlatform()->getLocalMac();
  }

  void setupECMPForwarding(int ecmpWidth) {
    utility::EcmpSetupAnyNPorts6 ecmpHelper(getProgrammedState(), kRid);
    auto newState = ecmpHelper.setupECMPForwarding(
        ecmpHelper.resolveNextHops(getProgrammedState(), ecmpWidth), ecmpWidth);
    applyNewState(newState);
  }

  std::unique_ptr<facebook::fboss::TxPacket> createUdpPkt(
      int hopLimit = 64) const {
    auto vlanId = utility::firstVlanID(initialConfig());
    auto intfMac = utility::getInterfaceMac(getProgrammedState(), vlanId);

    return utility::makeUDPTxPacket(
        getHwSwitch(),
        vlanId,
        intfMac,
        intfMac,
        folly::IPAddressV6("2620:0:1cfe:face:b00c::3"),
        folly::IPAddressV6("2620:0:1cfe:face:b00c::4"),
        8000,
        8001,
        kDscp() << 2, // Trailing 2 bits are for ECN
        hopLimit);
  }

  void sendUdpPkt(int hopLimit = 64) {
    getHwSwitchEnsemble()->ensureSendPacketSwitched(createUdpPkt(hopLimit));
  }

 private:
  const RouterID kRid{0};
};

TEST_F(HwDscpQueueMappingTest, CheckDscpQueueMapping) {
  if (!isSupported(HwAsic::Feature::L3_QOS)) {
    return;
  }
  auto setup = [=]() {
    setupECMPForwarding(kEcmpWidthForTest());
    auto kAclName = "acl1";
    auto newCfg{initialConfig()};
    utility::addDscpAclToCfg(&newCfg, kAclName, kDscp());
    utility::addTrafficCounter(&newCfg, kCounterName());
    utility::addQueueMatcher(&newCfg, kAclName, kQueueId(), kCounterName());
    applyNewConfig(newCfg);
  };

  auto verify = [=]() {
    auto beforeQueueOutPkts = getLatestPortStats(masterLogicalPortIds()[0])
                                  .get_queueOutPackets_()
                                  .at(kQueueId());
    sendUdpPkt();
    auto afterQueueOutPkts = getLatestPortStats(masterLogicalPortIds()[0])
                                 .get_queueOutPackets_()
                                 .at(kQueueId());

    EXPECT_EQ(1, afterQueueOutPkts - beforeQueueOutPkts);
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwDscpQueueMappingTest, AclAndQosMap) {
  if (!isSupported(HwAsic::Feature::L3_QOS)) {
    return;
  }
  auto setup = [=]() {
    setupECMPForwarding(kEcmpWidthForTest());
    auto newCfg{initialConfig()};
    utility::addDscpQosPolicy(&newCfg, "qp1", {{kQueueId(), {kDscp()}}});
    utility::setCPUQosPolicy(&newCfg, "qp1");

    auto* acl = utility::addAcl(&newCfg, "acl0");
    cfg::Ttl ttl; // Match packets with hop limit > 127
    std::tie(*ttl.value_ref(), *ttl.mask_ref()) = std::make_tuple(0x80, 0x80);
    acl->ttl_ref() = ttl;
    utility::addAclStat(&newCfg, "acl0", kCounterName());
    applyNewConfig(newCfg);
  };

  auto verify = [=]() {
    auto beforeQueueOutPkts = getLatestPortStats(masterLogicalPortIds()[0])
                                  .get_queueOutPackets_()
                                  .at(kQueueId());
    auto beforeAclInOutPkts = utility::getAclInOutPackets(
        getHwSwitch(), getProgrammedState(), "acl0", kCounterName());
    sendUdpPkt(255);
    auto afterQueueOutPkts = getLatestPortStats(masterLogicalPortIds()[0])
                                 .get_queueOutPackets_()
                                 .at(kQueueId());
    auto afterAclInOutPkts = utility::getAclInOutPackets(
        getHwSwitch(), getProgrammedState(), "acl0", kCounterName());
    EXPECT_EQ(1, afterQueueOutPkts - beforeQueueOutPkts);
    EXPECT_EQ(2, afterAclInOutPkts - beforeAclInOutPkts);
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwDscpQueueMappingTest, AclAndQosMapConflict) {
  if (!isSupported(HwAsic::Feature::L3_QOS)) {
    return;
  }
  // The QoS map sends packets to queue 7 while the ACL sends them to queue 2
  constexpr auto kQueueIdAcl = 2;
  constexpr auto kQueueIdQosMap = 7;
  auto setup = [=]() {
    setupECMPForwarding(kEcmpWidthForTest());
    auto newCfg{initialConfig()};
    utility::addDscpQosPolicy(&newCfg, "qp1", {{kQueueIdQosMap, {kDscp()}}});
    utility::setCPUQosPolicy(&newCfg, "qp1");
    utility::addDscpAclToCfg(&newCfg, "acl0", kDscp());
    utility::addTrafficCounter(&newCfg, kCounterName());
    utility::addQueueMatcher(&newCfg, "acl0", kQueueIdAcl, kCounterName());
    applyNewConfig(newCfg);
  };

  auto verify = [=]() {
    auto beforeQueueOutPktsAcl = getLatestPortStats(masterLogicalPortIds()[0])
                                     .get_queueOutPackets_()
                                     .at(kQueueIdAcl);
    auto beforeQueueOutPktsQosMap =
        getLatestPortStats(masterLogicalPortIds()[0])
            .get_queueOutPackets_()
            .at(kQueueIdQosMap);
    auto beforeAclInOutPkts = utility::getAclInOutPackets(
        getHwSwitch(), getProgrammedState(), "acl0", kCounterName());
    sendUdpPkt();
    auto afterQueueOutPktsAcl = getLatestPortStats(masterLogicalPortIds()[0])
                                    .get_queueOutPackets_()
                                    .at(kQueueIdAcl);
    auto afterQueueOutPktsQosMap = getLatestPortStats(masterLogicalPortIds()[0])
                                       .get_queueOutPackets_()
                                       .at(kQueueIdQosMap);

    auto afterAclInOutPkts = utility::getAclInOutPackets(
        getHwSwitch(), getProgrammedState(), "acl0", kCounterName());

    // The ACL overrides the decision of the QoS map
    EXPECT_EQ(0, afterQueueOutPktsQosMap - beforeQueueOutPktsQosMap);
    EXPECT_EQ(1, afterQueueOutPktsAcl - beforeQueueOutPktsAcl);
    EXPECT_EQ(2, afterAclInOutPkts - beforeAclInOutPkts);
  };

  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
