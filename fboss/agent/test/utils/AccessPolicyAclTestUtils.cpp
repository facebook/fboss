// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/utils/AccessPolicyAclTestUtils.h"

#include <algorithm>

#include <fmt/core.h>
#include <folly/IPAddress.h>
#include <folly/logging/xlog.h>
#include <thrift/lib/cpp/util/EnumUtils.h>

#include "fboss/agent/AsicUtils.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/packet/IPProto.h"
#include "fboss/agent/test/utils/AclTestUtils.h"
#include "fboss/agent/test/utils/ConfigUtils.h"

namespace facebook::fboss::utility {

namespace {

constexpr auto kTcp = static_cast<int16_t>(IP_PROTO::IP_PROTO_TCP);
constexpr auto kUdp = static_cast<int16_t>(IP_PROTO::IP_PROTO_UDP);
constexpr auto kIcmpV6 = static_cast<int16_t>(IP_PROTO::IP_PROTO_IPV6_ICMP);
constexpr auto kIpv6Encap = static_cast<int16_t>(IP_PROTO::IP_PROTO_IPV6);
constexpr int16_t kTcpSyn = 2;
constexpr int32_t kBgpL4Port = 179;
// Mirrors DHCPv4Handler::kBootP{S,C}Port and
// DHCPv6Packet::DHCP6_{SERVERAGENT,CLIENT}_UDPPORT, which live in targets too
// heavy to pull into this util.
constexpr int32_t kDhcpV4ServerPort = 67;
constexpr int32_t kDhcpV4ClientPort = 68;
constexpr int32_t kDhcpV6ServerPort = 547;
constexpr int32_t kDhcpV6ClientPort = 546;

// A SAI ACL counter label is char[32]. SaiAclTableManager::addAclCounter throws
// an uncaught FbossError on a longer name, which aborts the hw agent.
constexpr size_t kMaxAclNameLen = 31;

// ACCESS_POLICY_RESTRICT_ACL_TABLE_PRIORITY in access_policy_acl.cinc.
constexpr int32_t kRestrictedTablePriority = 1;

// AclTable1 configures priority 0, which SaiAclTableManager floors to 23. This
// table shares the default group with it, so anything at or below 23 leaves the
// two in an undefined order.
constexpr int kAccessPolicyClassIdTablePriority = 30;

// A probe for a rule with no port qualifier still has to carry some port, and
// one a rule does qualify on would hand the probe to that rule instead. Search
// from the candidate rather than hardcoding, so adding a rule cannot silently
// collide.
int32_t unmatchedL4Port(int32_t candidate) {
  auto claimed = [](int32_t port) {
    return std::any_of(
        kAccessPolicyVersions.begin(),
        kAccessPolicyVersions.end(),
        [port](auto version) {
          const auto& rules = accessPolicyRules(version);
          return std::any_of(
              rules.begin(), rules.end(), [port](const auto& rule) {
                return rule.l4DstPort == port || rule.l4SrcPort == port;
              });
        });
  };
  while (claimed(candidate)) {
    ++candidate;
  }
  return candidate;
}

int32_t unmatchedL4DstPort() {
  static const int32_t port = unmatchedL4Port(9999);
  return port;
}

int32_t unmatchedL4SrcPort() {
  static const int32_t port = unmatchedL4Port(4321);
  return port;
}

// ACCESS_POLICY_RESTRICT_ACL_QUALIFIERS in
// configerator/source/neteng/fboss/coop/templates/access_policy_acl.cinc.
std::vector<cfg::AclTableQualifier> restrictedTableQualifiers() {
  return {
      cfg::AclTableQualifier::L4_SRC_PORT,
      cfg::AclTableQualifier::L4_DST_PORT,
      cfg::AclTableQualifier::IP_PROTOCOL_NUMBER,
      cfg::AclTableQualifier::ICMPV4_TYPE,
      cfg::AclTableQualifier::ICMPV4_CODE,
      cfg::AclTableQualifier::ICMPV6_TYPE,
      cfg::AclTableQualifier::ICMPV6_CODE,
      cfg::AclTableQualifier::TCP_FLAGS,
      cfg::AclTableQualifier::ETHER_TYPE,
      cfg::AclTableQualifier::DST_IPV6,
      cfg::AclTableQualifier::DST_IPV4};
}

std::vector<cfg::AclTableQualifier> classIdTableQualifiers() {
  auto qualifiers = restrictedTableQualifiers();
  qualifiers.push_back(cfg::AclTableQualifier::LOOKUP_CLASS_PORT);
  return qualifiers;
}

std::vector<cfg::AclTableActionType> accessPolicyActionTypes() {
  // Production leaves actionTypes empty; the tests need the per entry counters.
  return {
      cfg::AclTableActionType::PACKET_ACTION, cfg::AclTableActionType::COUNTER};
}

cfg::AclTable makeAclTable(
    const std::string& name,
    int32_t priority,
    std::vector<cfg::AclTableQualifier> qualifiers) {
  cfg::AclTable table;
  table.name() = name;
  table.priority() = priority;
  table.actionTypes() = accessPolicyActionTypes();
  table.qualifiers() = std::move(qualifiers);
  return table;
}

// The rule set the Safety team ships, in configerator order. ACL match is
// first hit, so the order is part of the contract.
//
// This file is open sourced, so a rule named after an internal service is
// renamed to s<N>, its destination addresses are replaced with RFC 3849
// documentation addresses, and a port that identifies an internal service is
// replaced with an arbitrary one. Only the IANA well known ports the rule names
// call out, and the protocol and TCP flag criteria, stay faithful.
std::vector<AccessPolicyRule> buildAccessPolicyRules() {
  std::vector<AccessPolicyRule> rules;
  auto add = [&rules](AccessPolicyRule rule) {
    rule.counterName = fmt::format("ap0-{:02d}", rules.size());
    CHECK_LE(rule.counterName.size(), kMaxAclNameLen);
    rules.push_back(std::move(rule));
  };
  auto addTcpDstPorts = [&add](
                            const std::string& prefix,
                            const std::vector<int32_t>& l4DstPorts) {
    for (size_t i = 0; i < l4DstPorts.size(); ++i) {
      add(
          {.name = fmt::format("{}-{}", prefix, i + 1),
           .proto = kTcp,
           .l4DstPort = l4DstPorts[i]});
    }
  };
  auto addS1DstIps = [&add](
                         const std::string& prefix,
                         const std::vector<std::string>& dstIps) {
    for (size_t i = 0; i < dstIps.size(); ++i) {
      add(
          {.name = fmt::format("{}-{}", prefix, i + 1),
           .proto = kTcp,
           .l4DstPort = 8080,
           .dstIp = dstIps[i]});
    }
  };

  add({.name = "restrict-permit-icmpv6", .proto = kIcmpV6});
  add({.name = "restrict-permit-lldp", .etherType = cfg::EtherType::LLDP});
  for (auto l4DstPort : {67, 68, 546, 547}) {
    add(
        {.name = fmt::format("restrict-permit-dhcp-{}", l4DstPort),
         .proto = kUdp,
         .l4DstPort = l4DstPort});
  }
  add({.name = "restrict-permit-53", .l4DstPort = 53});
  add({.name = "restrict-permit-udp-123", .proto = kUdp, .l4DstPort = 123});
  add({.name = "restrict-permit-443", .proto = kTcp, .l4DstPort = 443});
  add({.name = "restrict-permit-s0-vip-prefix", .dstIp = "2001:db8:9a00::/40"});
  addS1DstIps(
      "restrict-permit-8080-s1",
      {"2001:db8:59bb:58d1::2b24/128",
       "2001:db8:5cf6:345b::3601/128",
       "2001:db8:1f36:27bc::906d/128",
       "2001:db8:e839:1908::cb6a/128",
       "2001:db8:2833:f3c8::c42a/128",
       "2001:db8:b975:9521::525f/128",
       "2001:db8:42c2:593d::faf1/128",
       "2001:db8:88fb:a901::ccde/128",
       "2001:db8:3383:8b22::e8eb/128"});
  addS1DstIps(
      "restrict-permit-8080-s1-rc",
      {"2001:db8:6f21:4c08::a13e/128",
       "2001:db8:d40b:1e93::7c52/128",
       "2001:db8:a5c:b27f::e094/128",
       "2001:db8:91d8:3fa6::4b71/128"});
  add({.name = "restrict-permit-s19", .l4DstPort = 32483});
  add({.name = "restrict-permit-8443", .l4DstPort = 8443});
  add({.name = "restrict-permit-tcp-s20", .proto = kTcp, .l4DstPort = 45655});
  addTcpDstPorts("restrict-permit-tcp-s2", {42311, 42312, 42313});
  addTcpDstPorts(
      "restrict-permit-tcp-s3",
      {2034, 2035, 2036, 2037, 2038, 2039, 2040, 2041, 2042, 2043, 2044});
  add({.name = "restrict-permit-tcp-s21", .proto = kTcp, .l4DstPort = 30315});
  addTcpDstPorts(
      "restrict-permit-tcp-s4",
      {6245, 39636, 23575, 58445, 28490, 21193, 36439});
  addTcpDstPorts("restrict-permit-tcp-s5", {14429, 14430, 14431, 14432});
  add({.name = "restrict-permit-tcp-s6", .proto = kTcp, .l4DstPort = 48197});
  addTcpDstPorts("restrict-permit-tcp-s7", {23450, 23451, 23452});
  add({.name = "restrict-permit-tcp-s8", .proto = kTcp, .l4DstPort = 43918});
  addTcpDstPorts("restrict-permit-tcp-s9", {27318, 26591});
  addTcpDstPorts("restrict-permit-tcp-s10", {10405, 10406});
  addTcpDstPorts("restrict-permit-tcp-s11", {30146, 30147});
  add({.name = "restrict-permit-tcp-s12", .proto = kTcp, .l4DstPort = 2200});
  addTcpDstPorts("restrict-permit-tcp-s13", {8414, 24971});
  add({.name = "restrict-permit-tcp-s14", .proto = kTcp, .l4DstPort = 44795});
  addTcpDstPorts("restrict-permit-tcp-s15", {16163, 5980});
  addTcpDstPorts("restrict-permit-tcp-s16", {54978, 41691, 3157});
  add({.name = "restrict-permit-tcp-s17", .proto = kTcp, .l4DstPort = 31960});
  add({.name = "restrict-permit-tcp-s18", .proto = kTcp, .l4DstPort = 44824});
  add(
      {.name = "restrict-permit-tcp-ssh-response",
       .proto = kTcp,
       .l4SrcPort = 17206});
  add(
      {.name = "restrict-permit-udp-rtc-response",
       .proto = kUdp,
       .l4SrcPort = 4614});
  add(
      {.name = "restrict-deny-tcp-syn",
       .action = cfg::AclActionType::DENY,
       .proto = kTcp,
       .tcpFlagsBitMap = kTcpSyn});
  add({.name = "restrict-permit-tcp", .proto = kTcp});
  add(
      {.name = "restrict-deny-6",
       .action = cfg::AclActionType::DENY,
       .l4DstPort = 6});
  add({.name = "restrict-deny", .action = cfg::AclActionType::DENY});

  return rules;
}

// access_policy_rules_v1.cinc, anonymised as above. A service keeps the s<N> it
// already has, so a rule that survives the upgrade keeps its name and the warm
// boot delta stays honest.
std::vector<AccessPolicyRule> buildAccessPolicyRulesV1() {
  std::vector<AccessPolicyRule> rules;
  auto add = [&rules](AccessPolicyRule rule) {
    rule.counterName = fmt::format("ap1-{:03d}", rules.size());
    CHECK_LE(rule.counterName.size(), kMaxAclNameLen);
    rules.push_back(std::move(rule));
  };
  auto addTcpDstPorts = [&add](
                            const std::string& prefix,
                            const std::vector<int32_t>& l4DstPorts) {
    for (size_t i = 0; i < l4DstPorts.size(); ++i) {
      add(
          {.name = fmt::format("{}-{}", prefix, i + 1),
           .proto = kTcp,
           .l4DstPort = l4DstPorts[i]});
    }
  };
  auto addTls443DstIps = [&add](
                             const std::string& namePattern,
                             const std::string& dstIpPattern,
                             int count) {
    for (int i = 1; i <= count; ++i) {
      add(
          {.name = fmt::format(fmt::runtime(namePattern), i),
           .proto = kTcp,
           .l4DstPort = 443,
           .dstIp = fmt::format(fmt::runtime(dstIpPattern), i)});
    }
  };

  add({.name = "restrict-permit-icmpv6", .proto = kIcmpV6});
  add({.name = "restrict-permit-lldp", .etherType = cfg::EtherType::LLDP});
  for (auto l4DstPort : {67, 68, 546, 547}) {
    add(
        {.name = fmt::format("restrict-permit-dhcp-{}", l4DstPort),
         .proto = kUdp,
         .l4DstPort = l4DstPort});
  }

  add(
      {.name = "restrict-permit-53-1",
       .l4DstPort = 53,
       .dstIp = "2001:db8:d000:1::5/128"});
  add(
      {.name = "restrict-permit-53-2",
       .l4DstPort = 53,
       .dstIp = "2001:db8:d000:2::5/128"});
  add({.name = "restrict-permit-udp-123", .proto = kUdp, .l4DstPort = 123});
  addTls443DstIps("restrict-permit-s0-tmp{:02d}", "2001:db8:a0{:02x}::/56", 36);
  addTls443DstIps("restrict-permit-s22-{:02d}", "2001:db8:b0{:02x}::/56", 40);
  addTls443DstIps("restrict-permit-s5-{:02d}", "2001:db8:c000:{:x}::1/128", 37);
  addTls443DstIps("restrict-permit-s23-{}", "2001:db8:c100:{:x}::2/128", 3);
  addTls443DstIps("restrict-permit-s24-{}", "2001:db8:c200:{:x}::3/128", 3);
  addTls443DstIps("restrict-permit-s25-{}", "2001:db8:c300:{:x}::4/128", 4);
  add({.name = "restrict-permit-vm-iso-vip", .dstIp = "2001:db8:9a00::/40"});
  add({.name = "restrict-permit-s19", .l4DstPort = 32483});
  add({.name = "restrict-permit-tcp-s20", .proto = kTcp, .l4DstPort = 45655});

  // Surviving members keep the index they had, so a removal does not renumber
  // the rules around it.
  add({.name = "restrict-permit-tcp-s2-2", .proto = kTcp, .l4DstPort = 42312});
  add({.name = "restrict-permit-tcp-s2-3", .proto = kTcp, .l4DstPort = 42313});
  addTcpDstPorts(
      "restrict-permit-tcp-s3",
      {2034, 2035, 2036, 2037, 2038, 2039, 2040, 2041, 2042, 2043, 2044});
  add({.name = "restrict-permit-tcp-s21", .proto = kTcp, .l4DstPort = 30315});
  addTcpDstPorts("restrict-permit-tcp-s4-v1", {19844, 19845});
  add({.name = "restrict-permit-tcp-s4-2", .proto = kTcp, .l4DstPort = 39636});
  add({.name = "restrict-permit-tcp-s4-3", .proto = kTcp, .l4DstPort = 23575});
  add({.name = "restrict-permit-tcp-s4-4", .proto = kTcp, .l4DstPort = 58445});
  add({.name = "restrict-permit-tcp-s7-3", .proto = kTcp, .l4DstPort = 23452});
  add(
      {.name = "restrict-permit-tcp-s12-tls",
       .proto = kTcp,
       .l4DstPort = 26205,
       .dstIp = "2001:db8:d000:4::7/128"});
  add({.name = "restrict-permit-tcp-s13-2", .proto = kTcp, .l4DstPort = 24971});
  add({.name = "restrict-permit-tcp-s14", .proto = kTcp, .l4DstPort = 44795});
  add({.name = "restrict-permit-tcp-s18", .proto = kTcp, .l4DstPort = 44824});

  for (int i = 1; i <= 11; ++i) {
    add(
        {.name = fmt::format("restrict-permit-udp-rtc-{:02d}", i),
         .proto = kUdp,
         .l4SrcPort = 4614,
         .dstIp = fmt::format("2001:db8:e0{:02x}::/56", i)});
  }

  add(
      {.name = "restrict-deny-6",
       .action = cfg::AclActionType::DENY,
       .l4DstPort = 6});

  // Ahead of the SYN deny, so this expected polling lands on its own counter
  // rather than the generic one.
  add(
      {.name = "restrict-deny-tcp-s26-1",
       .action = cfg::AclActionType::DENY,
       .proto = kTcp,
       .l4DstPort = 34411});
  add(
      {.name = "restrict-deny-tcp-s26-2",
       .action = cfg::AclActionType::DENY,
       .proto = kTcp,
       .l4DstPort = 34412});

  add(
      {.name = "restrict-deny-tcp-syn",
       .action = cfg::AclActionType::DENY,
       .proto = kTcp,
       .tcpFlagsBitMap = kTcpSyn});
  add({.name = "restrict-permit-tcp-return", .proto = kTcp});

  add(
      {.name = "restrict-deny-udp-s27",
       .action = cfg::AclActionType::DENY,
       .proto = kUdp,
       .l4SrcPort = 37012});
  add(
      {.name = "restrict-deny-ipv6-encap",
       .action = cfg::AclActionType::DENY,
       .proto = kIpv6Encap});

  add({.name = "restrict-deny", .action = cfg::AclActionType::DENY});

  return rules;
}

const std::string& tableForShape(AccessPolicyShape shape) {
  static const std::string kClassId = kAccessPolicyClassIdTable();
  static const std::string kRestrictedName = kAccessPolicyRestrictedTable();
  return shape == AccessPolicyShape::ClassId ? kClassId : kRestrictedName;
}

cfg::AclEntry makeAclEntry(
    AccessPolicyShape shape,
    const AccessPolicyRule& rule,
    cfg::AclActionType denyAction) {
  cfg::AclEntry entry;
  entry.name() = rule.name;
  entry.actionType() =
      rule.action == cfg::AclActionType::DENY ? denyAction : rule.action;
  if (shape == AccessPolicyShape::ClassId) {
    entry.lookupClassPort() = rule.lookupClass;
  }
  if (rule.proto.has_value()) {
    entry.proto() = *rule.proto;
  }
  if (rule.l4DstPort.has_value()) {
    entry.l4DstPort() = *rule.l4DstPort;
  }
  if (rule.l4SrcPort.has_value()) {
    entry.l4SrcPort() = *rule.l4SrcPort;
  }
  if (rule.tcpFlagsBitMap.has_value()) {
    entry.tcpFlagsBitMap() = *rule.tcpFlagsBitMap;
  }
  if (rule.dstIp.has_value()) {
    entry.dstIp() = *rule.dstIp;
  }
  if (rule.etherType.has_value()) {
    entry.etherType() = *rule.etherType;
  }
  return entry;
}

bool ruleMatchesProbe(
    const AccessPolicyRule& rule,
    const AccessPolicyProbe& probe) {
  if (rule.etherType.has_value() && rule.etherType != probe.etherType) {
    return false;
  }
  if (rule.proto.has_value() && rule.proto != probe.proto) {
    return false;
  }
  if (rule.l4DstPort.has_value() && rule.l4DstPort != probe.l4DstPort) {
    return false;
  }
  if (rule.l4SrcPort.has_value() && rule.l4SrcPort != probe.l4SrcPort) {
    return false;
  }
  // Unlike the other qualifiers, hardware matches TCP flags under a mask: the
  // rule's bits have to be set in the packet, and the rest are don't care.
  if (rule.tcpFlagsBitMap.has_value() &&
      (probe.tcpFlagsBitMap.value_or(0) & *rule.tcpFlagsBitMap) !=
          *rule.tcpFlagsBitMap) {
    return false;
  }
  if (rule.dstIp.has_value()) {
    if (!probe.dstIp.has_value()) {
      return false;
    }
    auto [prefix, length] = folly::IPAddress::createNetwork(*rule.dstIp);
    folly::IPAddress dstIp(*probe.dstIp);
    // inSubnet() throws on a family mismatch; a v4 probe simply misses a v6
    // prefix.
    if (dstIp.family() != prefix.family() || !dstIp.inSubnet(prefix, length)) {
      return false;
    }
  }
  return true;
}

std::vector<AccessPolicyProbe> buildAccessPolicyProbes(
    AccessPolicyVersion version) {
  std::vector<AccessPolicyProbe> probes;
  for (const auto& rule : accessPolicyRules(version)) {
    // makeProbePacket() emits TCP, UDP and ICMPv6 frames only. A rule matching
    // anything else is still programmed, and verifyBatch holds its counter at
    // zero; it just has no probe of its own.
    if (rule.etherType.has_value() ||
        (rule.proto.has_value() && *rule.proto != kTcp && *rule.proto != kUdp &&
         *rule.proto != kIcmpV6)) {
      continue;
    }
    AccessPolicyProbe probe;
    probe.name = rule.name;
    // UDP is the protocol no protocol-qualified rule ahead of these matches.
    probe.proto = rule.proto.value_or(kUdp);
    if (probe.proto == kTcp || probe.proto == kUdp) {
      probe.l4DstPort = rule.l4DstPort.value_or(unmatchedL4DstPort());
      probe.l4SrcPort = rule.l4SrcPort.value_or(unmatchedL4SrcPort());
    }
    if (rule.tcpFlagsBitMap.has_value()) {
      probe.tcpFlagsBitMap = rule.tcpFlagsBitMap;
    } else if (
        probe.proto == kTcp &&
        (rule.l4DstPort.has_value() || rule.l4SrcPort.has_value())) {
      // SYN makes removing the rule drop the probe on restrict-deny-tcp-syn;
      // without it restrict-permit-tcp keeps it permitted and the removal is
      // invisible.
      probe.tcpFlagsBitMap = kTcpSyn;
    }
    if (rule.dstIp.has_value()) {
      probe.dstIp = folly::IPAddress::createNetwork(*rule.dstIp).first.str();
    }
    auto match = accessPolicyMatch(probe, rule.lookupClass, {}, version);
    CHECK(match.has_value() && match->name == rule.name)
        << "probe for " << rule.name << " is matched by "
        << (match.has_value() ? std::string_view(match->name)
                              : std::string_view("no rule"));
    probes.push_back(std::move(probe));
  }
  return probes;
}

std::vector<ControlPlaneProbe> buildControlPlaneProbes() {
  std::vector<ControlPlaneProbe> probes;
  auto add = [&probes](
                 const std::string& name,
                 ControlPlanePacket packet,
                 AccessPolicyProbe policyMatch) {
    policyMatch.name = name;
    probes.push_back({name, packet, std::move(policyMatch)});
  };
  auto nonIp = [](cfg::EtherType etherType) {
    AccessPolicyProbe probe;
    probe.etherType = etherType;
    return probe;
  };
  auto udp = [](int32_t l4DstPort) {
    AccessPolicyProbe probe;
    probe.proto = kUdp;
    probe.l4DstPort = l4DstPort;
    probe.l4SrcPort = unmatchedL4SrcPort();
    return probe;
  };
  auto tcp = [](int32_t l4DstPort, int32_t l4SrcPort) {
    AccessPolicyProbe probe;
    probe.proto = kTcp;
    probe.l4DstPort = l4DstPort;
    probe.l4SrcPort = l4SrcPort;
    return probe;
  };
  AccessPolicyProbe icmpV6;
  icmpV6.proto = kIcmpV6;

  add("arp-request",
      ControlPlanePacket::ArpRequest,
      nonIp(cfg::EtherType::ARP));
  add("arp-reply", ControlPlanePacket::ArpReply, nonIp(cfg::EtherType::ARP));
  add("ndp-neighbor-solicit",
      ControlPlanePacket::NdpNeighborSolicitation,
      icmpV6);
  add("ndp-neighbor-advertise",
      ControlPlanePacket::NdpNeighborAdvertisement,
      icmpV6);
  add("ndp-router-solicit", ControlPlanePacket::NdpRouterSolicitation, icmpV6);
  add("ndp-router-advertise",
      ControlPlanePacket::NdpRouterAdvertisement,
      icmpV6);
  add("lldp", ControlPlanePacket::Lldp, nonIp(cfg::EtherType::LLDP));
  add("lldp-customer-bridge",
      ControlPlanePacket::LldpCustomerBridge,
      nonIp(cfg::EtherType::LLDP));
  add("lacp", ControlPlanePacket::Lacp, nonIp(cfg::EtherType::LACP));
  add("dhcpv4-to-server",
      ControlPlanePacket::DhcpV4ToServer,
      udp(kDhcpV4ServerPort));
  add("dhcpv4-to-client",
      ControlPlanePacket::DhcpV4ToClient,
      udp(kDhcpV4ClientPort));
  add("dhcpv6-to-server",
      ControlPlanePacket::DhcpV6ToServer,
      udp(kDhcpV6ServerPort));
  add("dhcpv6-to-client",
      ControlPlanePacket::DhcpV6ToClient,
      udp(kDhcpV6ClientPort));
  add("bgp-dst-port",
      ControlPlanePacket::BgpDstPort,
      tcp(kBgpL4Port, unmatchedL4SrcPort()));
  add("bgp-src-port",
      ControlPlanePacket::BgpSrcPort,
      tcp(unmatchedL4DstPort(), kBgpL4Port));
  add("ip2me", ControlPlanePacket::Ip2Me, udp(unmatchedL4DstPort()));
  add("ip2me-network-control",
      ControlPlanePacket::Ip2MeNetworkControl,
      udp(unmatchedL4DstPort()));
  add("link-local-mcast",
      ControlPlanePacket::LinkLocalMcast,
      udp(unmatchedL4DstPort()));
  add("link-local-mcast-network-control",
      ControlPlanePacket::LinkLocalMcastNetworkControl,
      udp(unmatchedL4DstPort()));
  add("link-local-ucast",
      ControlPlanePacket::LinkLocalUcast,
      udp(unmatchedL4DstPort()));
  add("ttl1", ControlPlanePacket::Ttl1, udp(unmatchedL4DstPort()));

  return probes;
}

} // namespace

std::string kAccessPolicyClassIdTable() {
  return "AccessPolicyClassIDTable";
}

std::string kAccessPolicyRestrictedTable() {
  return "AccessPolicyRestrictedTable";
}

std::string kAccessPolicyTableGroup() {
  return "access-policy-ingress-ACL-Table-Group";
}

std::optional<AccessPolicyShape> accessPolicyShape(
    const std::vector<const HwAsic*>& asics) {
  auto asicType = checkSameAndGetAsic(asics)->getAsicType();
  switch (asicType) {
    case cfg::AsicType::ASIC_TYPE_TOMAHAWK3:
      return AccessPolicyShape::ClassId;
    case cfg::AsicType::ASIC_TYPE_EBRO:
      return AccessPolicyShape::PortBound;
    default:
      XLOG(INFO) << "No access policy shape for asic "
                 << apache::thrift::util::enumNameSafe(asicType);
      return std::nullopt;
  }
}

const std::vector<AccessPolicyRule>& accessPolicyRules(
    AccessPolicyVersion version) {
  static const std::vector<AccessPolicyRule> v0Rules = buildAccessPolicyRules();
  static const std::vector<AccessPolicyRule> v1Rules =
      buildAccessPolicyRulesV1();
  switch (version) {
    case AccessPolicyVersion::V0:
      return v0Rules;
    case AccessPolicyVersion::V1:
      return v1Rules;
  }
  throw FbossError(
      "Unhandled access policy version ", static_cast<int>(version));
}

const std::vector<AccessPolicyProbe>& accessPolicyProbes(
    AccessPolicyVersion version) {
  static const std::vector<AccessPolicyProbe> v0Probes =
      buildAccessPolicyProbes(AccessPolicyVersion::V0);
  static const std::vector<AccessPolicyProbe> v1Probes =
      buildAccessPolicyProbes(AccessPolicyVersion::V1);
  switch (version) {
    case AccessPolicyVersion::V0:
      return v0Probes;
    case AccessPolicyVersion::V1:
      return v1Probes;
  }
  throw FbossError(
      "Unhandled access policy version ", static_cast<int>(version));
}

const std::vector<ControlPlaneProbe>& controlPlaneProbes() {
  static const std::vector<ControlPlaneProbe> probes =
      buildControlPlaneProbes();
  return probes;
}

const std::vector<std::string>& accessPolicyRepresentativeRules() {
  static const std::vector<std::string> names = [] {
    std::vector<std::string> representatives{
        "restrict-permit-icmpv6",
        "restrict-permit-dhcp-547",
        "restrict-permit-53",
        "restrict-permit-tcp-s3-11",
        "restrict-permit-s0-vip-prefix",
        "restrict-permit-8080-s1-1",
        "restrict-permit-tcp-ssh-response",
        "restrict-permit-udp-rtc-response",
        "restrict-deny-tcp-syn",
        "restrict-permit-tcp"};
    for (const auto& name : representatives) {
      CHECK(
          std::any_of(
              accessPolicyRules().begin(),
              accessPolicyRules().end(),
              [&name](const auto& rule) { return rule.name == name; }))
          << "representative " << name << " names no access policy rule";
    }
    return representatives;
  }();
  return names;
}

std::optional<AccessPolicyRule> accessPolicyMatch(
    const AccessPolicyProbe& probe,
    cfg::AclLookupClassPort lookupClass,
    const std::set<std::string>& omitRules,
    AccessPolicyVersion version) {
  for (const auto& rule : accessPolicyRules(version)) {
    if (rule.lookupClass != lookupClass || omitRules.count(rule.name)) {
      continue;
    }
    if (ruleMatchesProbe(rule, probe)) {
      return rule;
    }
  }
  return std::nullopt;
}

cfg::AclTable* findAccessPolicyAclTable(
    cfg::SwitchConfig& config,
    const std::string& name) {
  // utility::getAclTable() only searches the first table group at a stage.
  for (auto& group : *config.aclTableGroups()) {
    for (auto& table : *group.aclTables()) {
      if (*table.name() == name) {
        return &table;
      }
    }
  }
  throw FbossError("No ACL table named ", name);
}

void addAccessPolicyTables(cfg::SwitchConfig& config, AccessPolicyShape shape) {
  if (shape == AccessPolicyShape::ClassId) {
    // Joins the switch bound group the base config already created, next to
    // AclTable1, the way coop does. Creating a group here instead would put
    // the table somewhere production never puts it; addAclTable throws if the
    // caller has not made that group.
    utility::addAclTable(
        &config,
        kAccessPolicyClassIdTable(),
        kAccessPolicyClassIdTablePriority,
        accessPolicyActionTypes(),
        classIdTableQualifiers());
    return;
  }
  // A second INGRESS group, alongside the switch bound one, is the shape coop
  // programs: this one binds to ports. utility::addAclTableGroup would replace
  // the group already at the stage rather than add to it.
  cfg::AclTableGroup group;
  group.name() = kAccessPolicyTableGroup();
  group.stage() = cfg::AclStage::INGRESS;
  group.bindPoint() = cfg::AclTableGroupBindPoint::PORT;
  group.aclTables() = {makeAclTable(
      kAccessPolicyRestrictedTable(),
      kRestrictedTablePriority,
      restrictedTableQualifiers())};
  if (!config.aclTableGroups()) {
    config.aclTableGroups() = {};
  }
  config.aclTableGroups()->push_back(std::move(group));
}

void addAccessPolicyAcls(
    cfg::SwitchConfig& config,
    const std::vector<const HwAsic*>& asics,
    AccessPolicyShape shape,
    const std::set<std::string>& omitRules,
    cfg::AclActionType denyAction,
    AccessPolicyVersion version) {
  const auto& rules = accessPolicyRules(version);
  for (const auto& name : omitRules) {
    CHECK(
        std::any_of(
            rules.begin(),
            rules.end(),
            [&name](const auto& rule) { return rule.name == name; }))
        << "omitted rule " << name << " names no access policy rule";
  }
  auto counterTypes = utility::getAclCounterTypes(asics);
  for (const auto& rule : rules) {
    if (omitRules.count(rule.name)) {
      continue;
    }
    auto entry = makeAclEntry(shape, rule, denyAction);
    auto* table = findAccessPolicyAclTable(config, tableForShape(shape));
    if (!utility::aclEntrySupported(table, entry)) {
      throw FbossError(
          "ACL entry ",
          rule.name,
          " is not supported by table ",
          *table->name());
    }
    table->aclEntries()->push_back(entry);
    utility::addAclStat(&config, rule.name, rule.counterName, counterTypes);
  }
}

void removeAccessPolicy(cfg::SwitchConfig& config, AccessPolicyShape shape) {
  for (auto version : kAccessPolicyVersions) {
    for (const auto& rule : accessPolicyRules(version)) {
      utility::delAclStat(&config, rule.name, rule.counterName);
    }
  }
  auto groups = config.aclTableGroups();
  if (shape == AccessPolicyShape::ClassId) {
    if (groups) {
      for (auto& group : *groups) {
        auto& tables = *group.aclTables();
        std::erase_if(tables, [](const cfg::AclTable& table) {
          return *table.name() == kAccessPolicyClassIdTable();
        });
      }
    }
    for (auto& port : *config.ports()) {
      port.userMetaData().reset();
    }
    return;
  }
  if (groups) {
    std::erase_if(*groups, [](const cfg::AclTableGroup& group) {
      return *group.name() == kAccessPolicyTableGroup();
    });
  }
  for (auto& port : *config.ports()) {
    port.ingressAclTableName().reset();
  }
}

void bindAccessPolicyPort(
    cfg::SwitchConfig& config,
    AccessPolicyShape shape,
    PortID portId,
    cfg::AclLookupClassPort lookupClass) {
  auto port = utility::findCfgPort(config, portId);
  if (shape == AccessPolicyShape::ClassId) {
    port->userMetaData() = lookupClass;
    return;
  }
  if (lookupClass == cfg::AclLookupClassPort::CLASS_PORT_RESTRICTED) {
    port->ingressAclTableName() = kAccessPolicyRestrictedTable();
  } else if (lookupClass == cfg::AclLookupClassPort::CLASS_PORT_UNCONSTRAINED) {
    port->ingressAclTableName().reset();
  } else {
    throw FbossError(
        "Unhandled access policy lookup class ", static_cast<int>(lookupClass));
  }
}

} // namespace facebook::fboss::utility
