// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <fmt/core.h>
#include <folly/IPAddressV6.h>
#include <folly/io/Cursor.h>
#include <limits>

#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/packet/ICMPHdr.h"
#include "fboss/agent/packet/IPProto.h"
#include "fboss/agent/packet/IPv6Hdr.h"
#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/ResourceLibUtil.h"
#include "fboss/agent/test/utils/AccessPolicyAclTestUtils.h"
#include "fboss/agent/test/utils/AclTestUtils.h"
#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/lib/CommonUtils.h"

DECLARE_bool(enable_acl_table_group);

namespace facebook::fboss {

namespace {

constexpr auto kRestricted = cfg::AclLookupClassPort::CLASS_PORT_RESTRICTED;
constexpr auto kUnconstrained =
    cfg::AclLookupClassPort::CLASS_PORT_UNCONSTRAINED;

constexpr int kRestrictedPortIdx = 0;
constexpr int kUnconstrainedPortIdx = 1;
constexpr int kEgressPortIdx = 2;
constexpr size_t kRequiredInterfacePorts = kEgressPortIdx + 1;

// The probe loops straight back in off the egress port; a hop limit of 2 makes
// the second pass expire instead of circulating, which is what lets the egress
// packet count be compared exactly.
constexpr uint8_t kHopLimit = 2;

folly::IPAddressV6 kSrcIp() {
  return folly::IPAddressV6("2001:db8:2::1");
}

folly::IPAddressV6 kDstIp() {
  return folly::IPAddressV6("2001:db8:1::1");
}

} // namespace

class AgentAccessPolicyAclTest : public AgentHwTest {
 protected:
  void setCmdLineFlagOverrides() const override {
    AgentHwTest::setCmdLineFlagOverrides();
    FLAGS_enable_acl_table_group = true;
  }

  std::optional<size_t> maxRequiredInterfacePorts() const override {
    return kRequiredInterfacePorts;
  }

  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto config = utility::onePortPerInterfaceConfig(
        ensemble.getSw(),
        ensemble.masterLogicalInterfacePortIds(),
        true /*interfaceHasSubnet*/);
    if (coldBootWithAccessPolicy()) {
      addAccessPolicy(
          config,
          ensemble.getL3Asics(),
          ensemble.masterLogicalInterfacePortIds(),
          coldBootOmitRules());
    }
    return config;
  }

  virtual bool coldBootWithAccessPolicy() const {
    return true;
  }

  virtual bool warmBootWithAccessPolicy() const {
    return true;
  }

  virtual std::set<std::string> coldBootOmitRules() const {
    return {};
  }

  virtual std::set<std::string> warmBootOmitRules() const {
    return {};
  }

  void runAccessPolicyTest() {
    auto setup = [this]() { programRouteToEgressPort(); };
    auto verify = [this]() {
      verifyAccessPolicy(coldBootWithAccessPolicy(), coldBootOmitRules());
    };
    auto setupPostWarmboot = [this]() {
      if (!warmBootConfigDiffers()) {
        return;
      }
      // Start from the config the warm boot came up on, so the only delta the
      // agent sees is the access policy.
      auto config = getAgentEnsemble()->getCurrentConfig();
      utility::removeAccessPolicy(config, shape());
      if (warmBootWithAccessPolicy()) {
        addAccessPolicy(
            config,
            getL3Asics(),
            masterLogicalInterfacePortIds(),
            warmBootOmitRules());
      }
      applyNewConfig(config);
    };
    auto verifyPostWarmboot = [this]() {
      if (!warmBootConfigDiffers()) {
        return;
      }
      verifyAccessPolicy(warmBootWithAccessPolicy(), warmBootOmitRules());
    };
    verifyAcrossWarmBoots(setup, verify, setupPostWarmboot, verifyPostWarmboot);
  }

 private:
  bool warmBootConfigDiffers() const {
    return coldBootWithAccessPolicy() != warmBootWithAccessPolicy() ||
        coldBootOmitRules() != warmBootOmitRules();
  }

  utility::AccessPolicyShape shape() const {
    auto shape = utility::accessPolicyShape(getL3Asics());
    CHECK(shape.has_value());
    return *shape;
  }

  void addAccessPolicy(
      cfg::SwitchConfig& config,
      const std::vector<const HwAsic*>& asics,
      const std::vector<PortID>& portIds,
      const std::set<std::string>& omitRules) const {
    auto policyShape = utility::accessPolicyShape(asics);
    CHECK(policyShape.has_value());
    utility::addAccessPolicyTables(config, *policyShape);
    utility::addAccessPolicyAcls(config, asics, *policyShape, omitRules);
    utility::bindAccessPolicyPort(
        config, *policyShape, portIds[kRestrictedPortIdx], kRestricted);
    utility::bindAccessPolicyPort(
        config, *policyShape, portIds[kUnconstrainedPortIdx], kUnconstrained);
  }

  void programRouteToEgressPort() {
    utility::EcmpSetupTargetedPorts6 ecmpHelper(
        getProgrammedState(), getSw()->needL2EntryForNeighbor());
    boost::container::flat_set<PortDescriptor> nhops{
        PortDescriptor(masterLogicalInterfacePortIds()[kEgressPortIdx])};
    applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      return ecmpHelper.resolveNextHops(in, nhops);
    });
    auto wrapper = getSw()->getRouteUpdater();
    ecmpHelper.programRoutes(&wrapper, nhops);
  }

  std::unique_ptr<TxPacket> makeProbePacket(
      const utility::AccessPolicyProbe& probe) {
    auto vlanId = getVlanIDForTx();
    auto intfMac = getMacForFirstInterfaceWithPorts(getProgrammedState());
    auto srcMac = utility::MacAddressGenerator().get(intfMac.u64HBO() + 1);
    auto dstIp =
        probe.dstIp.has_value() ? folly::IPAddressV6(*probe.dstIp) : kDstIp();
    CHECK(probe.proto.has_value()) << "probe " << probe.name << " has no proto";
    auto proto = static_cast<IP_PROTO>(*probe.proto);
    if (proto == IP_PROTO::IP_PROTO_UDP) {
      return utility::makeUDPTxPacket(
          getSw(),
          vlanId,
          srcMac,
          intfMac,
          kSrcIp(),
          dstIp,
          *probe.l4SrcPort,
          *probe.l4DstPort,
          0 /*trafficClass*/,
          kHopLimit);
    }
    if (proto == IP_PROTO::IP_PROTO_TCP) {
      return utility::makeTCPTxPacket(
          getSw(),
          vlanId,
          srcMac,
          intfMac,
          kSrcIp(),
          dstIp,
          *probe.l4SrcPort,
          *probe.l4DstPort,
          0 /*trafficClass*/,
          kHopLimit,
          std::nullopt /*payload*/,
          tcpFlags(probe));
    }
    return makeIcmpV6ProbePacket(vlanId, srcMac, intfMac, dstIp);
  }

  static uint8_t tcpFlags(const utility::AccessPolicyProbe& probe) {
    auto flags = probe.tcpFlagsBitMap.value_or(0);
    CHECK_LE(flags, std::numeric_limits<uint8_t>::max())
        << "probe " << probe.name << " does not fit a TCP flags byte";
    return static_cast<uint8_t>(flags);
  }

  std::unique_ptr<TxPacket> makeIcmpV6ProbePacket(
      std::optional<VlanID> vlanId,
      folly::MacAddress srcMac,
      folly::MacAddress dstMac,
      const folly::IPAddressV6& dstIp) {
    std::vector<uint8_t> body(56, 0xff);
    IPv6Hdr ipHdr(kSrcIp(), dstIp);
    ipHdr.nextHeader = static_cast<uint8_t>(IP_PROTO::IP_PROTO_IPV6_ICMP);
    ipHdr.payloadLength = ICMPHdr::SIZE + body.size();
    ipHdr.hopLimit = kHopLimit;

    ICMPHdr icmpHdr(
        static_cast<uint8_t>(ICMPv6Type::ICMPV6_TYPE_ECHO_REQUEST),
        static_cast<uint8_t>(ICMPv6Code::ICMPV6_CODE_ECHO_REQUEST),
        0 /*csum*/);
    auto pkt = getSw()->allocatePacket(icmpHdr.computeTotalLengthV6(
        body.size(), vlanId.has_value() /*taggedPkt*/));
    folly::io::RWPrivateCursor cursor(pkt->buf());
    icmpHdr.serializeFullPacket(
        &cursor,
        dstMac,
        srcMac,
        vlanId,
        ipHdr,
        body.size(),
        [&body](folly::io::RWPrivateCursor* bodyCursor) {
          bodyCursor->push(body.data(), body.size());
        });
    return pkt;
  }

  std::map<std::string, uint64_t> aclCounters(
      const std::set<std::string>& omitRules) {
    std::vector<std::string> counterNames;
    for (const auto& rule : utility::accessPolicyRules()) {
      if (!omitRules.count(rule.name)) {
        counterNames.push_back(rule.counterName);
      }
    }
    return utility::getAclInOutPacketsMap(getSw(), counterNames);
  }

  void verifyProbe(
      const utility::AccessPolicyProbe& probe,
      PortID ingressPort,
      cfg::AclLookupClassPort lookupClass,
      bool accessPolicyProgrammed,
      const std::set<std::string>& omitRules) {
    auto match = accessPolicyProgrammed
        ? utility::accessPolicyMatch(probe, lookupClass, omitRules)
        : std::nullopt;
    auto expectPermit =
        !match.has_value() || match->action == cfg::AclActionType::PERMIT;
    SCOPED_TRACE(
        fmt::format(
            "probe {} on port {} class {}: expect {} on {}",
            probe.name,
            static_cast<int>(ingressPort),
            apache::thrift::util::enumNameSafe(lookupClass),
            expectPermit ? "PERMIT" : "DROP",
            match.has_value() ? match->name : "no rule"));

    auto egressPort = masterLogicalInterfacePortIds()[kEgressPortIdx];
    auto countersBefore = accessPolicyProgrammed
        ? aclCounters(omitRules)
        : std::map<std::string, uint64_t>();
    auto egressPktsBefore =
        *getNextUpdatedPortStats(egressPort).outUnicastPkts_();

    ASSERT_TRUE(
        getSw()->sendPacketOutOfPortAsync(makeProbePacket(probe), ingressPort));

    WITH_RETRIES({
      auto egressPktsAfter =
          *getNextUpdatedPortStats(egressPort).outUnicastPkts_();
      EXPECT_EVENTUALLY_EQ(
          egressPktsAfter - egressPktsBefore, expectPermit ? 1 : 0);
      auto countersAfter = accessPolicyProgrammed
          ? aclCounters(omitRules)
          : std::map<std::string, uint64_t>();
      for (const auto& rule : utility::accessPolicyRules()) {
        auto before = countersBefore.find(rule.counterName);
        if (before == countersBefore.end()) {
          continue;
        }
        uint64_t expected =
            match.has_value() && match->name == rule.name ? 1 : 0;
        EXPECT_EVENTUALLY_EQ(
            countersAfter.at(rule.counterName) - before->second, expected)
            << "acl " << rule.name;
      }
    });
  }

  void verifyClass(
      int ingressPortIdx,
      cfg::AclLookupClassPort lookupClass,
      bool accessPolicyProgrammed,
      const std::set<std::string>& omitRules) {
    auto ingressPort = masterLogicalInterfacePortIds()[ingressPortIdx];
    for (const auto& probe : utility::accessPolicyProbes()) {
      verifyProbe(
          probe, ingressPort, lookupClass, accessPolicyProgrammed, omitRules);
    }
  }

  // A warm boot restores the SAI counters at their pre-reboot values and fb303
  // only picks them up on its first collection; without waiting, the first
  // probe reads that jump as its own traffic.
  void waitForStableAclCounters(const std::set<std::string>& omitRules) {
    auto previous = aclCounters(omitRules);
    WITH_RETRIES({
      getNextUpdatedPortStats(masterLogicalInterfacePortIds()[kEgressPortIdx]);
      auto current = aclCounters(omitRules);
      auto stable = current == previous;
      previous = current;
      EXPECT_EVENTUALLY_TRUE(stable);
    });
  }

  void verifyAccessPolicy(
      bool accessPolicyProgrammed,
      const std::set<std::string>& omitRules) {
    if (accessPolicyProgrammed) {
      waitForStableAclCounters(omitRules);
    }
    verifyClass(
        kRestrictedPortIdx, kRestricted, accessPolicyProgrammed, omitRules);
    verifyClass(
        kUnconstrainedPortIdx,
        kUnconstrained,
        accessPolicyProgrammed,
        omitRules);
  }
};

class AgentAccessPolicyClassIdAclTest : public AgentAccessPolicyAclTest {
 protected:
  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    return {
        ProductionFeature::ACCESS_POLICY_CLASS_ID_ACL,
        ProductionFeature::MULTI_ACL_TABLE,
        ProductionFeature::ACL_COUNTER,
        ProductionFeature::L3_FORWARDING};
  }
};

class AgentAccessPolicyPortBoundAclTest : public AgentAccessPolicyAclTest {
 protected:
  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    return {
        ProductionFeature::PORT_BOUND_INGRESS_ACL,
        ProductionFeature::L3_FORWARDING};
  }
};

template <typename BaseT>
class AgentAccessPolicyAclAddedTest : public BaseT {
 protected:
  bool coldBootWithAccessPolicy() const override {
    return false;
  }
};

template <typename BaseT>
class AgentAccessPolicyAclEntryAddedTest
    : public BaseT,
      public ::testing::WithParamInterface<std::string> {
 protected:
  std::set<std::string> coldBootOmitRules() const override {
    return {this->GetParam()};
  }
};

template <typename BaseT>
class AgentAccessPolicyAclEntryDeletedTest
    : public BaseT,
      public ::testing::WithParamInterface<std::string> {
 protected:
  std::set<std::string> warmBootOmitRules() const override {
    return {this->GetParam()};
  }
};

using AgentAccessPolicyClassIdAclAddedTest =
    AgentAccessPolicyAclAddedTest<AgentAccessPolicyClassIdAclTest>;
using AgentAccessPolicyPortBoundAclAddedTest =
    AgentAccessPolicyAclAddedTest<AgentAccessPolicyPortBoundAclTest>;
using AgentAccessPolicyClassIdAclEntryAddedTest =
    AgentAccessPolicyAclEntryAddedTest<AgentAccessPolicyClassIdAclTest>;
using AgentAccessPolicyPortBoundAclEntryAddedTest =
    AgentAccessPolicyAclEntryAddedTest<AgentAccessPolicyPortBoundAclTest>;
using AgentAccessPolicyClassIdAclEntryDeletedTest =
    AgentAccessPolicyAclEntryDeletedTest<AgentAccessPolicyClassIdAclTest>;
using AgentAccessPolicyPortBoundAclEntryDeletedTest =
    AgentAccessPolicyAclEntryDeletedTest<AgentAccessPolicyPortBoundAclTest>;

TEST_F(AgentAccessPolicyClassIdAclTest, AccessPolicyAcl) {
  runAccessPolicyTest();
}

TEST_F(AgentAccessPolicyPortBoundAclTest, AccessPolicyAcl) {
  runAccessPolicyTest();
}

TEST_F(AgentAccessPolicyClassIdAclAddedTest, AccessPolicyAclAddedOnWarmboot) {
  runAccessPolicyTest();
}

TEST_F(AgentAccessPolicyPortBoundAclAddedTest, AccessPolicyAclAddedOnWarmboot) {
  runAccessPolicyTest();
}

TEST_P(
    AgentAccessPolicyClassIdAclEntryAddedTest,
    AccessPolicyAclEntryAddedOnWarmboot) {
  runAccessPolicyTest();
}

TEST_P(
    AgentAccessPolicyPortBoundAclEntryAddedTest,
    AccessPolicyAclEntryAddedOnWarmboot) {
  runAccessPolicyTest();
}

TEST_P(
    AgentAccessPolicyClassIdAclEntryDeletedTest,
    AccessPolicyAclEntryDeletedOnWarmboot) {
  runAccessPolicyTest();
}

TEST_P(
    AgentAccessPolicyPortBoundAclEntryDeletedTest,
    AccessPolicyAclEntryDeletedOnWarmboot) {
  runAccessPolicyTest();
}

namespace {
std::string aclEntryTestName(
    const ::testing::TestParamInfo<std::string>& info) {
  auto name = info.param;
  std::replace(name.begin(), name.end(), '-', '_');
  return name;
}
} // namespace

INSTANTIATE_TEST_SUITE_P(
    AccessPolicy,
    AgentAccessPolicyClassIdAclEntryAddedTest,
    ::testing::ValuesIn(utility::accessPolicyRepresentativeRules()),
    aclEntryTestName);

INSTANTIATE_TEST_SUITE_P(
    AccessPolicy,
    AgentAccessPolicyPortBoundAclEntryAddedTest,
    ::testing::ValuesIn(utility::accessPolicyRepresentativeRules()),
    aclEntryTestName);

INSTANTIATE_TEST_SUITE_P(
    AccessPolicy,
    AgentAccessPolicyClassIdAclEntryDeletedTest,
    ::testing::ValuesIn(utility::accessPolicyRepresentativeRules()),
    aclEntryTestName);

INSTANTIATE_TEST_SUITE_P(
    AccessPolicy,
    AgentAccessPolicyPortBoundAclEntryDeletedTest,
    ::testing::ValuesIn(utility::accessPolicyRepresentativeRules()),
    aclEntryTestName);

} // namespace facebook::fboss
