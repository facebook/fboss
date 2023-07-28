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
#include "fboss/agent/hw/test/HwTestAclUtils.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/hw/test/TrafficPolicyUtils.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/ResourceLibUtil.h"

namespace facebook::fboss {

class HwDscpQueueMappingTest : public HwLinkStateDependentTest {
 protected:
  void SetUp() override {
    HwLinkStateDependentTest::SetUp();
    helper_ = std::make_unique<utility::EcmpSetupAnyNPorts6>(
        getProgrammedState(), RouterID(0));
  }
  cfg::SwitchConfig initialConfig() const override {
    auto cfg = utility::onePortPerInterfaceConfig(
        getHwSwitch(),
        masterLogicalPortIds(),
        getAsic()->desiredLoopbackModes());
    return cfg;
  }

  void setupHelper() {
    resolveNeigborAndProgramRoutes(*helper_, kEcmpWidth);
  }

  void verifyDscpQueueMappingHelper(bool frontPanel) {
    if (!isSupported(HwAsic::Feature::L3_QOS)) {
#if defined(GTEST_SKIP)
      GTEST_SKIP();
#endif
      return;
    }

    auto setup = [this]() {
      setupHelper();

      auto kAclName = "acl1";
      auto newCfg{initialConfig()};
      utility::addDscpAclToCfg(&newCfg, kAclName, kDscp());
      std::vector<cfg::CounterType> counterTypes{cfg::CounterType::PACKETS};
      utility::addTrafficCounter(&newCfg, kCounterName(), counterTypes);
      utility::addQueueMatcher(&newCfg, kAclName, kQueueId(), kCounterName());

      applyNewConfig(newCfg);
    };

    auto verify = [this, frontPanel]() {
      auto beforeQueueOutPkts =
          getLatestPortStats(masterLogicalInterfacePortIds()[0])
              .get_queueOutPackets_()
              .at(kQueueId());

      sendPacket(frontPanel);

      auto afterQueueOutPkts =
          getLatestPortStats(masterLogicalInterfacePortIds()[0])
              .get_queueOutPackets_()
              .at(kQueueId());

      /*
       * Packet from CPU / looped back from front panel port (with pipeline
       * bypass), hits ACL and increments counter (queue2Count = 1).
       * On some platforms, looped back packets for unknown MACs are flooded
       * and counted on queue *before* the split horizon check.
       * This packet will match the DSCP based ACL and thus increment the
       * queue2Count = 2.
       */
      EXPECT_GE(afterQueueOutPkts - beforeQueueOutPkts, 1);
    };

    verifyAcrossWarmBoots(setup, verify);
  }

  void verifyAclAndQosMapHelper(bool frontPanel) {
    if (!isSupported(HwAsic::Feature::L3_QOS)) {
#if defined(GTEST_SKIP)
      GTEST_SKIP();
#endif
      return;
    }

    auto setup = [this]() {
      setupHelper();

      auto newCfg{initialConfig()};

      // QosMap
      utility::addDscpQosPolicy(&newCfg, "qp1", {{kQueueId(), {kDscp()}}});
      utility::setCPUQosPolicy(&newCfg, "qp1");
      utility::setDefaultQosPolicy(&newCfg, "qp1");

      // ACL
      auto* acl = utility::addAcl(&newCfg, "acl0");
      cfg::Ttl ttl; // Match packets with hop limit > 127
      std::tie(*ttl.value(), *ttl.mask()) = std::make_tuple(0x80, 0x80);
      acl->ttl() = ttl;
      utility::addAclStat(&newCfg, "acl0", kCounterName());

      applyNewConfig(newCfg);
    };

    auto verify = [this, frontPanel]() {
      auto beforeQueueOutPkts =
          getLatestPortStats(masterLogicalInterfacePortIds()[0])
              .get_queueOutPackets_()
              .at(kQueueId());
      auto beforeAclInOutPkts = utility::getAclInOutPackets(
          getHwSwitch(), getProgrammedState(), "acl0", kCounterName());

      sendPacket(frontPanel, 255 /* ttl, > 127 to match ACL */);

      auto afterQueueOutPkts =
          getLatestPortStats(masterLogicalInterfacePortIds()[0])
              .get_queueOutPackets_()
              .at(kQueueId());
      auto afterAclInOutPkts = utility::getAclInOutPackets(
          getHwSwitch(), getProgrammedState(), "acl0", kCounterName());

      EXPECT_EQ(1, afterQueueOutPkts - beforeQueueOutPkts);
      EXPECT_EQ(2, afterAclInOutPkts - beforeAclInOutPkts);
    };

    verifyAcrossWarmBoots(setup, verify);
  }

  void verifyAclAndQosMapConflictHelper(bool frontPanel) {
    if (!isSupported(HwAsic::Feature::L3_QOS)) {
#if defined(GTEST_SKIP)
      GTEST_SKIP();
#endif
      return;
    }

    auto setup = [this]() {
      setupHelper();

      auto newCfg{initialConfig()};

      // The QoS map sends packets to queue kQueueIdQosMap() i.e. 7,
      // The ACL sends them to queue kQueueIdAcl() i.e. 2.

      // QosMap
      utility::addDscpQosPolicy(
          &newCfg, "qp1", {{kQueueIdQosMap(), {kDscp()}}});
      utility::setCPUQosPolicy(&newCfg, "qp1");
      utility::setDefaultQosPolicy(&newCfg, "qp1");

      // ACL
      utility::addDscpAclToCfg(&newCfg, "acl0", kDscp());
      std::vector<cfg::CounterType> counterTypes{cfg::CounterType::PACKETS};
      utility::addTrafficCounter(&newCfg, kCounterName(), counterTypes);
      utility::addQueueMatcher(&newCfg, "acl0", kQueueIdAcl(), kCounterName());

      applyNewConfig(newCfg);
    };

    auto verify = [this, frontPanel]() {
      auto beforeQueueOutPktsAcl =
          getLatestPortStats(masterLogicalInterfacePortIds()[0])
              .get_queueOutPackets_()
              .at(kQueueIdAcl());
      auto beforeQueueOutPktsQosMap =
          getLatestPortStats(masterLogicalInterfacePortIds()[0])
              .get_queueOutPackets_()
              .at(kQueueIdQosMap());
      auto beforeAclInOutPkts = utility::getAclInOutPackets(
          getHwSwitch(), getProgrammedState(), "acl0", kCounterName());

      sendPacket(frontPanel);

      auto afterQueueOutPktsAcl =
          getLatestPortStats(masterLogicalInterfacePortIds()[0])
              .get_queueOutPackets_()
              .at(kQueueIdAcl());
      auto afterQueueOutPktsQosMap =
          getLatestPortStats(masterLogicalInterfacePortIds()[0])
              .get_queueOutPackets_()
              .at(kQueueIdQosMap());

      auto afterAclInOutPkts = utility::getAclInOutPackets(
          getHwSwitch(), getProgrammedState(), "acl0", kCounterName());

      // The ACL overrides the decision of the QoS map
      EXPECT_EQ(0, afterQueueOutPktsQosMap - beforeQueueOutPktsQosMap);

      /*
       * Packet from CPU / looped back from front panel port (with pipeline
       * bypass), hits ACL and increments counter (queue2Count = 1).
       * On some platforms, looped back packets for unknown MACs are flooded
       * and counted on queue *before* the split horizon check.
       * This packet will match the DSCP based ACL and thus increment the
       * queue2Count = 2.
       */
      EXPECT_GE(afterQueueOutPktsAcl - beforeQueueOutPktsAcl, 1);

      EXPECT_EQ(2, afterAclInOutPkts - beforeAclInOutPkts);
    };

    verifyAcrossWarmBoots(setup, verify);
  }

 private:
  void sendPacket(bool frontPanel, uint8_t ttl = 64) {
    auto vlanId = utility::firstVlanID(getProgrammedState());
    auto intfMac = utility::getFirstInterfaceMac(getProgrammedState());
    auto srcMac = utility::MacAddressGenerator().get(intfMac.u64NBO() + 1);
    auto txPacket = utility::makeUDPTxPacket(
        getHwSwitch(),
        vlanId,
        srcMac, // src mac
        intfMac, // dst mac
        kSrcIP(),
        kDstIP(),
        8000, // l4 src port
        8001, // l4 dst port
        kDscp() << 2, // Trailing 2 bits are for ECN
        ttl);

    // port is in LB mode, so it will egress and immediately loop back.
    // Since it is not re-written, it should hit the pipeline as if it
    // ingressed on the port, and be properly queued.
    if (frontPanel) {
      auto outPort = helper_->ecmpPortDescriptorAt(kEcmpWidth).phyPortID();
      getHwSwitchEnsemble()->ensureSendPacketOutOfPort(
          std::move(txPacket), outPort);
    } else {
      getHwSwitchEnsemble()->ensureSendPacketSwitched(std::move(txPacket));
    }
  }

  folly::IPAddressV6 kSrcIP() {
    return folly::IPAddressV6("2620:0:1cfe:face:b00c::1");
  }

  folly::IPAddressV6 kDstIP() {
    return helper_->ip(0);
  }

  folly::MacAddress kMacAddress() {
    return folly::MacAddress("0:2:3:4:5:10");
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

  int kQueueIdQosMap() const {
    return 7;
  }

  int kQueueIdAcl() const {
    return 2;
  }

  static inline constexpr auto kEcmpWidth = 1;
  const VlanID kVlanID{utility::kBaseVlanId};
  const InterfaceID kIntfID{utility::kBaseVlanId};
  std::unique_ptr<utility::EcmpSetupAnyNPorts6> helper_;
};

// Verify that traffic arriving on front panel port egresses via queue based on
// DSCP
TEST_F(HwDscpQueueMappingTest, VerifyDscpQueueMappingFrontPanel) {
  verifyDscpQueueMappingHelper(true /* front panel port */);
}

// Verify that traffic originating from CPU egresses via queue based on DSCP
TEST_F(HwDscpQueueMappingTest, VerifyDscpQueueMappingCpu) {
  verifyDscpQueueMappingHelper(false /* CPU port */);
}

// Verify that traffic arriving on front panel port with non-conflicting ACL
// (counter based on ttl) and QoS Map (DSCP to Queue)
TEST_F(HwDscpQueueMappingTest, VerifyAclAndQosMapFrontPanel) {
  verifyAclAndQosMapHelper(true /* front panel port */);
}

// Verify that traffic originating from CPU with non-conflicting ACL (counter
// based on ttl) and QoS Map (DSCP to Queue)
TEST_F(HwDscpQueueMappingTest, VerifyAclAndQosMapCpu) {
  verifyAclAndQosMapHelper(false /* CPU port */);
}

// Verify that traffic arriving on front panel port with conflicting ACL (DSCP
// to queue) and QoS Map (DSCP to Queue)
TEST_F(HwDscpQueueMappingTest, VerifyAclAndQosMapConflictFrontPanel) {
  verifyAclAndQosMapConflictHelper(true /* front panel port */);
}

// Verify that traffic originating from CPU port with conflicting ACL (DSCP
// to queue) and QoS Map (DSCP to Queue)
TEST_F(HwDscpQueueMappingTest, VerifyAclAndQosMapConflictCpu) {
  verifyAclAndQosMapConflictHelper(false /* CPU port */);
}

} // namespace facebook::fboss
