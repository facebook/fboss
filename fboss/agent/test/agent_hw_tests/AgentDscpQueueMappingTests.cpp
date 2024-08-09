/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/TxPacket.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/ResourceLibUtil.h"
#include "fboss/agent/test/utils/AclTestUtils.h"
#include "fboss/agent/test/utils/AsicUtils.h"
#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/agent/test/utils/OlympicTestUtils.h"
#include "fboss/agent/test/utils/PacketTestUtils.h"
#include "fboss/agent/test/utils/TrafficPolicyTestUtils.h"
#include "fboss/lib/CommonUtils.h"

namespace facebook::fboss {

class AgentDscpQueueMappingTestBase : public AgentHwTest {
 protected:
  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    return {production_features::ProductionFeature::L3_QOS};
  }

  void setupHelper() {
    utility::EcmpSetupAnyNPorts6 ecmpHelper(getProgrammedState());
    resolveNeigborAndProgramRoutes(ecmpHelper, kEcmpWidth);
  }

  void sendPacket(bool frontPanel, uint8_t ttl = 64) {
    auto vlanId = utility::firstVlanID(getProgrammedState());
    auto intfMac = utility::getFirstInterfaceMac(getProgrammedState());
    auto srcMac = utility::MacAddressGenerator().get(intfMac.u64NBO() + 1);
    auto txPacket = utility::makeUDPTxPacket(
        getSw(),
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
      utility::EcmpSetupAnyNPorts6 ecmpHelper(getProgrammedState());
      auto outPort = ecmpHelper.ecmpPortDescriptorAt(kEcmpWidth).phyPortID();
      getSw()->sendPacketOutOfPortAsync(std::move(txPacket), outPort);
    } else {
      getSw()->sendPacketSwitchedAsync(std::move(txPacket));
    }
  }

  folly::IPAddressV6 kSrcIP() {
    return folly::IPAddressV6("2620:0:1cfe:face:b00c::1");
  }

  folly::IPAddressV6 kDstIP() {
    utility::EcmpSetupAnyNPorts6 ecmpHelper(getProgrammedState());
    return ecmpHelper.ip(0);
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
};

class AgentDscpQueueMappingTest : public AgentDscpQueueMappingTestBase {
 protected:
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto cfg = utility::onePortPerInterfaceConfig(
        ensemble.getSw(),
        ensemble.masterLogicalPortIds(),
        true /*interfaceHasSubnet*/);
    // QosMap
    auto l3Asics = ensemble.getL3Asics();
    utility::addOlympicQosMaps(cfg, l3Asics);
    auto kAclName = "acl1";
    auto asic = utility::checkSameAndGetAsic(l3Asics);
    utility::addDscpAclToCfg(asic, &cfg, kAclName, kDscp());
    utility::addTrafficCounter(
        &cfg, kCounterName(), utility::getAclCounterTypes(l3Asics));
    utility::addQueueMatcher(
        &cfg, kAclName, kQueueId(), ensemble.isSai(), kCounterName());
    return cfg;
  }

  void verifyDscpQueueMappingHelper() {
    auto setup = [this]() { setupHelper(); };

    auto verify = [this]() {
      for (bool frontPanel : {false, true}) {
        auto beforeQueueOutPkts =
            getLatestPortStats(masterLogicalInterfacePortIds()[0])
                .get_queueOutPackets_()
                .at(kQueueId());

        sendPacket(frontPanel);

        WITH_RETRIES({
          auto afterQueueOutPkts =
              getLatestPortStats(masterLogicalInterfacePortIds()[0])
                  .get_queueOutPackets_()
                  .at(kQueueId());

          XLOG(DBG2) << "verify send packets "
                     << (frontPanel ? "out of port" : "switched")
                     << " beforeQueueOutPkts = " << beforeQueueOutPkts
                     << " afterQueueOutPkti = " << afterQueueOutPkts;

          /*
           * Packet from CPU / looped back from front panel port (with
           * pipeline bypass), hits ACL and increments counter (queue2Count
           * = 1). On some platforms, looped back packets for unknown MACs
           * are flooded and counted on queue *before* the split horizon
           * check. This packet will match the DSCP based ACL and thus
           * increment the queue2Count = 2.
           */
          EXPECT_EVENTUALLY_GE(afterQueueOutPkts - beforeQueueOutPkts, 1);
        });
      }
    };

    verifyAcrossWarmBoots(setup, verify);
  }
};

class AgentAclAndDscpQueueMappingTest : public AgentDscpQueueMappingTestBase {
 protected:
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto cfg = utility::onePortPerInterfaceConfig(
        ensemble.getSw(),
        ensemble.masterLogicalPortIds(),
        true /*interfaceHasSubnet*/);
    // QosMap
    utility::addOlympicQosMaps(cfg, ensemble.getL3Asics());

    // ACL
    auto* acl = utility::addAcl(&cfg, "acl0");
    cfg::Ttl ttl; // Match packets with hop limit > 127
    std::tie(*ttl.value(), *ttl.mask()) = std::make_tuple(0x80, 0x80);
    acl->ttl() = ttl;
    auto l3Asics = ensemble.getL3Asics();
    auto asic = utility::checkSameAndGetAsic(l3Asics);
    utility::addEtherTypeToAcl(asic, acl, cfg::EtherType::IPv6);
    utility::addAclStat(
        &cfg,
        "acl0",
        kCounterName(),
        utility::getAclCounterTypes(ensemble.getL3Asics()));
    return cfg;
  }

  void verifyAclAndQosMapHelper() {
    auto setup = [this]() { setupHelper(); };

    auto verify = [this]() {
      for (bool frontPanel : {false, true}) {
        XLOG(DBG2) << "verify send packets "
                   << (frontPanel ? "out of port" : "switched");
        auto beforeQueueOutPkts =
            getLatestPortStats(masterLogicalInterfacePortIds()[0])
                .get_queueOutPackets_()
                .at(kQueueId());
        auto beforeAclInOutPkts =
            utility::getAclInOutPackets(getSw(), kCounterName());

        XLOG(DBG2) << "beforeQueueOutPkts = " << beforeQueueOutPkts
                   << " beforeAclInOutPkts = " << beforeAclInOutPkts;

        sendPacket(frontPanel, 255 /* ttl, > 127 to match ACL */);

        WITH_RETRIES({
          auto afterQueueOutPkts =
              getLatestPortStats(masterLogicalInterfacePortIds()[0])
                  .get_queueOutPackets_()
                  .at(kQueueId());
          auto afterAclInOutPkts =
              utility::getAclInOutPackets(getSw(), kCounterName());

          XLOG(DBG2) << "afterQueueOutPkts = " << afterQueueOutPkts
                     << " afterAclInOutPkts = " << afterAclInOutPkts;

          EXPECT_EVENTUALLY_EQ(1, afterQueueOutPkts - beforeQueueOutPkts);
          // On some ASICs looped back pkt hits the ACL before being
          // dropped in the ingress pipeline, hence GE
          EXPECT_EVENTUALLY_GE(afterAclInOutPkts, beforeAclInOutPkts + 1);
          // At most we should get a pkt bump of 2
          EXPECT_EVENTUALLY_LE(afterAclInOutPkts, beforeAclInOutPkts + 2);
        });
      }
    };

    verifyAcrossWarmBoots(setup, verify);
  }
};

class AgentAclConflictAndDscpQueueMappingTest
    : public AgentDscpQueueMappingTestBase {
 protected:
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto cfg = utility::onePortPerInterfaceConfig(
        ensemble.getSw(),
        ensemble.masterLogicalPortIds(),
        true /*interfaceHasSubnet*/);

    auto l3Asics = ensemble.getL3Asics();
    auto asic = utility::checkSameAndGetAsic(l3Asics);
    // The QoS map sends packets to queue kQueueIdQosMap() i.e. 7,
    // The ACL sends them to queue kQueueIdAcl() i.e. 2.
    // QosMap
    utility::addOlympicQosMaps(cfg, l3Asics);

    // ACL
    utility::addDscpAclToCfg(asic, &cfg, "acl0", kDscp());
    utility::addTrafficCounter(
        &cfg, kCounterName(), utility::getAclCounterTypes(l3Asics));
    utility::addQueueMatcher(
        &cfg, "acl0", kQueueIdAcl(), ensemble.isSai(), kCounterName());
    return cfg;
  }

  void verifyAclAndQosMapConflictHelper() {
    auto setup = [this]() { setupHelper(); };

    auto verify = [this]() {
      for (bool frontPanel : {false, true}) {
        XLOG(DBG2) << "verify send packets "
                   << (frontPanel ? "out of port" : "switched");
        auto beforeQueueOutPktsAcl =
            getLatestPortStats(masterLogicalInterfacePortIds()[0])
                .get_queueOutPackets_()
                .at(kQueueIdAcl());
        auto beforeQueueOutPktsQosMap =
            getLatestPortStats(masterLogicalInterfacePortIds()[0])
                .get_queueOutPackets_()
                .at(kQueueIdQosMap());
        auto beforeAclInOutPkts =
            utility::getAclInOutPackets(getSw(), kCounterName());

        XLOG(DBG2) << "beforeQueueOutPktsAcl = " << beforeQueueOutPktsAcl
                   << " beforeQueueOutPktsQosMap = " << beforeQueueOutPktsQosMap
                   << " beforeAclInOutPkts = " << beforeAclInOutPkts;

        sendPacket(frontPanel);

        WITH_RETRIES({
          auto afterQueueOutPktsAcl =
              getLatestPortStats(masterLogicalInterfacePortIds()[0])
                  .get_queueOutPackets_()
                  .at(kQueueIdAcl());
          auto afterQueueOutPktsQosMap =
              getLatestPortStats(masterLogicalInterfacePortIds()[0])
                  .get_queueOutPackets_()
                  .at(kQueueIdQosMap());

          auto afterAclInOutPkts =
              utility::getAclInOutPackets(getSw(), kCounterName());

          XLOG(DBG2) << "afterQueueOutPktsAcl = " << afterQueueOutPktsAcl
                     << " afterQueueOutPktsQosMap = " << afterQueueOutPktsQosMap
                     << " afterAclInOutPkts = " << afterAclInOutPkts;

          // The ACL overrides the decision of the QoS map
          EXPECT_EVENTUALLY_EQ(
              0, afterQueueOutPktsQosMap - beforeQueueOutPktsQosMap);

          /*
           * Packet from CPU / looped back from front panel port (with
           * pipeline bypass), hits ACL and increments counter
           * (queue2Count = 1). On some platforms, looped back packets for
           * unknown MACs are flooded and counted on queue *before* the
           * split horizon check. This packet will match the DSCP based
           * ACL and thus increment the queue2Count = 2.
           */
          EXPECT_EVENTUALLY_GE(afterQueueOutPktsAcl - beforeQueueOutPktsAcl, 1);

          // On some ASICs looped back pkt hits the ACL before being
          // dropped in the ingress pipeline, hence GE
          EXPECT_EVENTUALLY_GE(afterAclInOutPkts, beforeAclInOutPkts + 1);
          // At most we should get a pkt bump of 2
          EXPECT_EVENTUALLY_LE(afterAclInOutPkts, beforeAclInOutPkts + 2);
        });
      }
    };

    verifyAcrossWarmBoots(setup, verify);
  }
};

// Verify that traffic arriving on front panel/cpu port egresses via queue
// based on DSCP
TEST_F(AgentDscpQueueMappingTest, VerifyDscpQueueMapping) {
  verifyDscpQueueMappingHelper();
}

// Verify that traffic arriving on front panel/cpu port with non-conflicting
// ACL (counter based on ttl) and QoS Map (DSCP to Queue)
TEST_F(AgentAclAndDscpQueueMappingTest, VerifyAclAndQosMap) {
  verifyAclAndQosMapHelper();
}

// Verify that traffic arriving on front panel/cpu port with conflicting ACL
// (DSCP to queue) and QoS Map (DSCP to Queue)
TEST_F(AgentAclConflictAndDscpQueueMappingTest, VerifyAclAndQosMapConflict) {
  verifyAclAndQosMapConflictHelper();
}

} // namespace facebook::fboss
