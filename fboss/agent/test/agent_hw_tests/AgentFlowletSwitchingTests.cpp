/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/test/agent_hw_tests/AgentArsBase.h"

#include "fboss/agent/AsicUtils.h"
#include "fboss/agent/EcmpResourceManager.h"
#include "fboss/agent/FibHelpers.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/utils/AclTestUtils.h"
#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/agent/test/utils/LoadBalancerTestUtils.h"
#include "fboss/agent/test/utils/MirrorTestUtils.h"
#include "fboss/agent/test/utils/ScaleTestUtils.h"
#include "fboss/agent/test/utils/UdfTestUtils.h"
#include "fboss/lib/CommonUtils.h"

DECLARE_bool(flowletSwitchingEnable);

const std::string kSflowMirrorName = "sflow_mirror";
const std::string sflowDestinationVIP = "2001::101";
const std::string aclMirror = "acl_mirror";
const std::string aclDestinationVIP = "2002::101";

namespace facebook::fboss {

class AgentFlowletSwitchingTest : public AgentArsBase {
 public:
  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    return {
        production_features::ProductionFeature::DLB,
        production_features::ProductionFeature::SINGLE_ACL_TABLE};
  }

 protected:
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto cfg = utility::onePortPerInterfaceConfig(
        ensemble.getSw(),
        ensemble.masterLogicalPortIds(),
        true /*interfaceHasSubnet*/);
    utility::addFlowletConfigs(
        cfg,
        ensemble.masterLogicalPortIds(),
        ensemble.isSai(),
        cfg::SwitchingMode::PER_PACKET_QUALITY);
    return cfg;
  }

  void setCmdLineFlagOverrides() const override {
    AgentHwTest::setCmdLineFlagOverrides();
    FLAGS_flowletSwitchingEnable = true;
    FLAGS_force_init_fp = false;
    FLAGS_enable_ecmp_random_spray = true;
  }
};

class AgentFlowletAclPriorityTest : public AgentFlowletSwitchingTest {
 public:
  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    return {
        production_features::ProductionFeature::DLB,
        production_features::ProductionFeature::UDF_WR_IMMEDIATE_ACL,
        production_features::ProductionFeature::SINGLE_ACL_TABLE};
  }
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto cfg = AgentFlowletSwitchingTest::initialConfig(ensemble);
    std::vector<std::string> udfGroups = {
        utility::kUdfAclRoceOpcodeGroupName,
        utility::kRoceUdfFlowletGroupName,
        utility::kUdfAclAethNakGroupName,
        utility::kUdfAclRethWrImmZeroGroupName,
    };
    addAclTableConfig(cfg, udfGroups);
    cfg.udfConfig() = utility::addUdfAclConfig(
        utility::kUdfOffsetBthOpcode | utility::kUdfOffsetBthReserved |
        utility::kUdfOffsetAethSyndrome | utility::kUdfOffsetRethDmaLength);
    return cfg;
  }
};

class AgentFlowletMirrorTest : public AgentFlowletSwitchingTest {
 public:
  // TH* supports upto 4 different source types to mirror to same egress port.
  // Here IFP mirror action and ingress port sflow actions can generate 2 copies
  // going to same VIP or different VIP (different egress port in the test)
  enum MirrorScope {
    MIRROR_ONLY,
    MIRROR_SFLOW_SAME_VIP,
    MIRROR_SFLOW_DIFFERENT_VIP,
  };
  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    return {
        production_features::ProductionFeature::DLB,
        production_features::ProductionFeature::SFLOWv6_SAMPLING,
        production_features::ProductionFeature::INGRESS_MIRRORING,
        production_features::ProductionFeature::SINGLE_ACL_TABLE};
  }

 protected:
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto cfg = AgentFlowletSwitchingTest::initialConfig(ensemble);
    std::vector<std::string> udfGroups = getUdfGroupsForAcl(AclType::UDF_NAK);
    addAclTableConfig(cfg, udfGroups);
    addAclAndStat(&cfg, AclType::UDF_NAK, ensemble.isSai());
    // overwrite existing traffic policy which only has a counter action
    // It is added in addAclAndStat above
    cfg.dataPlaneTrafficPolicy() = cfg::TrafficPolicyConfig();
    std::string counterName = getCounterName(AclType::UDF_NAK);
    utility::addAclMirrorAction(
        &cfg, getAclName(AclType::UDF_NAK), counterName, aclMirror);

    // mirror session for acl
    utility::configureSflowMirror(
        cfg, aclMirror, false /* truncate */, aclDestinationVIP, 6344);

    return cfg;
  }

  void verifyMirror(MirrorScope scope) {
    // In addition to counting ACL hit with verifyAcl, verify packet mirrored
    auto mirrorPort = helper_->ecmpPortDescriptorAt(1).phyPortID();
    auto sflowPort = helper_->ecmpPortDescriptorAt(2).phyPortID();
    auto pktsMirrorBefore =
        *getNextUpdatedPortStats(mirrorPort).outUnicastPkts__ref();
    auto pktsSflowBefore =
        *getNextUpdatedPortStats(sflowPort).outUnicastPkts__ref();

    verifyAcl(AclType::UDF_NAK);

    WITH_RETRIES({
      auto pktsMirrorAfter =
          *getNextUpdatedPortStats(mirrorPort).outUnicastPkts__ref();
      auto pktsSflowAfter =
          *getNextUpdatedPortStats(sflowPort).outUnicastPkts__ref();
      XLOG(DBG2) << "PacketMirrorCounter: " << pktsMirrorBefore << " -> "
                 << pktsMirrorAfter
                 << " PacketSflowCounter: " << pktsSflowBefore << " -> "
                 << pktsSflowAfter;
      if (scope == MirrorScope::MIRROR_ONLY) {
        EXPECT_EVENTUALLY_GT(pktsMirrorAfter, pktsMirrorBefore);
      } else if (scope == MirrorScope::MIRROR_SFLOW_SAME_VIP) {
        EXPECT_EVENTUALLY_GE(pktsMirrorAfter, pktsMirrorBefore + 2);
      } else if (scope == MirrorScope::MIRROR_SFLOW_DIFFERENT_VIP) {
        EXPECT_EVENTUALLY_GT(pktsMirrorAfter, pktsMirrorBefore);
        EXPECT_EVENTUALLY_GT(pktsSflowAfter, pktsSflowBefore);
      }
    });
  }
};

// empty to UDF A
TEST_F(AgentFlowletSwitchingTest, VerifyFlowletToUdfFlowlet) {
  flowletSwitchingAclHitHelper(AclType::FLOWLET, AclType::UDF_FLOWLET);
}

// empty to UDF A + B
TEST_F(AgentFlowletSwitchingTest, VerifyFlowletToUdfFlowletWithUdfAck) {
  flowletSwitchingAclHitHelper(
      AclType::FLOWLET, AclType::UDF_FLOWLET_WITH_UDF_ACK);
}

// UDF A to UDF B
TEST_F(AgentFlowletSwitchingTest, VerifyUdfAckToUdfFlowlet) {
  flowletSwitchingAclHitHelper(AclType::UDF_ACK, AclType::UDF_FLOWLET);
}

// UDF A to UDF A + B
TEST_F(AgentFlowletSwitchingTest, VerifyUdfFlowletToUdfFlowletWithUdfAck) {
  flowletSwitchingAclHitHelper(
      AclType::UDF_FLOWLET, AclType::UDF_FLOWLET_WITH_UDF_ACK);
}

// UDF A to UDF A + B + C
TEST_F(AgentFlowletSwitchingTest, VerifyUdfFlowletToUdfFlowletWithUdfNak) {
  flowletSwitchingAclHitHelper(
      AclType::UDF_FLOWLET, AclType::UDF_FLOWLET_WITH_UDF_NAK);
}

// UDF A + B + C to UDF B + C
TEST_F(AgentFlowletSwitchingTest, VerifyUdfFlowletWithUdfNakToUdfNak) {
  flowletSwitchingAclHitHelper(
      AclType::UDF_FLOWLET_WITH_UDF_NAK, AclType::UDF_NAK);
}

// UDF A + B to UDF B
TEST_F(AgentFlowletSwitchingTest, VerifyUdfFlowletWithUdfAckToUdfAck) {
  flowletSwitchingAclHitHelper(
      AclType::UDF_FLOWLET_WITH_UDF_ACK, AclType::UDF_ACK);
}

// UDF A to UDF B
TEST_F(AgentFlowletSwitchingTest, VerifyUdfFlowletToUdfAck) {
  flowletSwitchingAclHitHelper(AclType::UDF_FLOWLET, AclType::UDF_ACK);
}

// UDF A + B to empty
TEST_F(AgentFlowletSwitchingTest, VerifyUdfFlowletWithUdfAckToFlowlet) {
  flowletSwitchingAclHitHelper(
      AclType::UDF_FLOWLET_WITH_UDF_ACK, AclType::FLOWLET);
}

class AgentFlowletSprayTest : public AgentFlowletSwitchingTest {
 protected:
  void setCmdLineFlagOverrides() const override {
    AgentFlowletSwitchingTest::setCmdLineFlagOverrides();
    FLAGS_dlbResourceCheckEnable = false;
  }

  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    return {
        production_features::ProductionFeature::DLB,
        production_features::ProductionFeature::ECMP_RANDOM_SPRAY,
        production_features::ProductionFeature::ACL_COUNTER,
        production_features::ProductionFeature::SINGLE_ACL_TABLE};
  }
};

/* Add route 3001::1 > DLB
 * Add route 4001::1 > Spray
 *
 * Test 1:
 * Send 3001::1 AR=1 hit ACL 1, do DLB
 * Send 3001::1 AR=0 hit ACL 2, cancel spray, static ECMP
 *
 * Test 2:
 * Send 4001::1 AR=1 hit ACL 1, no DLB, random spray
 * Send 4001::1 AR=0 hit ACL 2, cancel spray, static ECMP
 */
TEST_F(AgentFlowletSprayTest, VerifyEcmpRandomSpray) {
  auto populatePortsAndDescs = [this](
                                   int start,
                                   int end,
                                   std::vector<PortID>& portIDs,
                                   flat_set<PortDescriptor>& portDescs) {
    std::vector<PortDescriptor> tempPortDescs;
    for (int w = start; w < end; ++w) {
      portIDs.push_back(helper_->ecmpPortDescriptorAt(w).phyPortID());
      tempPortDescs.emplace_back(helper_->ecmpPortDescriptorAt(w));
    }
    portDescs.insert(
        std::make_move_iterator(tempPortDescs.begin()),
        std::make_move_iterator(tempPortDescs.end()));
  };

  std::vector<PortID> dlbPortIDs;
  flat_set<PortDescriptor> dlbPortDescs;
  populatePortsAndDescs(0, 4, dlbPortIDs, dlbPortDescs);

  std::vector<PortID> sprayPortIDs;
  flat_set<PortDescriptor> randomSprayPortDescs;
  populatePortsAndDescs(4, 8, sprayPortIDs, randomSprayPortDescs);

  auto setup = [this, dlbPortDescs, randomSprayPortDescs]() {
    generatePrefixes();

    auto newCfg{initialConfig(*getAgentEnsemble())};
    std::vector<std::string> udfGroups =
        getUdfGroupsForAcl(AclType::UDF_FLOWLET);
    addAclTableConfig(newCfg, udfGroups);

    // 1. add higher priority DLB enable ACL matching on BTH reserved field
    addAclAndStat(&newCfg, AclType::UDF_FLOWLET, getAgentEnsemble()->isSai());
    // overwrite match action to also include Flowlet action
    newCfg.dataPlaneTrafficPolicy() = cfg::TrafficPolicyConfig();
    cfg::MatchAction matchAction = cfg::MatchAction();
    matchAction.flowletAction() = cfg::FlowletAction::FORWARD;
    auto aclCounterName = getCounterName(AclType::UDF_FLOWLET);
    matchAction.counter() = aclCounterName;
    utility::addMatcher(&newCfg, getAclName(AclType::UDF_FLOWLET), matchAction);

    // 2. add catch-all ECMP hash cancel ACL
    addAclAndStat(
        &newCfg, AclType::ECMP_HASH_CANCEL, getAgentEnsemble()->isSai());
    applyNewConfig(newCfg);

    // 200000 - 2000126 - DLB ECMP groups we don't care
    // 200127 - DLB ECMP group under test
    // 200128 - Random spray ECMP group under test
    const auto kMaxDlbEcmpGroup =
        utility::getMaxDlbEcmpGroups(getAgentEnsemble()->getL3Asics()) - 1;
    auto wrapper = getSw()->getRouteUpdater();
    std::vector<RoutePrefixV6> prefixes128 = {
        prefixes.begin(), prefixes.begin() + kMaxDlbEcmpGroup};
    std::vector<flat_set<PortDescriptor>> nhopSets128 = {
        nhopSets.begin(), nhopSets.begin() + kMaxDlbEcmpGroup};

    // generate route hitting DLB
    auto dlbPrefix = RoutePrefixV6{folly::IPAddressV6("3001::1"), 128};
    prefixes128.push_back(dlbPrefix);
    nhopSets128.push_back(dlbPortDescs);

    // generate route hitting random spray
    auto randomSprayPrefix = RoutePrefixV6{folly::IPAddressV6("4001::1"), 128};
    prefixes128.push_back(randomSprayPrefix);
    nhopSets128.push_back(randomSprayPortDescs);

    // 3. program routes
    helper_->programRoutes(&wrapper, nhopSets128, prefixes128);

    XLOG(DBG3) << "setting ECMP Member Status: ";
    applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      auto out = in->clone();
      for (const auto& [_, switchSetting] :
           std::as_const(*out->getSwitchSettings())) {
        auto newSwitchSettings = switchSetting->modify(&out);
        newSwitchSettings->setForceEcmpDynamicMemberUp(true);
      }
      return out;
    });
  };
  auto verify = [this, dlbPortIDs, sprayPortIDs]() {
    auto sendTrafficAndVerifyLB = [this](
                                      const folly::IPAddress& dstIp,
                                      int reserved,
                                      const std::vector<PortID>& ports,
                                      bool loadBalanceExpected) {
      auto dlbAclCountBefore = utility::getAclInOutPackets(
          getSw(), getCounterName(AclType::UDF_FLOWLET));
      auto cancelAclCountBefore = utility::getAclInOutPackets(
          getSw(), getCounterName(AclType::ECMP_HASH_CANCEL));

      auto egressPort = helper_->ecmpPortDescriptorAt(8).phyPortID();
      int packetCount = 200000;
      auto vlanId = getVlanIDForTx();
      auto intfMac =
          utility::getMacForFirstInterfaceWithPorts(getProgrammedState());
      utility::pumpRoCETraffic(
          true,
          utility::getAllocatePktFn(getAgentEnsemble()),
          utility::getSendPktFunc(getAgentEnsemble()),
          intfMac,
          vlanId,
          egressPort,
          folly::IPAddress("1001::1"),
          dstIp,
          utility::kUdfL4DstPort,
          255,
          std::nullopt,
          packetCount,
          utility::kUdfRoceOpcodeAck,
          reserved,
          {});

      WITH_RETRIES({
        auto dlbAclCountAfter = utility::getAclInOutPackets(
            getSw(), getCounterName(AclType::UDF_FLOWLET));
        auto cancelAclCountAfter = utility::getAclInOutPackets(
            getSw(), getCounterName(AclType::ECMP_HASH_CANCEL));

        XLOG(DBG2) << "\n"
                   << "aclPacketCounter("
                   << getCounterName(AclType::UDF_FLOWLET)
                   << "): " << dlbAclCountBefore << " -> " << (dlbAclCountAfter)
                   << "\n"
                   << "aclPacketCounter("
                   << getCounterName(AclType::ECMP_HASH_CANCEL)
                   << "): " << cancelAclCountBefore << " -> "
                   << (cancelAclCountAfter) << "\n";

        if (reserved) {
          EXPECT_EVENTUALLY_GE(
              dlbAclCountAfter, dlbAclCountBefore + packetCount);
          EXPECT_EVENTUALLY_EQ(cancelAclCountAfter, cancelAclCountBefore);
        } else {
          EXPECT_EVENTUALLY_GE(
              cancelAclCountAfter, cancelAclCountBefore + packetCount);
          EXPECT_EVENTUALLY_EQ(dlbAclCountAfter, dlbAclCountBefore);
        }

        auto portStats = getNextUpdatedPortStats(ports);
        for (const auto& [portId, stats] : portStats) {
          XLOG(DBG2) << "Ecmp egress Port: " << portId
                     << ", Count: " << *stats.outUnicastPkts__ref();
        }
        if (loadBalanceExpected) {
          EXPECT_EVENTUALLY_TRUE(utility::isLoadBalanced(portStats, 25));
        } else {
          EXPECT_EVENTUALLY_FALSE(utility::isLoadBalanced(portStats, 25));
        }
      });

      getAgentEnsemble()->clearPortStats();
    };

    // Test 1
    // Hit DLB ACL and do DLB
    sendTrafficAndVerifyLB(
        folly::IPAddress("3001::1"), utility::kRoceReserved, dlbPortIDs, true);
    // Miss DLB ACL and do static hash
    sendTrafficAndVerifyLB(folly::IPAddress("3001::1"), 0, dlbPortIDs, false);

    // Test 2
    // Hit DLB ACL but do random spray
    sendTrafficAndVerifyLB(
        folly::IPAddress("4001::1"),
        utility::kRoceReserved,
        sprayPortIDs,
        true);
    // Miss DLB ACL and do static hash
    sendTrafficAndVerifyLB(folly::IPAddress("4001::1"), 0, sprayPortIDs, false);
  };

  verifyAcrossWarmBoots(setup, verify);
};

TEST_F(AgentFlowletSwitchingTest, VerifyEcmp) {
  auto setup = [this]() {
    this->setup(kEcmpWidth);
    generateApplyConfig(AclType::FLOWLET);
  };

  auto verify = [this]() {
    setEcmpMemberStatus(getAgentEnsemble());

    auto verifyCounts = [this](int destPort, bool bumpOnHit) {
      // gather stats for all ECMP members
      int pktsBefore[kEcmpWidth];
      int pktsBeforeTotal = 0;
      for (int i = 0; i < kEcmpWidth; i++) {
        auto ecmpEgressPort = helper_->ecmpPortDescriptorAt(i).phyPortID();
        pktsBefore[i] =
            *getNextUpdatedPortStats(ecmpEgressPort).outUnicastPkts__ref();
        pktsBeforeTotal += pktsBefore[i];
      }
      auto aclPktCountBefore = utility::getAclInOutPackets(
          getSw(), getCounterName(AclType::FLOWLET));
      int packetCount = 1000;

      std::vector<uint8_t> rethHdr(16);
      rethHdr[15] = 0xFF; // non-zero sized packet
      auto egressPort = helper_->ecmpPortDescriptorAt(4).phyPortID();
      sendRoceTraffic(
          egressPort,
          utility::kUdfRoceOpcodeWriteImmediate,
          rethHdr,
          packetCount,
          destPort);

      WITH_RETRIES({
        auto aclPktCountAfter = utility::getAclInOutPackets(
            getSw(), getCounterName(AclType::FLOWLET));

        int pktsAfter[kEcmpWidth];
        int pktsAfterTotal = 0;
        for (int i = 0; i < kEcmpWidth; i++) {
          auto ecmpEgressPort = helper_->ecmpPortDescriptorAt(i).phyPortID();
          pktsAfter[i] =
              *getNextUpdatedPortStats(ecmpEgressPort).outUnicastPkts__ref();
          pktsAfterTotal += pktsAfter[i];
          XLOG(DBG2) << "Ecmp egress Port: " << ecmpEgressPort
                     << ", Count: " << pktsBefore[i] << " -> " << pktsAfter[i];
        }

        XLOG(DBG2) << "\n"
                   << "aclPacketCounter(" << getCounterName(AclType::FLOWLET)
                   << "): " << aclPktCountBefore << " -> " << (aclPktCountAfter)
                   << "\n";

        // Irrespective of which LB mechanism is used, all packets should egress
        EXPECT_EVENTUALLY_GE(pktsAfterTotal, pktsBeforeTotal + packetCount);

        // Verify ACL count also for hit and miss
        if (bumpOnHit) {
          EXPECT_EVENTUALLY_GE(
              aclPktCountAfter, aclPktCountBefore + packetCount);
        } else {
          EXPECT_EVENTUALLY_EQ(aclPktCountAfter, aclPktCountBefore);
          // also verify traffic is not load-balanced, implying,
          // 3 out of the 4 egress ports should have 0 count
          int zeroCount = 0;
          for (int i = 0; i < kEcmpWidth; i++) {
            if (pktsAfter[i] - pktsBefore[i] == 0) {
              zeroCount++;
            }
          }
          EXPECT_EVENTUALLY_EQ(kEcmpWidth - 1, zeroCount);
        }
      });
    };

    // Verify DLB is hit with ACL matching packet
    verifyCounts(4791, true);
    // Verify packet is still ECMP'd without DLB using static hash
    verifyCounts(1024, false);
  };

  verifyAcrossWarmBoots(setup, verify);
}

// UDF A to empty
TEST_F(AgentFlowletSwitchingTest, VerifyUdfFlowletToFlowlet) {
  flowletSwitchingAclHitHelper(AclType::UDF_FLOWLET, AclType::FLOWLET);
}

TEST_F(AgentFlowletSwitchingTest, VerifyUdfFlowletToUdfWrImmZero) {
  flowletSwitchingAclHitHelper(AclType::UDF_FLOWLET, AclType::UDF_WR_IMM_ZERO);
}

TEST_F(AgentFlowletSwitchingTest, VerifyUdfWrImmZeroToUdfFlowlet) {
  flowletSwitchingAclHitHelper(AclType::UDF_WR_IMM_ZERO, AclType::UDF_FLOWLET);
}

TEST_F(AgentFlowletSwitchingTest, VerifyOneUdfGroupAddition) {
  verifyUdfAddDelete(AclType::UDF_FLOWLET, AclType::UDF_FLOWLET_WITH_UDF_ACK);
}

TEST_F(AgentFlowletSwitchingTest, VerifyOneUdfGroupDeletion) {
  verifyUdfAddDelete(AclType::UDF_FLOWLET_WITH_UDF_ACK, AclType::UDF_FLOWLET);
}

TEST_F(AgentFlowletSwitchingTest, VerifyUdfNakToUdfAckWithNak) {
  flowletSwitchingAclHitHelper(AclType::UDF_NAK, AclType::UDF_ACK_WITH_NAK);
}

TEST_F(AgentFlowletSwitchingTest, VerifyUdfAckWithNakToUdfNak) {
  flowletSwitchingAclHitHelper(AclType::UDF_ACK_WITH_NAK, AclType::UDF_NAK);
}

TEST_F(AgentFlowletSwitchingTest, VerifyUdfAndSendQueueAction) {
  auto setup = [this]() {
    this->setup();
    generateApplyConfig(AclType::UDF_ACK);
  };

  auto verify = [this]() {
    auto outPort = helper_->ecmpPortDescriptorAt(0).phyPortID();
    auto portStatsBefore = getNextUpdatedPortStats(outPort);
    auto pktsBefore = *portStatsBefore.outUnicastPkts__ref();
    auto pktsQueueBefore = portStatsBefore.queueOutPackets_()[kOutQueue];

    verifyAcl(AclType::UDF_ACK);

    WITH_RETRIES({
      auto portStatsAfter = getNextUpdatedPortStats(outPort);
      auto pktsAfter = *portStatsAfter.outUnicastPkts__ref();
      auto pktsQueueAfter = portStatsAfter.queueOutPackets_()[kOutQueue];
      XLOG(DBG2) << "Port Counter: " << pktsBefore << " -> " << pktsAfter
                 << "\nPort Queue " << kOutQueue
                 << " Counter: " << pktsQueueBefore << " -> " << pktsQueueAfter;
      EXPECT_EVENTUALLY_GT(pktsAfter, pktsBefore);
      EXPECT_EVENTUALLY_GT(pktsQueueAfter, pktsQueueBefore);
    });
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentFlowletMirrorTest, VerifyUdfNakMirrorAction) {
  auto setup = [this]() {
    this->setup();
    resolveMirror(aclMirror, utility::kMirrorToPortIndex);
  };

  auto verify = [this]() { verifyMirror(MirrorScope::MIRROR_ONLY); };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentFlowletMirrorTest, VerifyUdfNakMirrorSflowSameVip) {
  auto setup = [this]() {
    this->setup();
    auto newCfg{initialConfig(*getAgentEnsemble())};

    // mirror session for ingress port sflow
    // use same VIP as ACL mirror, only dst port varies
    utility::configureSflowMirror(
        newCfg, kSflowMirrorName, false /* truncate */, aclDestinationVIP);
    // configure sampling on traffic port
    addSamplingConfig(newCfg);

    applyNewConfig(newCfg);
    resolveMirror(aclMirror, utility::kMirrorToPortIndex);
  };

  auto verify = [this]() { verifyMirror(MirrorScope::MIRROR_SFLOW_SAME_VIP); };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentFlowletMirrorTest, VerifyUdfNakMirrorSflowDifferentVip) {
  auto setup = [this]() {
    this->setup();
    auto newCfg{initialConfig(*getAgentEnsemble())};

    // mirror session for ingress port sflow
    utility::configureSflowMirror(
        newCfg, kSflowMirrorName, false /* truncate */, sflowDestinationVIP);
    // configure sampling on traffic port
    addSamplingConfig(newCfg);

    applyNewConfig(newCfg);
    resolveMirror(aclMirror, utility::kMirrorToPortIndex);
    resolveMirror(kSflowMirrorName, utility::kSflowToPortIndex);
  };

  auto verify = [this]() {
    verifyMirror(MirrorScope::MIRROR_SFLOW_DIFFERENT_VIP);
  };

  verifyAcrossWarmBoots(setup, verify);
}

// Skip this and next test due to lack of TCAM in ACL table on TH3
TEST_F(AgentFlowletAclPriorityTest, VerifyUdfAclPriority) {
  auto setup = [this]() {
    this->setup();
    const auto& ensemble = *getAgentEnsemble();
    auto newCfg{initialConfig(ensemble)};
    // production priorities
    addAclAndStat(&newCfg, AclType::UDF_NAK, ensemble.isSai());
    addAclAndStat(&newCfg, AclType::UDF_ACK, ensemble.isSai());
    addAclAndStat(&newCfg, AclType::UDF_WR_IMM_ZERO, ensemble.isSai());
    addAclAndStat(&newCfg, AclType::UDF_FLOWLET, ensemble.isSai());
    // Keep this at the end since each of the above calls update udfConfig
    // differently
    newCfg.udfConfig() = utility::addUdfAclConfig(
        utility::kUdfOffsetBthOpcode | utility::kUdfOffsetBthReserved |
        utility::kUdfOffsetAethSyndrome | utility::kUdfOffsetRethDmaLength);
    applyNewConfig(newCfg);
  };

  auto verify = [this]() {
    verifyAcl(AclType::UDF_NAK);
    verifyAcl(AclType::UDF_ACK);
    verifyAcl(AclType::UDF_WR_IMM_ZERO);
    verifyAcl(AclType::UDF_FLOWLET);
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentFlowletAclPriorityTest, VerifyUdfAclPriorityWB) {
  auto setup = [this]() {
    this->setup();
    auto newCfg{initialConfig(*getAgentEnsemble())};
    applyNewConfig(newCfg);
  };

  auto setupPostWarmboot = [this]() {
    this->setup();
    const auto& ensemble = *getAgentEnsemble();
    auto newCfg{initialConfig(ensemble)};
    // production priorities
    addAclAndStat(&newCfg, AclType::UDF_NAK, ensemble.isSai());
    addAclAndStat(&newCfg, AclType::UDF_ACK, ensemble.isSai());
    addAclAndStat(&newCfg, AclType::UDF_WR_IMM_ZERO, ensemble.isSai());
    addAclAndStat(&newCfg, AclType::UDF_FLOWLET, ensemble.isSai());
    // Keep this at the end since each of the above calls update udfConfig
    // differently
    newCfg.udfConfig() = utility::addUdfAclConfig(
        utility::kUdfOffsetBthOpcode | utility::kUdfOffsetBthReserved |
        utility::kUdfOffsetAethSyndrome | utility::kUdfOffsetRethDmaLength);
    applyNewConfig(newCfg);
  };

  auto verifyPostWarmboot = [this]() {
    verifyAcl(AclType::UDF_NAK);
    verifyAcl(AclType::UDF_ACK);
    verifyAcl(AclType::UDF_WR_IMM_ZERO);
    verifyAcl(AclType::UDF_FLOWLET);
  };

  verifyAcrossWarmBoots(setup, []() {}, setupPostWarmboot, verifyPostWarmboot);
}

TEST_F(AgentFlowletSwitchingTest, CreateMaxDlbGroups) {
  auto verify = [this] {
    generatePrefixes();
    const auto kMaxDlbEcmpGroup =
        utility::getMaxDlbEcmpGroups(getAgentEnsemble()->getL3Asics());
    // install 60% of max DLB ecmp groups
    {
      int count = static_cast<int>(0.6 * kMaxDlbEcmpGroup);
      auto wrapper = getSw()->getRouteUpdater();
      std::vector<RoutePrefixV6> prefixes60 = {
          prefixes.begin(), prefixes.begin() + count};
      std::vector<flat_set<PortDescriptor>> nhopSets60 = {
          nhopSets.begin(), nhopSets.begin() + count};
      helper_->programRoutes(&wrapper, nhopSets60, prefixes60);
    }
    // install 128 groups, failed update
    {
      auto wrapper = getSw()->getRouteUpdater();
      std::vector<RoutePrefixV6> prefixes128 = {
          prefixes.begin(), prefixes.begin() + kMaxDlbEcmpGroup};
      std::vector<flat_set<PortDescriptor>> nhopSets128 = {
          nhopSets.begin(), nhopSets.begin() + kMaxDlbEcmpGroup};
      EXPECT_THROW(
          helper_->programRoutes(&wrapper, nhopSets128, prefixes128),
          FbossError);

      // overflow the dlb groups and ensure that the dlb resource stat
      // is updated. Also once routes are removed, the stat should reset.
      // TODO - Add support for SAI
      if (!getAgentEnsemble()->isSai()) {
        FLAGS_dlbResourceCheckEnable = false;
        std::vector<RoutePrefixV6> prefixes129 = {
            prefixes.begin(), prefixes.begin() + kMaxDlbEcmpGroup + 1};
        std::vector<flat_set<PortDescriptor>> nhopSets129 = {
            nhopSets.begin(), nhopSets.begin() + kMaxDlbEcmpGroup + 1};
        EXPECT_NO_THROW(
            helper_->programRoutes(&wrapper, nhopSets129, prefixes129));
        auto switchId = getSw()
                            ->getScopeResolver()
                            ->scope(masterLogicalPortIds()[0])
                            .switchId();
        WITH_RETRIES({
          auto stats = getHwSwitchStats(switchId);
          EXPECT_EVENTUALLY_TRUE(*stats.arsExhausted());
        });
        helper_->unprogramRoutes(&wrapper, prefixes129);
        WITH_RETRIES({
          auto stats =
              getAgentEnsemble()->getSw()->getHwSwitchStatsExpensive(switchId);
          EXPECT_EVENTUALLY_FALSE(*stats.arsExhausted());
        });
        FLAGS_dlbResourceCheckEnable = true;
      }
    }
    // install 10% of max DLB ecmp groups
    {
      int count = static_cast<int>(0.1 * kMaxDlbEcmpGroup);
      auto wrapper = getSw()->getRouteUpdater();
      std::vector<RoutePrefixV6> prefixes10 = {
          prefixes.begin() + kMaxDlbEcmpGroup,
          prefixes.begin() + kMaxDlbEcmpGroup + count};
      std::vector<flat_set<PortDescriptor>> nhopSets10 = {
          nhopSets.begin() + kMaxDlbEcmpGroup,
          nhopSets.begin() + kMaxDlbEcmpGroup + count};
      EXPECT_NO_THROW(helper_->programRoutes(&wrapper, nhopSets10, prefixes10));
      helper_->unprogramRoutes(&wrapper, prefixes10);
    }
  };
  verifyAcrossWarmBoots([]() {}, verify);
}

TEST_F(AgentFlowletSwitchingTest, ApplyDlbResourceCheck) {
  // Start with 60% ECMP groups
  auto setup = [this]() {
    generatePrefixes();
    const auto kMaxDlbEcmpGroup =
        utility::getMaxDlbEcmpGroups(getAgentEnsemble()->getL3Asics());
    int count = static_cast<int>(0.6 * kMaxDlbEcmpGroup);
    auto wrapper = getSw()->getRouteUpdater();
    std::vector<RoutePrefixV6> prefixes60 = {
        prefixes.begin(), prefixes.begin() + count};
    std::vector<flat_set<PortDescriptor>> nhopSets60 = {
        nhopSets.begin(), nhopSets.begin() + count};
    helper_->programRoutes(&wrapper, nhopSets60, prefixes60);
  };
  // Post warmboot, dlb resource check is enforced since >75%
  auto setupPostWarmboot = [this]() {
    generatePrefixes();
    const auto kMaxDlbEcmpGroup =
        utility::getMaxDlbEcmpGroups(getAgentEnsemble()->getL3Asics());
    {
      auto wrapper = getSw()->getRouteUpdater();
      std::vector<RoutePrefixV6> prefixes128 = {
          prefixes.begin(), prefixes.begin() + kMaxDlbEcmpGroup};
      std::vector<flat_set<PortDescriptor>> nhopSets128 = {
          nhopSets.begin(), nhopSets.begin() + kMaxDlbEcmpGroup};
      EXPECT_THROW(
          helper_->programRoutes(&wrapper, nhopSets128, prefixes128),
          FbossError);
    }
    // install 10% of max DLB ecmp groups
    {
      int count = static_cast<int>(0.1 * kMaxDlbEcmpGroup);
      auto wrapper = getSw()->getRouteUpdater();
      std::vector<RoutePrefixV6> prefixes10 = {
          prefixes.begin(), prefixes.begin() + count};
      std::vector<flat_set<PortDescriptor>> nhopSets10 = {
          nhopSets.begin(), nhopSets.begin() + count};
      helper_->programRoutes(&wrapper, nhopSets10, prefixes10);
    }
  };
  verifyAcrossWarmBoots(setup, [] {}, setupPostWarmboot, [] {});
}

TEST_F(AgentFlowletSwitchingTest, VerifyEcmpSwitchingMode) {
  auto setup = [this]() { this->setup(4); };

  auto verify = [this]() {
    auto ecmpResourceMgr = getSw()->getEcmpResourceManager();
    if (ecmpResourceMgr) {
      auto flowletSwitchConfig =
          getSw()->getState()->getFlowletSwitchingConfig();
      EXPECT_NE(flowletSwitchConfig, nullptr);
      auto backupEcmpGroupType =
          getSw()->getEcmpResourceManager()->getBackupEcmpSwitchingMode();
      ASSERT_TRUE(backupEcmpGroupType.has_value());
      EXPECT_EQ(
          *backupEcmpGroupType, flowletSwitchConfig->getBackupSwitchingMode());
    }
    RoutePrefixV6 prefix{folly::IPAddressV6("::"), 0};
    verifyFwdSwitchingMode(prefix, cfg::SwitchingMode::PER_PACKET_QUALITY);
  };

  verifyAcrossWarmBoots(setup, verify);
}

class AgentFlowletBcmTest : public AgentFlowletSwitchingTest {
 protected:
  void setCmdLineFlagOverrides() const override {
    AgentFlowletSwitchingTest::setCmdLineFlagOverrides();
    FLAGS_dlbResourceCheckEnable = false;
    FLAGS_update_route_with_dlb_type = true;
  }
};

TEST_F(AgentFlowletBcmTest, VerifySwitchingModeUpdateSwState) {
  generatePrefixes();
  const auto kMaxDlbEcmpGroup =
      utility::getMaxDlbEcmpGroups(getAgentEnsemble()->getL3Asics());
  // Create two test prefix vectors
  std::vector<RoutePrefixV6> testPrefixes1 = {
      prefixes.begin(), prefixes.begin() + kMaxDlbEcmpGroup};
  std::vector<RoutePrefixV6> testPrefixes2 = {
      prefixes.begin() + kMaxDlbEcmpGroup,
      prefixes.begin() + kMaxDlbEcmpGroup + 128};
  std::vector<flat_set<PortDescriptor>> testNhopSets1 = {
      nhopSets.begin(), nhopSets.begin() + kMaxDlbEcmpGroup};
  std::vector<flat_set<PortDescriptor>> testNhopSets2 = {
      nhopSets.begin() + kMaxDlbEcmpGroup,
      nhopSets.begin() + kMaxDlbEcmpGroup + 128};

  auto verifySwitchingMode =
      [](const std::shared_ptr<SwitchState> state,
         const std::vector<RoutePrefixV6>& testPrefixes,
         const std::optional<cfg::SwitchingMode>& expectedMode) {
        for (const auto& prefix : testPrefixes) {
          auto route = findRoute<folly::IPAddressV6>(
              RouterID(0), {prefix.network(), prefix.mask()}, state);
          const auto& fwd = route->getForwardInfo();
          auto switchingMode = fwd.getOverrideEcmpSwitchingMode();
          EXPECT_EQ(switchingMode, expectedMode);
        }
      };

  auto setup = [=, this]() {
    {
      auto wrapper = getSw()->getRouteUpdater();
      helper_->programRoutes(&wrapper, testNhopSets1, testPrefixes1);
    }
    // verify ecmp switching mode not filled in
    verifySwitchingMode(getProgrammedState(), testPrefixes1, std::nullopt);
    {
      auto wrapper = getSw()->getRouteUpdater();
      helper_->programRoutes(&wrapper, testNhopSets2, testPrefixes2);
    }
    // verify ecmp switching mode not filled in
    verifySwitchingMode(getProgrammedState(), testPrefixes2, std::nullopt);
  };

  auto verifyPostWarmboot = [=, this]() {
    // First verify if thrift state is correctly written prior to warmboot
    auto wbState = getSw()->getWarmBootHelper()->getWarmBootState();
    auto state = SwitchState::fromThrift(*wbState.swSwitchState());
    verifySwitchingMode(state, testPrefixes1, std::nullopt);
    verifySwitchingMode(
        state,
        testPrefixes2,
        std::optional<cfg::SwitchingMode>(
            cfg::SwitchingMode::PER_PACKET_RANDOM));

    // Now verify if warmboot state is updated in sw state
    verifySwitchingMode(getProgrammedState(), testPrefixes1, std::nullopt);
    verifySwitchingMode(
        getProgrammedState(),
        testPrefixes2,
        std::optional<cfg::SwitchingMode>(
            cfg::SwitchingMode::PER_PACKET_RANDOM));
  };
  verifyAcrossWarmBoots(setup, [] {}, [] {}, verifyPostWarmboot);
}

} // namespace facebook::fboss
