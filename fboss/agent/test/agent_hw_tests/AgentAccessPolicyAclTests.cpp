// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <fmt/core.h>
#include <folly/IPAddressV6.h>
#include <folly/String.h>
#include <folly/io/Cursor.h>
#include <limits>
#include <thread>

#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/Utils.h"
#include "fboss/agent/packet/ICMPHdr.h"
#include "fboss/agent/packet/IPProto.h"
#include "fboss/agent/packet/IPv6Hdr.h"
#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/state/StateUtils.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/ResourceLibUtil.h"
#include "fboss/agent/test/utils/AccessPolicyAclTestUtils.h"
#include "fboss/agent/test/utils/AclTestUtils.h"
#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/agent/test/utils/PacketSnooper.h"
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

folly::IPAddressV4 kSrcIpV4() {
  return folly::IPAddressV4("10.0.0.2");
}

folly::MacAddress probeSrcMac(folly::MacAddress intfMac) {
  return utility::MacAddressGenerator().get(intfMac.u64HBO() + 1);
}

// DSCP 48, shifted into the IPv6 traffic class byte.
constexpr uint8_t kNetworkControlTrafficClass = 48 << 2;

// One second settle per round; stop early once no new punts arrive.
constexpr int kControlPlanePuntSettleRounds = 3;

} // namespace

class AgentAccessPolicyAclTest : public AgentHwTest {
 protected:
  void setCmdLineFlagOverrides() const override {
    AgentHwTest::setCmdLineFlagOverrides();
    FLAGS_enable_acl_table_group = true;
    // The base fixture collects these every stats tick, and a tick that
    // overruns a second costs every getNextUpdatedPortStats() an extra
    // second. This test reads none of them. Values are the HwSwitch.cpp
    // defaults the base fixture overrode.
    FLAGS_update_watermark_stats_interval_s = 60;
    FLAGS_update_voq_stats_interval_s = 60;
    FLAGS_update_cable_length_stats_s = 600;
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

  void runControlPlaneTest() {
    auto setup = [this]() { programRouteToEgressPort(); };
    auto verify = [this]() { verifyControlPlane(coldBootWithAccessPolicy()); };
    verifyAcrossWarmBoots(setup, verify);
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
    auto srcMac = probeSrcMac(intfMac);
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
    return makeIcmpV6Packet(
        vlanId,
        srcMac,
        intfMac,
        dstIp,
        ICMPv6Type::ICMPV6_TYPE_ECHO_REQUEST,
        kHopLimit);
  }

  static uint8_t tcpFlags(const utility::AccessPolicyProbe& probe) {
    auto flags = probe.tcpFlagsBitMap.value_or(0);
    CHECK_LE(flags, std::numeric_limits<uint8_t>::max())
        << "probe " << probe.name << " does not fit a TCP flags byte";
    return static_cast<uint8_t>(flags);
  }

  std::unique_ptr<TxPacket> makeIcmpV6Packet(
      std::optional<VlanID> vlanId,
      folly::MacAddress srcMac,
      folly::MacAddress dstMac,
      const folly::IPAddressV6& dstIp,
      ICMPv6Type icmpType,
      uint8_t hopLimit) {
    std::vector<uint8_t> body(56, 0xff);
    IPv6Hdr ipHdr(kSrcIp(), dstIp);
    ipHdr.nextHeader = static_cast<uint8_t>(IP_PROTO::IP_PROTO_IPV6_ICMP);
    ipHdr.payloadLength = ICMPHdr::SIZE + body.size();
    ipHdr.hopLimit = hopLimit;

    ICMPHdr icmpHdr(static_cast<uint8_t>(icmpType), 0 /*code*/, 0 /*csum*/);
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

  struct ControlPlanePacketAndDst {
    std::unique_ptr<TxPacket> pkt;
    std::optional<folly::IPAddress> dstIp;
  };

  // Everything the control plane builders need from the ingress port. Each port
  // is its own interface on its own VLAN and the switch retags on ingress, so
  // building with any other interface's VLAN makes the punted copy come back
  // unrecognisable to the snooper.
  struct ProbeContext {
    std::optional<VlanID> vlanId;
    folly::MacAddress srcMac;
    folly::MacAddress intfMac;
    folly::IPAddressV4 myIpV4;
    folly::IPAddressV6 myIpV6;
    uint16_t l4SrcPort{0};
    uint16_t l4DstPort{0};
  };

  ProbeContext probeContext(
      const utility::ControlPlaneProbe& probe,
      PortID ingressPort) {
    auto state = getProgrammedState();
    auto intfId = getInterfaceIDForPort(ingressPort, state);
    auto intf = state->getInterfaces()->getNode(intfId);
    auto v6Addrs = utility::getIntfAddrsV6(state, intfId);
    auto v4Addrs = utility::getIntfAddrsV4(state, intfId);
    CHECK(!v6Addrs.empty() && !v4Addrs.empty())
        << "interface " << intfId << " needs both a v4 and a v6 address";
    auto toL4Port = [&probe](std::optional<int32_t> port) {
      auto value = port.value_or(0);
      CHECK_GE(value, 0);
      CHECK_LE(value, std::numeric_limits<uint16_t>::max())
          << "probe " << probe.name << " L4 port does not fit 16 bits";
      return static_cast<uint16_t>(value);
    };
    return ProbeContext{
        getSw()->getVlanIDForTx(intfId),
        probeSrcMac(intf->getMac()),
        intf->getMac(),
        v4Addrs[0],
        v6Addrs[0],
        toL4Port(probe.policyMatch.l4SrcPort),
        toL4Port(probe.policyMatch.l4DstPort)};
  }

  ControlPlanePacketAndDst udpV6Probe(
      const ProbeContext& ctx,
      const folly::IPAddressV6& dstIp,
      uint8_t trafficClass,
      uint8_t hopLimit) {
    return ControlPlanePacketAndDst{
        utility::makeUDPTxPacket(
            getSw(),
            ctx.vlanId,
            ctx.srcMac,
            ctx.intfMac,
            kSrcIp(),
            dstIp,
            ctx.l4SrcPort,
            ctx.l4DstPort,
            trafficClass,
            hopLimit),
        folly::IPAddress(dstIp)};
  }

  ControlPlanePacketAndDst arpProbe(const ProbeContext& ctx, ARP_OPER oper) {
    return ControlPlanePacketAndDst{
        utility::makeARPTxPacket(
            getSw(),
            ctx.vlanId,
            ctx.srcMac,
            oper == ARP_OPER::ARP_OPER_REQUEST ? folly::MacAddress::BROADCAST
                                               : ctx.intfMac,
            kSrcIpV4(),
            ctx.myIpV4,
            oper),
        std::nullopt};
  }

  ControlPlanePacketAndDst makeControlPlanePacket(
      const utility::ControlPlaneProbe& probe,
      PortID ingressPort) {
    auto ctx = probeContext(probe, ingressPort);
    switch (probe.packet) {
      case utility::ControlPlanePacket::ArpRequest:
        return arpProbe(ctx, ARP_OPER::ARP_OPER_REQUEST);
      case utility::ControlPlanePacket::ArpReply:
        return arpProbe(ctx, ARP_OPER::ARP_OPER_REPLY);
      case utility::ControlPlanePacket::Ip2Me:
        return udpV6Probe(ctx, ctx.myIpV6, 0 /*trafficClass*/, kHopLimit);
      case utility::ControlPlanePacket::Ip2MeNetworkControl:
        return udpV6Probe(
            ctx, ctx.myIpV6, kNetworkControlTrafficClass, kHopLimit);
    }
    throw FbossError(
        "Unhandled control plane packet ", static_cast<int>(probe.packet));
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

  // A probe's expected outcome: which rule claims it, and whether that lets it
  // through. Shared so the batch and the single probe path cannot disagree.
  struct ProbeOutcome {
    std::optional<utility::AccessPolicyRule> match;
    bool permit{true};
  };

  static ProbeOutcome probeOutcome(
      const utility::AccessPolicyProbe& probe,
      cfg::AclLookupClassPort lookupClass,
      bool accessPolicyProgrammed,
      const std::set<std::string>& omitRules) {
    auto match = accessPolicyProgrammed
        ? utility::accessPolicyMatch(probe, lookupClass, omitRules)
        : std::nullopt;
    return {
        match,
        !match.has_value() || match->action == cfg::AclActionType::PERMIT};
  }

  void verifyProbe(
      const utility::AccessPolicyProbe& probe,
      PortID ingressPort,
      cfg::AclLookupClassPort lookupClass,
      bool accessPolicyProgrammed,
      const std::set<std::string>& omitRules) {
    auto outcome =
        probeOutcome(probe, lookupClass, accessPolicyProgrammed, omitRules);
    SCOPED_TRACE(
        fmt::format(
            "probe {} expects {}",
            probe.name,
            outcome.match.has_value() ? outcome.match->name : "no rule"));
    std::map<std::string, uint64_t> expectedCounters;
    if (outcome.match.has_value()) {
      expectedCounters[outcome.match->counterName] = 1;
    }
    verifyBatch(
        {&probe},
        expectedCounters,
        ingressPort,
        lookupClass,
        outcome.permit,
        accessPolicyProgrammed,
        omitRules);
  }

  // Permit and drop stay in separate batches: the egress count is one number,
  // so mixing them lets a rule that wrongly permits cancel one that wrongly
  // drops.
  void verifyBatch(
      const std::vector<const utility::AccessPolicyProbe*>& probes,
      const std::map<std::string, uint64_t>& expectedCounters,
      PortID ingressPort,
      cfg::AclLookupClassPort lookupClass,
      bool expectPermit,
      bool accessPolicyProgrammed,
      const std::set<std::string>& omitRules) {
    if (probes.empty()) {
      return;
    }
    SCOPED_TRACE(
        fmt::format(
            "{} probes on port {} class {}: expect {}",
            probes.size(),
            static_cast<int>(ingressPort),
            apache::thrift::util::enumNameSafe(lookupClass),
            expectPermit ? "PERMIT" : "DROP"));

    auto egressPort = masterLogicalInterfacePortIds()[kEgressPortIdx];
    auto countersBefore = accessPolicyProgrammed
        ? aclCounters(omitRules)
        : std::map<std::string, uint64_t>();
    auto egressPktsBefore =
        *getNextUpdatedPortStats(egressPort).outUnicastPkts_();

    for (const auto* probe : probes) {
      ASSERT_TRUE(
          getSw()->sendPacketOutOfPortAsync(
              makeProbePacket(*probe), ingressPort));
    }

    auto expectedEgress =
        expectPermit ? static_cast<int64_t>(probes.size()) : 0;
    bool matched = false;
    WITH_RETRIES({
      auto egressPktsAfter =
          *getNextUpdatedPortStats(egressPort).outUnicastPkts_();
      matched = (egressPktsAfter - egressPktsBefore == expectedEgress);
      EXPECT_EVENTUALLY_EQ(egressPktsAfter - egressPktsBefore, expectedEgress);
      if (accessPolicyProgrammed) {
        auto countersAfter = aclCounters(omitRules);
        for (const auto& rule : utility::accessPolicyRules()) {
          auto before = countersBefore.find(rule.counterName);
          if (before == countersBefore.end()) {
            continue;
          }
          auto expected = expectedCounters.find(rule.counterName);
          uint64_t expectedDelta =
              expected == expectedCounters.end() ? 0 : expected->second;
          auto delta = countersAfter.at(rule.counterName) - before->second;
          matched &= (delta == expectedDelta);
          EXPECT_EVENTUALLY_EQ(delta, expectedDelta) << "acl " << rule.name;
        }
      }
    });

    // Error signal only. The batch has already failed, and it names the rule
    // whose counter is wrong rather than the probe, so re-send each probe on
    // its own to get a log that names the offending one.
    if (!matched && probes.size() > 1) {
      for (const auto* probe : probes) {
        verifyProbe(
            *probe,
            ingressPort,
            lookupClass,
            accessPolicyProgrammed,
            omitRules);
      }
    }
  }

  void verifyClass(
      int ingressPortIdx,
      cfg::AclLookupClassPort lookupClass,
      bool accessPolicyProgrammed,
      const std::set<std::string>& omitRules) {
    auto ingressPort = masterLogicalInterfacePortIds()[ingressPortIdx];
    std::vector<const utility::AccessPolicyProbe*> permitProbes, dropProbes;
    std::map<std::string, uint64_t> permitCounters, dropCounters;
    for (const auto& probe : utility::accessPolicyProbes()) {
      auto [match, permit] =
          probeOutcome(probe, lookupClass, accessPolicyProgrammed, omitRules);
      (permit ? permitProbes : dropProbes).push_back(&probe);
      if (match.has_value()) {
        // Omitting a rule drops its probe onto a later one, so two probes can
        // share a counter.
        ++(permit ? permitCounters : dropCounters)[match->counterName];
      }
    }
    verifyBatch(
        permitProbes,
        permitCounters,
        ingressPort,
        lookupClass,
        true /*expectPermit*/,
        accessPolicyProgrammed,
        omitRules);
    verifyBatch(
        dropProbes,
        dropCounters,
        ingressPort,
        lookupClass,
        false /*expectPermit*/,
        accessPolicyProgrammed,
        omitRules);
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

  // Sends every probe, then settles once. A snooper per probe claims only its
  // own frame; a packet can be punted by more than one mechanism at once, so
  // unclaimed frames are not a failure. Waiting per probe instead would cost
  // the full settle for each one that is never punted.
  std::map<std::string, bool> sendControlPlaneProbes(
      PortID ingressPort,
      std::vector<ControlPlanePacketAndDst>& packets) {
    const auto& probes = utility::controlPlaneProbes();
    CHECK_EQ(packets.size(), probes.size());
    std::vector<std::unique_ptr<utility::SwSwitchPacketSnooper>> snoopers;
    for (size_t i = 0; i < probes.size(); ++i) {
      snoopers.push_back(
          std::make_unique<utility::SwSwitchPacketSnooper>(
              getSw(),
              fmt::format("control-plane-{}", probes[i].name),
              std::nullopt /*port*/,
              utility::makeEthFrame(
                  *packets[i].pkt, true /*skipTtlDecrement*/)));
      snoopers.back()->ignoreUnclaimedRxPkts();
    }
    for (size_t i = 0; i < probes.size(); ++i) {
      EXPECT_TRUE(
          getSw()->sendPacketOutOfPortAsync(
              std::move(packets[i].pkt), ingressPort))
          << "failed to send control plane probe " << probes[i].name;
    }

    // Keep settling while punts are still arriving, so a probe that is merely
    // slow is not recorded as one the platform does not trap.
    std::map<std::string, bool> punted;
    size_t seen = 0;
    for (int round = 0; round < kControlPlanePuntSettleRounds; ++round) {
      /* sleep override */
      std::this_thread::sleep_for(std::chrono::seconds(1));
      size_t nowSeen = 0;
      for (size_t i = 0; i < probes.size(); ++i) {
        punted[probes[i].name] = snoopers[i]->receivedPacket();
        nowSeen += punted[probes[i].name] ? 1 : 0;
      }
      if (round > 0 && nowSeen == seen) {
        break;
      }
      seen = nowSeen;
    }
    return punted;
  }

  std::map<std::string, bool> verifyControlPlaneClass(
      int ingressPortIdx,
      cfg::AclLookupClassPort lookupClass,
      bool accessPolicyProgrammed,
      const std::map<std::string, bool>& baselinePunted) {
    auto ingressPort = masterLogicalInterfacePortIds()[ingressPortIdx];
    auto egressPort = masterLogicalInterfacePortIds()[kEgressPortIdx];
    auto className = apache::thrift::util::enumNameSafe(lookupClass);
    auto egressBefore = *getNextUpdatedPortStats(egressPort).outUnicastPkts_();

    std::vector<ControlPlanePacketAndDst> packets;
    std::vector<std::optional<utility::AccessPolicyRule>> matches;
    for (const auto& probe : utility::controlPlaneProbes()) {
      packets.push_back(makeControlPlanePacket(probe, ingressPort));
      auto policyProbe = probe.policyMatch;
      if (packets.back().dstIp.has_value()) {
        policyProbe.dstIp = packets.back().dstIp->str();
      }
      matches.push_back(
          accessPolicyProgrammed
              ? utility::accessPolicyMatch(policyProbe, lookupClass)
              : std::nullopt);
    }
    auto punted = sendControlPlaneProbes(ingressPort, packets);

    std::vector<std::string> report;
    for (size_t i = 0; i < utility::controlPlaneProbes().size(); ++i) {
      const auto& probe = utility::controlPlaneProbes()[i];
      const auto& match = matches[i];
      auto denied =
          match.has_value() && match->action == cfg::AclActionType::DENY;
      auto reachedCpu = punted[probe.name];

      auto baseline = baselinePunted.find(probe.name);
      auto trapped = baseline != baselinePunted.end() && baseline->second;
      report.push_back(
          fmt::format(
              "{:34} {:36} {:6} {:8} {}",
              probe.name,
              match.has_value() ? match->name : "no rule",
              denied ? "DENY" : "PERMIT",
              reachedCpu ? "PUNTED" : "no punt",
              baseline != baselinePunted.end() && !trapped
                  ? "(not trapped at baseline)"
                  : ""));
      // Whether a deny stops a punt is recorded, not asserted; it varies by
      // ASIC. Suppressing a packet the policy permits is always wrong.
      if (!denied && trapped) {
        EXPECT_TRUE(reachedCpu)
            << "permitted control plane packet " << probe.name
            << " did not reach the CPU on class " << className;
      }
    }

    XLOG(INFO) << "Control plane punt report for class " << className << ":\n"
               << folly::join("\n", report);

    // Two ticks: the first may land before the last probe was counted.
    getNextUpdatedPortStats(egressPort);
    EXPECT_EQ(
        *getNextUpdatedPortStats(egressPort).outUnicastPkts_() - egressBefore,
        0)
        << "control plane packets were forwarded on class " << className;
    return punted;
  }

  void verifyControlPlane(bool accessPolicyProgrammed) {
    // The unconstrained port has no policy bound, so what it punts is the
    // baseline: a trap the platform lacks must not read as a policy drop.
    auto baseline = verifyControlPlaneClass(
        kUnconstrainedPortIdx, kUnconstrained, accessPolicyProgrammed, {});
    // Differencing against a baseline only says anything if the baseline is
    // complete, so name any probe the platform failed to trap and stop rather
    // than let the rest of the test pass vacuously.
    bool baselineComplete = true;
    for (const auto& [name, punted] : baseline) {
      if (!punted) {
        baselineComplete = false;
        ADD_FAILURE() << "control plane probe " << name
                      << " never reached the CPU on an unconstrained port";
      }
    }
    if (!baselineComplete) {
      return;
    }
    verifyControlPlaneClass(
        kRestrictedPortIdx, kRestricted, accessPolicyProgrammed, baseline);
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

// AclTable1 holds the CPU policing entries, which the forwarding tests above
// must not see.
template <typename BaseT>
class AgentAccessPolicyControlPlaneTest : public BaseT {
 protected:
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    return this->addCoppConfig(ensemble, BaseT::initialConfig(ensemble));
  }

  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    auto features = BaseT::getProductionFeaturesVerified();
    features.push_back(ProductionFeature::COPP);
    return features;
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
using AgentAccessPolicyClassIdControlPlaneTest =
    AgentAccessPolicyControlPlaneTest<AgentAccessPolicyClassIdAclTest>;
using AgentAccessPolicyPortBoundControlPlaneTest =
    AgentAccessPolicyControlPlaneTest<AgentAccessPolicyPortBoundAclTest>;
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

TEST_F(AgentAccessPolicyClassIdControlPlaneTest, ControlPlanePunt) {
  runControlPlaneTest();
}

TEST_F(AgentAccessPolicyPortBoundControlPlaneTest, ControlPlanePunt) {
  runControlPlaneTest();
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
