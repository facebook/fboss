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

#include <folly/Conv.h>

#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/AsicUtils.h"
#include "fboss/agent/EcmpResourceManager.h"
#include "fboss/agent/FibHelpers.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/TestUtils.h"
#include "fboss/agent/test/agent_hw_tests/AgentTestEcmpConstants.h"
#include "fboss/agent/test/utils/AclTestUtils.h"
#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/agent/test/utils/LoadBalancerTestUtils.h"
#include "fboss/agent/test/utils/MirrorTestUtils.h"
#include "fboss/agent/test/utils/PortTestUtils.h"
#include "fboss/agent/test/utils/ScaleTestUtils.h"
#include "fboss/agent/test/utils/UdfTestUtils.h"
#include "fboss/agent/types.h"
#include "fboss/lib/CommonUtils.h"
#include "fboss/lib/config/PlatformConfigUtils.h"

namespace facebook::fboss {

class AgentFlowletSwitchingTest : public AgentArsBase {
 public:
  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    return {
        ProductionFeature::DLB,
        ProductionFeature::UDF_WR_IMMEDIATE_ACL,
        ProductionFeature::SINGLE_ACL_TABLE};
  }

 protected:
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto cfg = AgentArsBase::initialConfig(ensemble);
    auto backupSwitchingMode = isChenab(ensemble)
        ? cfg::SwitchingMode::FIXED_ASSIGNMENT
        : cfg::SwitchingMode::PER_PACKET_RANDOM;
    utility::addFlowletConfigs(
        cfg,
        ensemble.masterLogicalPortIds(),
        ensemble.isSai(),
        cfg::SwitchingMode::PER_PACKET_QUALITY,
        backupSwitchingMode);
    return cfg;
  }

  void setCmdLineFlagOverrides() const override {
    AgentHwTest::setCmdLineFlagOverrides();
    FLAGS_flowletSwitchingEnable = true;
    FLAGS_force_init_fp = false;
    FLAGS_enable_ecmp_random_spray = true;
  }
};

// Prune keys off the link state the ASIC sees, and BRCM only reports that with
// PHY loopback rather than the default MAC loopback. The adapter rejects PHY
// loopback on the 400G ports, so -- as AgentAdjFrrRouteTest does -- every port
// this fixture hands to a test is one of the 800G ports it put in PHY loopback.
class AgentFlowletSourcePortPruneTest : public AgentFlowletSwitchingTest {
 public:
  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    auto features = AgentFlowletSwitchingTest::getProductionFeaturesVerified();
    features.push_back(ProductionFeature::ARS_SOURCE_PORT_PRUNE);
    return features;
  }

 protected:
  static constexpr int kMaxDlbLoadBalanceDeviationPct = 25;
  // A pruned member hands its whole share to a single survivor rather than
  // spreading it, so that survivor carries roughly twice what the others do.
  // Measured at every width, including widths needing no padding.
  static constexpr int kPrunedDlbLoadBalanceDeviationPct = 125;
  // Injecting from a member skews DLB even with nothing pruned. DLB steers on
  // measured port load and the ingress port is carrying the injected burst
  // under PHY loopback, so it looks busy and is given less: measured 27% to
  // 39% across widths, against under 5% when the ingress sits outside the
  // group. The spread is the mechanism working, not a defect, so the bound is
  // only here to catch a member dropping out entirely -- which the member
  // count assertion covers more directly.
  static constexpr int kMemberIngressDlbDeviationPct = 125;

  static constexpr int kFlowCount = 2048;
  static constexpr int kPacketsPerFlow = 8;
  static constexpr int kPacketCount = kFlowCount * kPacketsPerFlow;
  // Any UDP port other than 4791 misses the flowlet ACL, so the burst is not
  // DLB eligible and falls to the SDK's internally created secondary group.
  // No ECMP_HASH_CANCEL ACL is needed here: the DLB secondary is static hash
  // already, so missing the flowlet rule is enough to reach it. The FRR tests
  // do install one, because their backup group is random spray and the spray
  // has to be cancelled to get a static hash contrast.
  static constexpr int kNonDlbL4DstPort = 1024;

  // Which selection mechanism a burst exercises.
  enum class TrafficType {
    // 4791, one 5 tuple: the ARS object forwards. DLB selects per packet, not
    // per flow -- it spreads on measured port and queue load rather than on
    // header entropy -- so a single tuple still reaches every member. Sending
    // one flow is what makes the spread meaningful: with kFlowCount tuples the
    // members would fill up from hashing alone and prove nothing about DLB.
    Dlb,
    // 1024, one 5 tuple: secondary group with no entropy, so its static hash
    // must settle on exactly one member.
    SecondarySingleFlow,
    // 1024, kFlowCount flows: secondary group with real entropy, so the hash
    // spreads across members.
    SecondaryMultiFlow,
  };

  // Widths that are not a multiple of four map flows onto members unevenly
  // enough that the sampling noise at kFlowCount shows up: measured ~8% at five
  // and six members against ~0.4% at four. The wider bound absorbs that.
  //
  // It is deliberately *not* justified by secondary padding. Padding keeps
  // every member reachable but does not visibly skew the distribution -- with
  // pruning off, five and six member groups measure even. The 2:1 case is
  // pruning, and that uses kPrunedDlbLoadBalanceDeviationPct instead.
  static int hashSpreadBoundPct(int ecmpWidth) {
    return ecmpWidth % 4 == 0 ? kMaxDlbLoadBalanceDeviationPct
                              : kPrunedDlbLoadBalanceDeviationPct;
  }

  std::optional<size_t> maxRequiredInterfacePorts() const override {
    return std::nullopt;
  }

  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto cfg = AgentFlowletSwitchingTest::initialConfig(ensemble);
    for (auto& port : *cfg.ports()) {
      if (*port.speed() == cfg::PortSpeed::EIGHTHUNDREDG) {
        port.loopbackMode() = cfg::PortLoopbackMode::PHY;
      }
    }
    // The ARS config does not program an ECMP hash on its own, so without this
    // every packet resolves to the same member and prune has nothing to
    // redistribute across.
    utility::addLoadBalancerToConfig(
        cfg,
        checkSameAndGetAsicForTesting(ensemble.getL3Asics()),
        utility::LBHash::FULL_HASH);
    cfg.switchSettings()->ecmpGroupSettings() = splitHorizonSettings();
    return cfg;
  }

  // Split horizon is CREATE_ONLY and SaiSwitch rejects an ecmpGroupSettings
  // change after the first config application, so the value can only arrive
  // through initialConfig(). A prune-on variant is therefore its own fixture
  // overriding this, not a setter called mid-test. Empty here: this fixture is
  // the prune-off side.
  virtual EcmpGroupSettingsMap splitHorizonSettings() const {
    return {};
  }

  static EcmpGroupSettingsMap splitHorizonOn(
      const std::vector<cfg::EcmpGroupType>& groupTypes) {
    cfg::EcmpGroupSettings settings;
    settings.enableSplitHorizon() = true;
    EcmpGroupSettingsMap enabled;
    for (auto groupType : groupTypes) {
      enabled.emplace(groupType, settings);
    }
    return enabled;
  }

  // Derived from the programmed state rather than cached from initialConfig(),
  // which a warm boot never calls.
  std::vector<PortID> getTestPorts() const override {
    std::vector<PortID> phyLoopbackPorts;
    auto state = getProgrammedState();
    for (auto portId : masterLogicalInterfacePortIds()) {
      auto port = state->getPorts()->getNodeIf(portId);
      if (port && port->getLoopbackMode() == cfg::PortLoopbackMode::PHY) {
        phyLoopbackPorts.push_back(portId);
      }
    }
    return phyLoopbackPorts;
  }

  // AgentArsBase::setup() indexes getTestPorts() at both [0, ecmpWidth) and
  // kFrontPanelPortForTest without bounds checking.
  void checkEnoughPhyLoopbackPorts(int ecmpWidth) const {
    size_t needed = std::max(ecmpWidth, kFrontPanelPortForTest + 1);
    auto available = getTestPorts().size();
    CHECK_GE(available, needed)
        << "need " << needed << " ports in PHY loopback, found " << available;
  }

  // Sends one burst into the group and checks how the group spread it. Delivery
  // of the whole burst is always asserted; everything else is opt in.
  //
  // ecmpWidth
  //   Members in the group. testPorts[0, ecmpWidth) are the members and stats
  //   are polled over exactly those ports.
  // ingressPortIdx
  //   Index into testPorts of the port the burst is injected on. Inside
  //   [0, ecmpWidth) it is a member, so split horizon can act on it; outside,
  //   it is not a pruning candidate and the phase serves as a control.
  //   When it is a member: the port is in PHY loopback, so its outUnicastPkts
  //   also counts what the test transmitted, and those are subtracted before
  //   judging what the group sent back out of it. It is then tallied apart from
  //   membersWithTraffic. When it is not a member it is outside the polled set
  //   entirely and nothing about it is measured.
  // traffic
  //   Which selection mechanism to exercise. Also decides the L4 destination
  //   port and whether the burst is one 5 tuple or kFlowCount of them.
  // expectPruned
  //   Whether the ingress must egress nothing. Only meaningful when the ingress
  //   is a member; ignored otherwise.
  // expectedMembersWithTraffic
  //   How many members other than the ingress must carry a share. Defaults to
  //   the whole group, less the ingress when the ingress is a member.
  // maxDeviationPct
  //   If set, the lightest and heaviest forwarding member must be within this
  //   percentage of each other. Unset skips the balance check.
  // assertMemberCount
  //   Set false where which members forward cannot be pinned down: a DLB burst
  //   quantises onto flowlet sets, and a single flow can land on the ingress
  //   itself, which the count deliberately excludes.
  void sendFlowsAndVerifyPrune(
      int ecmpWidth,
      int ingressPortIdx,
      TrafficType traffic,
      bool expectPruned,
      std::optional<int> expectedMembersWithTraffic = std::nullopt,
      std::optional<int> maxDeviationPct = std::nullopt,
      bool assertMemberCount = true) {
    const bool ingressIsMember = ingressPortIdx < ecmpWidth;
    const bool dlbExpected = traffic == TrafficType::Dlb;
    const int l4DstPort =
        dlbExpected ? utility::kUdfL4DstPort : kNonDlbL4DstPort;
    // Only SecondaryMultiFlow needs entropy: it is the one burst selected by a
    // static hash. DLB selects per packet on load, and the secondary single
    // flow case is asserting what a hash does with no entropy at all, so both
    // send one 5 tuple.
    const bool singleFlow = traffic != TrafficType::SecondaryMultiFlow;
    const int flowCount = singleFlow ? 1 : kFlowCount;
    const int packetsPerFlow = singleFlow ? kPacketCount : kPacketsPerFlow;
    auto testPorts = getTestPorts();
    std::vector<PortID> ecmpPorts(
        testPorts.begin(), testPorts.begin() + ecmpWidth);
    auto ingressPort = testPorts[ingressPortIdx];
    const auto counterName = getCounterName(AclType::FLOWLET);

    auto statsBefore = getNextUpdatedPortStats(ecmpPorts);
    auto aclPktsBefore = utility::getAclInOutPackets(getSw(), counterName);

    std::vector<uint8_t> rethHdr(16);
    rethHdr[15] = 0xFF; // non-zero sized packet, so DLB is engaged
    // setup() programs ::/0, so every source IP still resolves to this ECMP
    // group while giving the hash something to spread on.
    for (int flow = 0; flow < flowCount; flow++) {
      sendRoceTraffic(
          ingressPort,
          utility::kUdfRoceOpcodeWriteImmediate,
          rethHdr,
          packetsPerFlow,
          l4DstPort,
          0,
          folly::IPAddressV6(folly::to<std::string>("1001::", flow + 1)));
    }

    WITH_RETRIES({
      auto statsAfter = getNextUpdatedPortStats(ecmpPorts);
      int64_t egressed = 0;
      int64_t ingressEgressed = 0;
      int membersWithTraffic = 0;
      int64_t lowestDelta = std::numeric_limits<int64_t>::max();
      int64_t highestDelta = 0;
      for (int i = 0; i < ecmpWidth; i++) {
        auto port = testPorts[i];
        auto delta = *statsAfter.at(port).outUnicastPkts_() -
            *statsBefore.at(port).outUnicastPkts_();
        if (i == ingressPortIdx) {
          delta -= kPacketCount;
          ingressEgressed = delta;
        } else if (delta > 0) {
          membersWithTraffic++;
        }
        if (delta > 0) {
          lowestDelta = std::min(lowestDelta, delta);
          highestDelta = std::max(highestDelta, delta);
        }
        egressed += delta;
        XLOG(DBG2) << "Ecmp egress port " << i << " (" << port << "): delta "
                   << delta;
      }
      // Only members are polled, so on a control phase the ingress is outside
      // ecmpPorts and ingressEgressed is not measured rather than zero.
      XLOG(DBG2) << "Ingress port " << ingressPort << " egressed "
                 << (ingressIsMember ? std::to_string(ingressEgressed)
                                     : std::string("not measured (non member)"))
                 << ", members with traffic " << membersWithTraffic
                 << ", expectPruned " << expectPruned;
      EXPECT_EVENTUALLY_GE(egressed, kPacketCount);

      auto aclPktsAfter = utility::getAclInOutPackets(getSw(), counterName);
      if (dlbExpected) {
        EXPECT_EVENTUALLY_GE(aclPktsAfter, aclPktsBefore + kPacketCount);
      } else {
        EXPECT_EVENTUALLY_EQ(aclPktsAfter, aclPktsBefore);
      }

      if (ingressIsMember) {
        if (expectPruned) {
          EXPECT_EVENTUALLY_EQ(0, ingressEgressed);
        } else if (traffic != TrafficType::SecondarySingleFlow) {
          // Only the static hash with no entropy is unassertable here: it picks
          // one member and that need not be the ingress. DLB spreads per packet
          // even on a single tuple, so an unpruned ingress must carry a share.
          EXPECT_EVENTUALLY_GT(ingressEgressed, 0);
        }
      }
      // Every member prune did not exclude has to carry a share. This is a
      // count, not a balance check -- how evenly they share is
      // maxDeviationPct's job, and padding makes an even split impossible at
      // some widths.
      if (assertMemberCount) {
        EXPECT_EVENTUALLY_EQ(
            expectedMembersWithTraffic.value_or(
                ingressIsMember ? ecmpWidth - 1 : ecmpWidth),
            membersWithTraffic);
      }
      if (maxDeviationPct.has_value() && highestDelta > 0) {
        XLOG(DBG2) << "Load spread lowest " << lowestDelta << ", highest "
                   << highestDelta;
        EXPECT_EVENTUALLY_TRUE(
            utility::isDeviationWithinThreshold(
                lowestDelta, highestDelta, *maxDeviationPct));
      }
    });
  }
};

class AgentFlowletAclPriorityTest : public AgentFlowletSwitchingTest {
 public:
  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    return {
        ProductionFeature::DLB,
        ProductionFeature::UDF_WR_IMMEDIATE_ACL,
        ProductionFeature::SINGLE_ACL_TABLE};
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

 protected:
  static constexpr int kNonRoceL4DstPort = 12345;
  static constexpr int kPacketCount = 1;

  struct AclCounters {
    int64_t roceAck{0};
    int64_t flowlet{0};
    int64_t sprayMiss{0};
    int64_t cancel{0};

    AclCounters operator-(const AclCounters& before) const {
      return AclCounters{
          .roceAck = roceAck - before.roceAck,
          .flowlet = flowlet - before.flowlet,
          .sprayMiss = sprayMiss - before.sprayMiss,
          .cancel = cancel - before.cancel};
    }
  };

  enum class ExpectedHit { RoceAck, Flowlet, RoceSprayMiss, Cancel };

  AclCounters readCounters() {
    auto read = [this](AclType aclType) {
      return static_cast<int64_t>(
          utility::getAclInOutPackets(getSw(), getCounterName(aclType)));
    };
    return AclCounters{
        .roceAck = read(AclType::UDF_ACK),
        .flowlet = read(AclType::UDF_FLOWLET),
        .sprayMiss = read(AclType::ROCE_SPRAY_MISS),
        .cancel = read(AclType::ECMP_HASH_CANCEL)};
  }

  void applyAclConfig(bool withSprayMiss = true) {
    auto newCfg{initialConfig(*getAgentEnsemble())};
    std::vector<std::string> udfGroups =
        getUdfGroupsForAcl(AclType::UDF_FLOWLET_WITH_UDF_ACK);
    addAclTableConfig(newCfg, udfGroups);
    ASSERT_NO_FATAL_FAILURE(addSprayMissAcls(newCfg, withSprayMiss));
    applyNewConfig(newCfg);
  }

  void addSprayMissAcls(cfg::SwitchConfig& cfg, bool withSprayMiss = true) {
    const bool isSai = getAgentEnsemble()->isSai();
    // Entry order sets ACL priority, so this reproduces production's block.
    addAclAndStat(&cfg, AclType::UDF_FLOWLET_WITH_UDF_ACK, isSai);
    if (withSprayMiss) {
      addAclAndStat(&cfg, AclType::ROCE_SPRAY_MISS, isSai);
    }
    addAclAndStat(&cfg, AclType::ECMP_HASH_CANCEL, isSai);

    // Production pairs the flowlet counter with the flowlet action.
    bool patched = false;
    for (auto& matchToAction : *cfg.dataPlaneTrafficPolicy()->matchToAction()) {
      if (*matchToAction.matcher() == getAclName(AclType::UDF_FLOWLET)) {
        matchToAction.action()->flowletAction() = cfg::FlowletAction::FORWARD;
        patched = true;
        break;
      }
    }
    ASSERT_TRUE(patched) << "no matcher found for "
                         << getAclName(AclType::UDF_FLOWLET);
  }

  // Asserts the whole counter vector: the other three staying at zero is how
  // an over-matching entry is caught.
  void
  runStep(int roceOpcode, uint8_t bthReserved, int l4DstPort, ExpectedHit hit) {
    auto before = readCounters();

    sendRoceTraffic(
        helper_->ecmpPortDescriptorAt(kFrontPanelPortForTest).phyPortID(),
        roceOpcode,
        std::optional<std::vector<uint8_t>>(),
        kPacketCount,
        l4DstPort,
        bthReserved);

    AclCounters deltas;
    WITH_RETRIES({
      deltas = readCounters() - before;
      auto expectDelta = [&](ExpectedHit owner, int64_t delta) {
        const int64_t expected = hit == owner ? kPacketCount : 0;
        EXPECT_EVENTUALLY_EQ(expected, delta);
      };
      expectDelta(ExpectedHit::RoceAck, deltas.roceAck);
      expectDelta(ExpectedHit::Flowlet, deltas.flowlet);
      expectDelta(ExpectedHit::RoceSprayMiss, deltas.sprayMiss);
      expectDelta(ExpectedHit::Cancel, deltas.cancel);
    });

    XLOG(DBG2) << fmt::format(
        "traffic={},opcode={},BTH.reserved=0x{:02x},dport={} "
        "udf_roce_ack={:+} flowlet={:+} spray_miss={:+} hash_cancel={:+}",
        l4DstPort == utility::kUdfL4DstPort ? "RoCE" : "non-RoCE",
        roceOpcode,
        bthReserved,
        l4DstPort,
        deltas.roceAck,
        deltas.flowlet,
        deltas.sprayMiss,
        deltas.cancel);
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
  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    return {
        ProductionFeature::DLB,
        ProductionFeature::SFLOWv6_SAMPLING,
        ProductionFeature::UDF_WR_IMMEDIATE_ACL,
        ProductionFeature::INGRESS_MIRRORING,
        ProductionFeature::SINGLE_ACL_TABLE};
  }

 protected:
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto cfg = AgentFlowletSwitchingTest::initialConfig(ensemble);
    std::vector<std::string> udfGroups = getUdfGroupsForAcl(AclType::UDF_NAK);
    addAclTableConfig(cfg, udfGroups);
    cfg.udfConfig() = utility::addUdfAclConfig(
        utility::kUdfOffsetBthOpcode | utility::kUdfOffsetAethSyndrome);

    // mirror session for acl
    utility::configureSflowMirror(
        cfg, kAclMirror, false /* truncate */, aclDestinationVIP, 6344);

    return cfg;
  }

  void verifyMirror(MirrorScope scope) {
    // In addition to counting ACL hit with verifyAcl, verify packet mirrored
    auto mirrorPort = helper_->ecmpPortDescriptorAt(1).phyPortID();
    auto sflowPort = helper_->ecmpPortDescriptorAt(2).phyPortID();
    auto pktsMirrorBefore =
        *getNextUpdatedPortStats(mirrorPort).outUnicastPkts_();
    auto pktsSflowBefore =
        *getNextUpdatedPortStats(sflowPort).outUnicastPkts_();

    verifyAcl(AclType::UDF_NAK);

    WITH_RETRIES({
      auto pktsMirrorAfter =
          *getNextUpdatedPortStats(mirrorPort).outUnicastPkts_();
      auto pktsSflowAfter =
          *getNextUpdatedPortStats(sflowPort).outUnicastPkts_();
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
    FLAGS_flowletStatsEnable = true;
    FLAGS_enable_ecmp_resource_manager = true;
    FLAGS_ars_resource_percentage = 100;
    FLAGS_ecmp_resource_manager_make_before_break_buffer = 0;
  }

  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    return {
        ProductionFeature::DLB,
        ProductionFeature::ECMP_RANDOM_SPRAY,
        ProductionFeature::ACL_COUNTER,
        ProductionFeature::UDF_WR_IMMEDIATE_ACL,
        ProductionFeature::SINGLE_ACL_TABLE};
  }

  std::optional<size_t> maxRequiredInterfacePorts() const override {
    // VerifyEcmpRandomSpray uses ecmpPortDescriptorAt(0..13), needs >=14 ports
    return 15;
  }
};

/* Add route 3001::1 > DLB
 * Add route 4001::1 > Spray
 * Add route 5001::1 > Spray
 *
 * Test 1:
 * Send 3001::1 AR=1 hit ACL 1, do DLB
 * Send 3001::1 AR=0 hit ACL 2, cancel spray, static ECMP
 *
 * Test 2:
 * Send 4001::1 AR=1 hit ACL 1, no DLB, random spray
 * Send 4001::1 AR=0 hit ACL 2, cancel spray, static ECMP
 *
 * Test 3:
 * Send 5001::1 AR=1 hit ACL 1, no DLB, random spray
 * Send 5001::1 AR=0 hit ACL 2, cancel spray, static ECMP
 */
TEST_F(AgentFlowletSprayTest, VerifyEcmpRandomSpray) {
  auto populatePortsAndDescs =
      [this](
          int start,
          int end,
          std::vector<PortID>& portIDs,
          boost::container::flat_set<PortDescriptor>& portDescs) {
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
  boost::container::flat_set<PortDescriptor> dlbPortDescs;
  populatePortsAndDescs(0, 4, dlbPortIDs, dlbPortDescs);

  std::vector<PortID> sprayPortIDs;
  boost::container::flat_set<PortDescriptor> randomSprayPortDescs;
  populatePortsAndDescs(4, 8, sprayPortIDs, randomSprayPortDescs);

  std::vector<PortID> sprayPortIDs2;
  boost::container::flat_set<PortDescriptor> randomSprayPortDescs2;
  populatePortsAndDescs(10, 14, sprayPortIDs2, randomSprayPortDescs2);

  auto setup = [this,
                dlbPortDescs,
                randomSprayPortDescs,
                randomSprayPortDescs2]() {
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
    const auto kMaxDlbEcmpGroup = getMaxArsGroups() - 1;
    auto wrapper = getSw()->getRouteUpdater();
    std::vector<RoutePrefixV6> prefixes128 = {
        prefixes.begin(), prefixes.begin() + kMaxDlbEcmpGroup};
    std::vector<boost::container::flat_set<PortDescriptor>> nhopSets128 = {
        nhopSets.begin(), nhopSets.begin() + kMaxDlbEcmpGroup};

    // generate route hitting DLB
    auto dlbPrefix = RoutePrefixV6{folly::IPAddressV6("3001::1"), 128};
    prefixes128.push_back(dlbPrefix);
    nhopSets128.push_back(dlbPortDescs);

    // generate route hitting random spray
    auto randomSprayPrefix = RoutePrefixV6{folly::IPAddressV6("4001::1"), 128};
    prefixes128.push_back(randomSprayPrefix);
    nhopSets128.push_back(randomSprayPortDescs);

    // generate route hitting random spray
    auto randomSprayPrefix2 = RoutePrefixV6{folly::IPAddressV6("5001::1"), 128};
    prefixes128.push_back(randomSprayPrefix2);
    nhopSets128.push_back(randomSprayPortDescs2);

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
  auto verify = [this, dlbPortIDs, sprayPortIDs, sprayPortIDs2]() {
    auto sendTrafficAndVerifyLB = [this](
                                      const folly::IPAddress& dstIp,
                                      int reserved,
                                      const std::vector<PortID>& ports,
                                      bool loadBalanceExpected,
                                      bool is_dlb = false) {
      auto switchId = getSw()
                          ->getScopeResolver()
                          ->scope(masterLogicalPortIds()[0])
                          .switchId();
      HwFlowletStats flowletStats;
      checkWithRetry(
          [&flowletStats, switchId, sw = getSw()]() {
            auto switchStats = sw->getHwSwitchStatsExpensive();
            if (switchStats.find(switchId) == switchStats.end()) {
              return false;
            }
            flowletStats = *switchStats.at(switchId).flowletStats();
            return true;
          },
          120,
          std::chrono::milliseconds(1000),
          " fetch port stats");

      auto reassignmentCounterBefore =
          flowletStats.l3EcmpDlbPortReassignmentCount().value();

      auto dlbAclCountBefore = utility::getAclInOutPackets(
          getSw(), getCounterName(AclType::UDF_FLOWLET));
      auto cancelAclCountBefore = utility::getAclInOutPackets(
          getSw(), getCounterName(AclType::ECMP_HASH_CANCEL));

      auto egressPort =
          helper_->ecmpPortDescriptorAt(kFrontPanelPortForTest).phyPortID();
      int packetCount = 200000;
      auto vlanId = getVlanIDForTx();
      auto intfMac =
          getMacForFirstInterfaceWithPortsForTesting(getProgrammedState());
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

        if (!getAgentEnsemble()->isSai()) {
          auto reassignmentCounterAfter =
              getSw()
                  ->getHwSwitchStatsExpensive(switchId)
                  .flowletStats()
                  ->l3EcmpDlbPortReassignmentCount()
                  .value();
          XLOG(DBG2) << "reassignmentCounter: " << reassignmentCounterBefore
                     << " -> " << reassignmentCounterAfter;
          if (is_dlb) {
            EXPECT_EVENTUALLY_GT(
                reassignmentCounterAfter, reassignmentCounterBefore);
          } else {
            EXPECT_EVENTUALLY_EQ(
                reassignmentCounterAfter, reassignmentCounterBefore);
          }
        }

        auto portStats = getNextUpdatedPortStats(ports);
        for (const auto& [portId, stats] : portStats) {
          XLOG(DBG2) << "Ecmp egress Port: " << portId
                     << ", Count: " << *stats.outUnicastPkts_();
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
        folly::IPAddress("3001::1"),
        utility::kRoceReserved,
        dlbPortIDs,
        true,
        true);
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

    // Test 3
    // Hit DLB ACL but do random spray. Verify another group just to be sure
    sendTrafficAndVerifyLB(
        folly::IPAddress("5001::1"),
        utility::kRoceReserved,
        sprayPortIDs2,
        true);
    // Miss DLB ACL and do static hash
    sendTrafficAndVerifyLB(
        folly::IPAddress("5001::1"), 0, sprayPortIDs2, false);
  };

  verifyAcrossWarmBoots(setup, verify);
};

TEST_F(AgentFlowletSwitchingTest, VerifyEcmp) {
  auto setup = [this]() {
    this->setup(kWideEcmpWidth);
    generateApplyConfig(AclType::FLOWLET);
  };

  auto verify = [this]() {
    auto verifyCounts = [this](int destPort, bool bumpOnHit) {
      // gather stats for all ECMP members
      int pktsBefore[kWideEcmpWidth];
      int pktsBeforeTotal = 0;
      for (int i = 0; i < kWideEcmpWidth; i++) {
        auto ecmpEgressPort = helper_->ecmpPortDescriptorAt(i).phyPortID();
        pktsBefore[i] =
            *getNextUpdatedPortStats(ecmpEgressPort).outUnicastPkts_();
        pktsBeforeTotal += pktsBefore[i];
      }
      // Use appropriate ACL counter based on ARS configuration
      auto aclPktCountBefore = utility::getAclInOutPackets(
          getSw(), getCounterName(AclType::FLOWLET));
      int packetCount = 1000;

      std::vector<uint8_t> rethHdr(16);
      rethHdr[15] = 0xFF; // non-zero sized packet
      auto egressPort =
          helper_->ecmpPortDescriptorAt(kFrontPanelPortForTest).phyPortID();
      sendRoceTraffic(
          egressPort,
          utility::kUdfRoceOpcodeWriteImmediate,
          rethHdr,
          packetCount,
          destPort);

      WITH_RETRIES({
        auto aclPktCountAfter = utility::getAclInOutPackets(
            getSw(), getCounterName(AclType::FLOWLET));

        int pktsAfter[kWideEcmpWidth];
        int pktsAfterTotal = 0;
        for (int i = 0; i < kWideEcmpWidth; i++) {
          auto ecmpEgressPort = helper_->ecmpPortDescriptorAt(i).phyPortID();
          pktsAfter[i] =
              *getNextUpdatedPortStats(ecmpEgressPort).outUnicastPkts_();
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
          for (int i = 0; i < kWideEcmpWidth; i++) {
            if (pktsAfter[i] - pktsBefore[i] == 0) {
              zeroCount++;
            }
          }
          EXPECT_EVENTUALLY_EQ(kWideEcmpWidth - 1, zeroCount);
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

class AgentFlowletSwitchingEnhancedScaleTest
    : public AgentFlowletSwitchingTest {
 public:
  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    return {
        ProductionFeature::DLB,
        ProductionFeature::ALTERNATE_ARS_MEMBERS,
        ProductionFeature::UDF_WR_IMMEDIATE_ACL,
        ProductionFeature::SINGLE_ACL_TABLE};
  }
  void setCmdLineFlagOverrides() const override {
    AgentFlowletSwitchingTest::setCmdLineFlagOverrides();
    FLAGS_enable_th5_ars_scale_mode = true;
    FLAGS_dlbResourceCheckEnable = false;
  }

  std::optional<size_t> maxRequiredInterfacePorts() const override {
    // Enhanced scale test creates large ECMP objects needing many ports
    return std::nullopt;
  }
};

TEST_F(AgentFlowletSwitchingEnhancedScaleTest, VerifyAlternateArsEcmpObjects) {
  auto setup = [this]() {
    this->setup(32);
    generateApplyConfig(AclType::FLOWLET);

    generatePrefixes();

    auto wrapper = getSw()->getRouteUpdater();

    const int maxGroups = 254;
    std::vector<RoutePrefixV6> testPrefixes(
        prefixes.begin(),
        prefixes.begin() + std::min(maxGroups, (int)prefixes.size()));
    std::vector<boost::container::flat_set<PortDescriptor>> testNhopSets(
        nhopSets.begin(),
        nhopSets.begin() + std::min(maxGroups, (int)nhopSets.size()));

    helper_->programRoutes(&wrapper, testNhopSets, testPrefixes);

    // Set default route to use ports [0,1,2,3] combination
    boost::container::flat_set<PortDescriptor> trafficNhopSet;
    std::vector<PortDescriptor> tempPortDescs = {
        helper_->ecmpPortDescriptorAt(0),
        helper_->ecmpPortDescriptorAt(1),
        helper_->ecmpPortDescriptorAt(2),
        helper_->ecmpPortDescriptorAt(3)};
    trafficNhopSet.insert(
        std::make_move_iterator(tempPortDescs.begin()),
        std::make_move_iterator(tempPortDescs.end()));

    auto defaultPrefix = RoutePrefixV6{folly::IPAddressV6("::"), 0};
    std::vector<RoutePrefixV6> defaultPrefixes = {defaultPrefix};
    std::vector<boost::container::flat_set<PortDescriptor>> defaultNhopSets = {
        trafficNhopSet};
    helper_->programRoutes(&wrapper, defaultNhopSets, defaultPrefixes);
  };

  auto verify = [this]() {
    auto verifyCounts =
        [this](
            int destPort, bool bumpOnHit, int portCount, bool useAlternateArs) {
          std::vector<PortID> ecmpPorts;
          std::vector<uint64_t> pktsBefore, pktsAfter;

          // Collect port IDs and before stats
          for (int i = 0; i < portCount; ++i) {
            ecmpPorts.push_back(helper_->ecmpPortDescriptorAt(i).phyPortID());
            pktsBefore.push_back(
                *getNextUpdatedPortStats(ecmpPorts[i]).outUnicastPkts_());
          }

          uint64_t pktsBeforeTotal =
              std::accumulate(pktsBefore.begin(), pktsBefore.end(), 0ULL);
          // Use appropriate ACL counter based on ARS configuration
          auto aclPktCountBefore = utility::getAclInOutPackets(
              getSw(), getCounterName(AclType::FLOWLET, useAlternateArs));

          std::vector<uint8_t> rethHdr(16);
          rethHdr[15] = 0xFF;
          auto egressPort =
              helper_->ecmpPortDescriptorAt(kFrontPanelPortForTest).phyPortID();
          sendRoceTraffic(
              egressPort,
              utility::kUdfRoceOpcodeWriteImmediate,
              rethHdr,
              1000,
              destPort);

          WITH_RETRIES({
            auto aclPktCountAfter = utility::getAclInOutPackets(
                getSw(), getCounterName(AclType::FLOWLET, useAlternateArs));

            // Collect after stats
            pktsAfter.clear();
            for (int i = 0; i < portCount; ++i) {
              pktsAfter.push_back(
                  *getNextUpdatedPortStats(ecmpPorts[i]).outUnicastPkts_());
              XLOG(DBG2) << "Ecmp egress Port " << i << ": " << ecmpPorts[i]
                         << ", Count: " << pktsBefore[i] << " -> "
                         << pktsAfter[i];
            }

            uint64_t pktsAfterTotal =
                std::accumulate(pktsAfter.begin(), pktsAfter.end(), 0ULL);
            XLOG(DBG2) << "Total packets: " << pktsBeforeTotal << " -> "
                       << pktsAfterTotal;
            XLOG(DBG2) << "ACL count: " << aclPktCountBefore << " -> "
                       << aclPktCountAfter;

            // Check total packets across all ports in the ECMP group
            EXPECT_EVENTUALLY_GE(pktsAfterTotal, pktsBeforeTotal + 1000);

            // Count ports with traffic using vector comparison
            int portsWithTraffic = 0;
            for (int i = 0; i < portCount; ++i) {
              if (pktsAfter[i] > pktsBefore[i]) {
                portsWithTraffic++;
              }
            }

            if (bumpOnHit) {
              EXPECT_EVENTUALLY_GE(aclPktCountAfter, aclPktCountBefore + 1000);
              // Verify that traffic is distributed across all ports when DLB is
              // enabled
              EXPECT_EVENTUALLY_EQ(portsWithTraffic, portCount);
            } else {
              EXPECT_EVENTUALLY_EQ(aclPktCountAfter, aclPktCountBefore);
              // When DLB is disabled, traffic should use traditional ECMP
              // hashing and go to exactly one port for a consistent flow
              EXPECT_EVENTUALLY_EQ(portsWithTraffic, 1);
            }
          });
        };

    // Initial verification with 4 ports (primary ARS)
    verifyCounts(4791, true, 4, true); // DLB enabled, alternate ARS
    verifyCounts(1024, false, 4, true); // DLB disabled, alternate ARS

    // Modify default route to point to primary ars
    {
      auto wrapper = getSw()->getRouteUpdater();

      // Create nexthop set with first 3 ports [0,1,2]
      boost::container::flat_set<PortDescriptor> threePortNhopSet;
      std::vector<PortDescriptor> tempPortDescs = {
          helper_->ecmpPortDescriptorAt(0),
          helper_->ecmpPortDescriptorAt(1),
          helper_->ecmpPortDescriptorAt(2)};
      threePortNhopSet.insert(
          std::make_move_iterator(tempPortDescs.begin()),
          std::make_move_iterator(tempPortDescs.end()));

      auto defaultPrefix = RoutePrefixV6{folly::IPAddressV6("::"), 0};
      std::vector<RoutePrefixV6> defaultPrefixes = {defaultPrefix};
      std::vector<boost::container::flat_set<PortDescriptor>>
          threePortNhopSets = {threePortNhopSet};
      helper_->programRoutes(&wrapper, threePortNhopSets, defaultPrefixes);
    }

    // Verify traffic on the 3 ports (should trigger primary ARS)
    verifyCounts(4791, true, 3, false); // DLB enabled, 3 ports, primary ARS
    verifyCounts(1024, false, 3, false); // DLB disabled, 3 ports, primary ARS

    // Move default route back to alternate ars
    {
      auto wrapper = getSw()->getRouteUpdater();

      // Create nexthop set with first 4 ports [0,1,2,3]
      boost::container::flat_set<PortDescriptor> fourPortNhopSet;
      std::vector<PortDescriptor> tempPortDescs = {
          helper_->ecmpPortDescriptorAt(0),
          helper_->ecmpPortDescriptorAt(1),
          helper_->ecmpPortDescriptorAt(2),
          helper_->ecmpPortDescriptorAt(3)};
      fourPortNhopSet.insert(
          std::make_move_iterator(tempPortDescs.begin()),
          std::make_move_iterator(tempPortDescs.end()));

      auto defaultPrefix = RoutePrefixV6{folly::IPAddressV6("::"), 0};
      std::vector<RoutePrefixV6> defaultPrefixes = {defaultPrefix};
      std::vector<boost::container::flat_set<PortDescriptor>> fourPortNhopSets =
          {fourPortNhopSet};
      helper_->programRoutes(&wrapper, fourPortNhopSets, defaultPrefixes);
    }

    // Verify traffic on the 4 ports again (back to alternate ARS)
    verifyCounts(4791, true, 4, true); // DLB enabled, 4 ports, alternate ARS
    verifyCounts(1024, false, 4, true); // DLB disabled, 4 ports, alternate ARS
  };

  verifyAcrossWarmBoots(setup, verify);
}

class AgentFlowletWideArsSwitchingTest : public AgentFlowletSwitchingTest {
 public:
  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    return {
        ProductionFeature::DLB,
        ProductionFeature::VIRTUAL_ARS_GROUP,
        ProductionFeature::UDF_WR_IMMEDIATE_ACL,
        ProductionFeature::SINGLE_ACL_TABLE};
  }
  void setCmdLineFlagOverrides() const override {
    AgentFlowletSwitchingTest::setCmdLineFlagOverrides();
    FLAGS_dlbResourceCheckEnable = true;
    FLAGS_enable_route_resource_protection = true;
    FLAGS_ecmp_width = kWideEcmpWidth;
    FLAGS_ars_resource_percentage = 100;
  }

 protected:
  static constexpr int kWideEcmpWidth = 256;
  static constexpr int kStatsCheckInterval = 25;
  static constexpr int kMinWidthForArsVirtualGroup = 65;
  static constexpr int kMaxVirtualArsGroups = 255;

  std::vector<PortID> getSubsidiaryPorts(const AgentEnsemble& ensemble) const {
    auto portsByControllingPort = utility::getSubsidiaryPortIDs(
        ensemble.getSw()->getPlatformMapping()->getPlatformPorts());
    const auto& platformPorts =
        ensemble.getSw()->getPlatformMapping()->getPlatformPorts();
    std::vector<PortID> ports;
    const SwitchID currentSwitchId = getCurrentSwitchIdForTesting();
    for (const auto& [controllingPort, subPorts] : portsByControllingPort) {
      if (ports.size() >= kWideEcmpWidth) {
        break;
      }
      auto ctrlIt = platformPorts.find(static_cast<int32_t>(controllingPort));
      if (ctrlIt == platformPorts.end() ||
          *ctrlIt->second.mapping()->portType() !=
              cfg::PortType::INTERFACE_PORT) {
        continue;
      }
      if (ensemble.scopeResolver().scope(PortID(controllingPort)).switchId() !=
          currentSwitchId) {
        continue;
      }
      for (auto subPort : subPorts) {
        auto subIt = platformPorts.find(static_cast<int32_t>(subPort));
        if (subIt != platformPorts.end() &&
            *subIt->second.mapping()->portType() ==
                cfg::PortType::MANAGEMENT_PORT) {
          continue;
        }
        ports.push_back(subPort);
      }
    }
    CHECK_GE(ports.size(), kWideEcmpWidth);
    return ports;
  }

  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto ports = getSubsidiaryPorts(ensemble);
    auto cfg = utility::onePortPerInterfaceConfig(
        ensemble.getSw(), ports, true /*interfaceHasSubnet*/);
    const auto& platformPorts =
        ensemble.getSw()->getPlatformMapping()->getPlatformPorts();

    // Force speed to 200G to create atleast 256 ports for th6
    for (auto& port : *cfg.ports()) {
      if (*port.speed() > cfg::PortSpeed::TWOHUNDREDG) {
        auto platIt =
            platformPorts.find(static_cast<int32_t>(*port.logicalID()));
        if (platIt != platformPorts.end()) {
          for (const auto& [profileID, profileCfg] :
               *platIt->second.supportedProfiles()) {
            if (utility::getSpeed(profileID) == cfg::PortSpeed::TWOHUNDREDG) {
              port.profileID() = profileID;
              port.speed() = cfg::PortSpeed::TWOHUNDREDG;
              break;
            }
          }
        }
      }
    }
    auto backupSwitchingMode = isChenab(ensemble)
        ? cfg::SwitchingMode::FIXED_ASSIGNMENT
        : cfg::SwitchingMode::PER_PACKET_RANDOM;
    utility::addFlowletConfigs(
        cfg,
        ports,
        ensemble.isSai(),
        cfg::SwitchingMode::PER_PACKET_QUALITY,
        backupSwitchingMode);
    cfg.flowletSwitchingConfig()->minWidthForArsVirtualGroup() =
        kMinWidthForArsVirtualGroup;
    cfg.flowletSwitchingConfig()->maxArsVirtualGroups() = kMaxVirtualArsGroups;
    cfg.flowletSwitchingConfig()->maxArsVirtualGroupWidth() = kWideEcmpWidth;
    return cfg;
  }

  std::vector<PortID> getTestPorts() const override {
    return getSubsidiaryPorts(*getAgentEnsemble());
  }

  void verifyTrafficDistribution(int ecmpWidth, int destPort, bool bumpOnHit) {
    std::vector<int> pktsBefore(ecmpWidth);
    int pktsBeforeTotal = 0;
    for (int i = 0; i < ecmpWidth; i += kStatsCheckInterval) {
      auto ecmpEgressPort = helper_->ecmpPortDescriptorAt(i).phyPortID();
      pktsBefore[i] =
          *getNextUpdatedPortStats(ecmpEgressPort).outUnicastPkts_();
      pktsBeforeTotal += pktsBefore[i];
    }

    auto aclPktCountBefore =
        utility::getAclInOutPackets(getSw(), getCounterName(AclType::FLOWLET));
    constexpr int packetCount = 1000;

    std::vector<uint8_t> rethHdr(16);
    rethHdr[15] = 0xFF;
    auto egressPort =
        helper_->ecmpPortDescriptorAt(kFrontPanelPortForTest).phyPortID();
    sendRoceTraffic(
        egressPort,
        utility::kUdfRoceOpcodeWriteImmediate,
        rethHdr,
        packetCount,
        destPort);

    WITH_RETRIES_N(5, {
      auto aclPktCountAfter = utility::getAclInOutPackets(
          getSw(), getCounterName(AclType::FLOWLET));

      std::vector<int> pktsAfter(ecmpWidth);
      int pktsAfterTotal = 0;
      for (int i = 0; i < ecmpWidth; i += kStatsCheckInterval) {
        auto ecmpEgressPort = helper_->ecmpPortDescriptorAt(i).phyPortID();
        pktsAfter[i] =
            *getNextUpdatedPortStats(ecmpEgressPort).outUnicastPkts_();
        pktsAfterTotal += pktsAfter[i];
        XLOG(DBG2) << "Ecmp egress Port: " << ecmpEgressPort
                   << ", Count: " << pktsBefore[i] << " -> " << pktsAfter[i];
      }

      XLOG(DBG2) << "\n"
                 << "aclPacketCounter(" << getCounterName(AclType::FLOWLET)
                 << "): " << aclPktCountBefore << " -> " << aclPktCountAfter
                 << "\n";

      EXPECT_EVENTUALLY_GT(pktsAfterTotal, pktsBeforeTotal);

      if (bumpOnHit) {
        EXPECT_EVENTUALLY_GE(aclPktCountAfter, aclPktCountBefore + packetCount);
      } else {
        EXPECT_EVENTUALLY_EQ(aclPktCountAfter, aclPktCountBefore);
        int zeroCount = 0;
        for (int i = 0; i < ecmpWidth; i += kStatsCheckInterval) {
          if (pktsAfter[i] - pktsBefore[i] == 0) {
            zeroCount++;
          }
        }
        int sampledPorts =
            (ecmpWidth + kStatsCheckInterval - 1) / kStatsCheckInterval;
        EXPECT_EVENTUALLY_GE(zeroCount, sampledPorts - 1);
      }
    });
  }
};

TEST_F(AgentFlowletWideArsSwitchingTest, VerifyEcmp) {
  auto setup = [this]() {
    this->setup(kWideEcmpWidth);
    generateApplyConfig(AclType::FLOWLET);
  };

  auto verify = [this]() {
    // Verify DLB is hit with ACL matching packet
    verifyTrafficDistribution(kWideEcmpWidth, 4791, true);
    // Verify packet is still ECMP'd without DLB using static hash
    verifyTrafficDistribution(kWideEcmpWidth, 1024, false);
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentFlowletWideArsSwitchingTest, EcmpScaleTest) {
  static constexpr int kDefaultRouteEcmpWidth = 256;
  static constexpr int kSecondaryEcmpWidth = 90;
  static constexpr int kNumSecondaryEcmpGroups = 254;

  auto setup = [this]() {
    this->setup(kDefaultRouteEcmpWidth);
    generateApplyConfig(AclType::FLOWLET);
    generatePrefixes();

    auto wrapper = getSw()->getRouteUpdater();

    std::vector<RoutePrefixV6> testPrefixes;
    std::vector<boost::container::flat_set<PortDescriptor>> testNhopSets;
    testPrefixes.reserve(kNumSecondaryEcmpGroups);
    testNhopSets.reserve(kNumSecondaryEcmpGroups);

    for (int i = 0; i < kNumSecondaryEcmpGroups; ++i) {
      testPrefixes.push_back(prefixes[i]);
      // Create unique nhop sets from 256 ports using rotation:
      // Groups 0-253: rotation with step=1, each group starts at port i
      boost::container::flat_set<PortDescriptor> nhopSet;
      int rotation = i % kDefaultRouteEcmpWidth;
      for (int j = 0; j < kSecondaryEcmpWidth; ++j) {
        int portIndex = (rotation + j) % kDefaultRouteEcmpWidth;
        nhopSet.insert(helper_->ecmpPortDescriptorAt(portIndex));
      }
      testNhopSets.push_back(nhopSet);
    }

    helper_->programRoutes(&wrapper, testNhopSets, testPrefixes);

    XLOG(DBG2) << "EcmpScaleTest::setup - programmed "
               << kNumSecondaryEcmpGroups << " secondary ECMP groups with "
               << kSecondaryEcmpWidth << " members each, "
               << "default route has " << kDefaultRouteEcmpWidth << " members";
  };

  auto verify = [this]() {
    // Verify DLB is hit with ACL matching packet
    verifyTrafficDistribution(kDefaultRouteEcmpWidth, 4791, true);
    // Verify packet is still ECMP'd without DLB using static hash
    verifyTrafficDistribution(kDefaultRouteEcmpWidth, 1024, false);
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
    auto pktsBefore = *portStatsBefore.outUnicastPkts_();
    auto pktsQueueBefore = portStatsBefore.queueOutPackets_()[kOutQueue];

    verifyAcl(AclType::UDF_ACK);

    WITH_RETRIES({
      auto portStatsAfter = getNextUpdatedPortStats(outPort);
      auto pktsAfter = *portStatsAfter.outUnicastPkts_();
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
    const auto& ensemble = *getAgentEnsemble();
    auto newCfg{initialConfig(ensemble)};
    addAclAndStat(
        &newCfg, AclType::UDF_NAK, ensemble.isSai(), true /* addMirror */);
    applyNewConfig(newCfg);
    resolveMirror(kAclMirror, utility::kMirrorToPortIndex);
  };

  auto verify = [this]() { verifyMirror(MirrorScope::MIRROR_ONLY); };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentFlowletMirrorTest, VerifyUdfNakMirrorSflowSameVip) {
  auto setup = [this]() {
    this->setup();
    const auto& ensemble = *getAgentEnsemble();
    auto newCfg{initialConfig(ensemble)};
    addAclAndStat(
        &newCfg, AclType::UDF_NAK, ensemble.isSai(), true /* addMirror */);

    // mirror session for ingress port sflow
    // use same VIP as ACL mirror, only dst port varies
    utility::configureSflowMirror(
        newCfg, kSflowMirrorName, false /* truncate */, aclDestinationVIP);
    // configure sampling on traffic port
    addSamplingConfig(newCfg);

    applyNewConfig(newCfg);
    resolveMirror(kAclMirror, utility::kMirrorToPortIndex);
  };

  auto verify = [this]() { verifyMirror(MirrorScope::MIRROR_SFLOW_SAME_VIP); };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentFlowletMirrorTest, VerifyUdfNakMirrorSflowDifferentVip) {
  auto setup = [this]() {
    this->setup();
    const auto& ensemble = *getAgentEnsemble();
    auto newCfg{initialConfig(ensemble)};
    addAclAndStat(
        &newCfg, AclType::UDF_NAK, ensemble.isSai(), true /* addMirror */);

    // mirror session for ingress port sflow
    utility::configureSflowMirror(
        newCfg, kSflowMirrorName, false /* truncate */, sflowDestinationVIP);
    // configure sampling on traffic port
    addSamplingConfig(newCfg);

    applyNewConfig(newCfg);
    resolveMirror(kAclMirror, utility::kMirrorToPortIndex);
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
    const auto kMaxDlbEcmpGroup = getMaxArsGroups();
    // install 60% of max DLB ecmp groups
    {
      int count = static_cast<int>(0.6 * kMaxDlbEcmpGroup);
      auto wrapper = getSw()->getRouteUpdater();
      std::vector<RoutePrefixV6> prefixes60 = {
          prefixes.begin(), prefixes.begin() + count};
      std::vector<boost::container::flat_set<PortDescriptor>> nhopSets60 = {
          nhopSets.begin(), nhopSets.begin() + count};
      helper_->programRoutes(&wrapper, nhopSets60, prefixes60);
    }
    // install 128 groups, failed update
    {
      auto wrapper = getSw()->getRouteUpdater();
      std::vector<RoutePrefixV6> prefixes128 = {
          prefixes.begin(), prefixes.begin() + kMaxDlbEcmpGroup};
      std::vector<boost::container::flat_set<PortDescriptor>> nhopSets128 = {
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
        std::vector<boost::container::flat_set<PortDescriptor>> nhopSets129 = {
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
      std::vector<boost::container::flat_set<PortDescriptor>> nhopSets10 = {
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
    const auto kMaxDlbEcmpGroup = getMaxArsGroups();
    int count = static_cast<int>(0.6 * kMaxDlbEcmpGroup);
    auto wrapper = getSw()->getRouteUpdater();
    std::vector<RoutePrefixV6> prefixes60 = {
        prefixes.begin(), prefixes.begin() + count};
    std::vector<boost::container::flat_set<PortDescriptor>> nhopSets60 = {
        nhopSets.begin(), nhopSets.begin() + count};
    helper_->programRoutes(&wrapper, nhopSets60, prefixes60);
  };
  // Post warmboot, dlb resource check is enforced since >75%
  auto setupPostWarmboot = [this]() {
    generatePrefixes();
    const auto kMaxDlbEcmpGroup = getMaxArsGroups();
    {
      auto wrapper = getSw()->getRouteUpdater();
      std::vector<RoutePrefixV6> prefixes128 = {
          prefixes.begin(), prefixes.begin() + kMaxDlbEcmpGroup};
      std::vector<boost::container::flat_set<PortDescriptor>> nhopSets128 = {
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
      std::vector<boost::container::flat_set<PortDescriptor>> nhopSets10 = {
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
  const auto kMaxDlbEcmpGroup = getMaxArsGroups();
  // Create two test prefix vectors
  std::vector<RoutePrefixV6> testPrefixes1 = {
      prefixes.begin(), prefixes.begin() + kMaxDlbEcmpGroup};
  std::vector<RoutePrefixV6> testPrefixes2 = {
      prefixes.begin() + kMaxDlbEcmpGroup,
      prefixes.begin() + kMaxDlbEcmpGroup + 128};
  std::vector<boost::container::flat_set<PortDescriptor>> testNhopSets1 = {
      nhopSets.begin(), nhopSets.begin() + kMaxDlbEcmpGroup};
  std::vector<boost::container::flat_set<PortDescriptor>> testNhopSets2 = {
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

/*
 * roce-spray-miss: an unqualified RoCEv2 entry below the DLB enable ACL,
 * splitting non-sprayed RDMA out of the catch-all so it can be counted.
 *
 * It is the non-sprayed bucket only because it is appended after udf-flowlet
 * and first match wins. Swapping the two would disable spray with no config
 * error, so every step asserts the whole counter vector, not just its own.
 */
TEST_F(AgentFlowletAclPriorityTest, VerifyRoceSprayMissAcl) {
  auto setup = [this]() {
    this->setup();
    applyAclConfig();
  };

  auto verify = [this]() {
    runStep(
        utility::kUdfRoceOpcodeWriteImmediate,
        utility::kRoceReserved,
        utility::kUdfL4DstPort,
        ExpectedHit::Flowlet);
    runStep(
        utility::kUdfRoceOpcodeWriteImmediate,
        0,
        utility::kUdfL4DstPort,
        ExpectedHit::RoceSprayMiss);
    runStep(
        utility::kUdfRoceOpcodeWriteImmediate,
        0,
        kNonRoceL4DstPort,
        ExpectedHit::Cancel);
    runStep(
        utility::kUdfRoceOpcodeAck,
        utility::kRoceReserved,
        utility::kUdfL4DstPort,
        ExpectedHit::RoceAck);
  };

  verifyAcrossWarmBoots(setup, verify);
}

// Reaching the four-entry block from the three-entry form production runs
// today. Inserting third renumbers the catch-all, which SAI can only do by
// removing and re-creating it.
TEST_F(AgentFlowletAclPriorityTest, VerifyRoceSprayMissAclInsertion) {
  auto setup = [this]() {
    this->setup();
    applyAclConfig(/*withSprayMiss*/ false);
  };

  auto setupPostWarmboot = [this]() { applyAclConfig(/*withSprayMiss*/ true); };

  auto verifyPostWarmboot = [this]() {
    runStep(
        utility::kUdfRoceOpcodeWriteImmediate,
        0,
        utility::kUdfL4DstPort,
        ExpectedHit::RoceSprayMiss);
    runStep(
        utility::kUdfRoceOpcodeWriteImmediate,
        0,
        kNonRoceL4DstPort,
        ExpectedHit::Cancel);
    runStep(
        utility::kUdfRoceOpcodeWriteImmediate,
        utility::kRoceReserved,
        utility::kUdfL4DstPort,
        ExpectedHit::Flowlet);
  };

  verifyAcrossWarmBoots(setup, []() {}, setupPostWarmboot, verifyPostWarmboot);
}

} // namespace facebook::fboss
