// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/utils/AccessPolicyAclTestUtils.h"

#include <fmt/core.h>
#include <folly/logging/xlog.h>
#include <thrift/lib/cpp/util/EnumUtils.h>

#include "fboss/agent/AsicUtils.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/packet/IPProto.h"

namespace facebook::fboss::utility {

namespace {

constexpr auto kTcp = static_cast<int16_t>(IP_PROTO::IP_PROTO_TCP);
constexpr auto kUdp = static_cast<int16_t>(IP_PROTO::IP_PROTO_UDP);
constexpr auto kIcmpV6 = static_cast<int16_t>(IP_PROTO::IP_PROTO_IPV6_ICMP);
constexpr int16_t kTcpSyn = 2;

// A SAI ACL counter label is char[32]. SaiAclTableManager::addAclCounter throws
// an uncaught FbossError on a longer name, which aborts the hw agent.
constexpr size_t kMaxAclNameLen = 31;

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

const std::vector<AccessPolicyRule>& accessPolicyRules() {
  static const std::vector<AccessPolicyRule> rules = buildAccessPolicyRules();
  return rules;
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

} // namespace facebook::fboss::utility
