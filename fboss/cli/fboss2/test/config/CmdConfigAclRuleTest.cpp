/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <gtest/gtest.h>
#include <stdexcept>
#include <string>

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/cli/fboss2/commands/config/acl/rule/AclRuleAttrs.h"
#include "fboss/cli/fboss2/commands/config/acl/rule/CmdConfigAclRule.h"
#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/test/config/CmdConfigTestBase.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"

using namespace ::testing;

namespace facebook::fboss {

// Seed mirrors a production ingress ACL: one AclTableGroup with one
// AclTable holding two AclEntries by name. Most match-fields default to
// unset so each test sees a clean baseline mutation.
static constexpr auto kSeedConfig = R"({
  "sw": {
    "aclTableGroups": [{
      "name": "ingress-ACL-Table-Group",
      "stage": 0,
      "aclTables": [{
        "name": "AclTable1",
        "priority": 0,
        "aclEntries": [
          {"name": "rule-1", "actionType": 1},
          {"name": "rule-2", "actionType": 0, "proto": 6}
        ],
        "actionTypes": [],
        "qualifiers": [],
        "udfGroups": []
      }]
    }]
  }
})";

class CmdConfigAclRuleTestFixture : public CmdConfigTestBase {
 public:
  CmdConfigAclRuleTestFixture()
      : CmdConfigTestBase(
            "fboss_acl_rule_config_test_%%%%-%%%%-%%%%-%%%%",
            kSeedConfig) {}

 protected:
  const std::string cmdPrefix_ = "config acl rule";

  cfg::AclEntry& getRule(const std::string& name) {
    auto& cfg = ConfigSession::getInstance().getAgentConfig();
    auto& tables = *(*cfg.sw()->aclTableGroups())[0].aclTables();
    auto& entries = *tables[0].aclEntries();
    for (auto& e : entries) {
      if (*e.name() == name) {
        return e;
      }
    }
    throw std::runtime_error("rule not found: " + name);
  }

  // Look up the MatchAction attached to `ruleName` via
  // dataPlaneTrafficPolicy.matchToAction. The CLI inserts entries on demand,
  // so this throws if no entry exists for the rule.
  cfg::MatchAction& getMatchAction(const std::string& ruleName) {
    auto& cfg = ConfigSession::getInstance().getAgentConfig();
    auto policy = cfg.sw()->dataPlaneTrafficPolicy();
    if (!policy) {
      throw std::runtime_error("dataPlaneTrafficPolicy not set");
    }
    for (auto& mta : *policy->matchToAction()) {
      if (*mta.matcher() == ruleName) {
        return *mta.action();
      }
    }
    throw std::runtime_error("matchToAction not found for: " + ruleName);
  }
};

// =============================================================
// AclRuleConfigArgs validation tests
// =============================================================

TEST_F(CmdConfigAclRuleTestFixture, argValidation_badArity) {
  EXPECT_THROW(AclRuleConfigArgs({}), std::invalid_argument);
  EXPECT_THROW(
      AclRuleConfigArgs({"AclTable1", "rule-1"}), std::invalid_argument);
  EXPECT_THROW(
      AclRuleConfigArgs({"AclTable1", "rule-1", "dscp"}),
      std::invalid_argument);
  EXPECT_THROW(
      AclRuleConfigArgs({"AclTable1", "rule-1", "dscp", "10", "extra", "more"}),
      std::invalid_argument);
}

TEST_F(CmdConfigAclRuleTestFixture, argValidation_unknownAttr) {
  EXPECT_THROW(
      AclRuleConfigArgs({"AclTable1", "rule-1", "bogus", "1"}),
      std::invalid_argument);
}

TEST_F(CmdConfigAclRuleTestFixture, argValidation_extraTokenOnlyForTtl) {
  // 5-token form is only valid for ttl.
  EXPECT_THROW(
      AclRuleConfigArgs({"AclTable1", "rule-1", "dscp", "10", "20"}),
      std::invalid_argument);
  // ttl with 4 or 5 tokens is fine.
  EXPECT_NO_THROW(AclRuleConfigArgs({"AclTable1", "rule-1", "ttl", "64"}));
  EXPECT_NO_THROW(
      AclRuleConfigArgs({"AclTable1", "rule-1", "ttl", "64", "255"}));
}

TEST_F(CmdConfigAclRuleTestFixture, argValidation_outOfRange) {
  EXPECT_THROW(
      AclRuleConfigArgs(
          {"AclTable1", "rule-1", "dscp", std::to_string(kDscpRange.max + 1)}),
      std::invalid_argument);
  EXPECT_THROW(
      AclRuleConfigArgs(
          {"AclTable1", "rule-1", "dscp", std::to_string(kDscpRange.min - 1)}),
      std::invalid_argument);
  EXPECT_THROW(
      AclRuleConfigArgs(
          {"AclTable1",
           "rule-1",
           "vlan",
           std::to_string(utils::kVlanIdMin - 1)}),
      std::invalid_argument);
  EXPECT_THROW(
      AclRuleConfigArgs(
          {"AclTable1",
           "rule-1",
           "vlan",
           std::to_string(utils::kVlanIdMax + 1)}),
      std::invalid_argument);
}

TEST_F(CmdConfigAclRuleTestFixture, argValidation_badIp) {
  EXPECT_THROW(
      AclRuleConfigArgs({"AclTable1", "rule-1", "source-ip", "not-an-ip"}),
      std::invalid_argument);
  EXPECT_NO_THROW(
      AclRuleConfigArgs({"AclTable1", "rule-1", "source-ip", "10.0.0.0/24"}));
  EXPECT_NO_THROW(AclRuleConfigArgs(
      {"AclTable1", "rule-1", "destination-ip", "fe80::/64"}));
}

TEST_F(CmdConfigAclRuleTestFixture, argValidation_badMac) {
  EXPECT_THROW(
      AclRuleConfigArgs(
          {"AclTable1", "rule-1", "destination-mac", "not-a-mac"}),
      std::invalid_argument);
  EXPECT_NO_THROW(AclRuleConfigArgs(
      {"AclTable1", "rule-1", "destination-mac", "aa:bb:cc:dd:ee:ff"}));
}

TEST_F(CmdConfigAclRuleTestFixture, argValidation_protocolKeyword) {
  AclRuleConfigArgs a({"AclTable1", "rule-1", "protocol", "tcp"});
  EXPECT_EQ(a.getAttribute(), "protocol");
  AclRuleConfigArgs b({"AclTable1", "rule-1", "protocol", "icmpv6"});
  AclRuleConfigArgs c({"AclTable1", "rule-1", "protocol", "58"});
  EXPECT_THROW(
      AclRuleConfigArgs({"AclTable1", "rule-1", "protocol", "bogus"}),
      std::invalid_argument);
}

TEST_F(CmdConfigAclRuleTestFixture, argValidation_ipFragmentEnum) {
  EXPECT_NO_THROW(AclRuleConfigArgs(
      {"AclTable1", "rule-1", "ip-fragment", "any-fragment"}));
  EXPECT_THROW(
      AclRuleConfigArgs({"AclTable1", "rule-1", "ip-fragment", "bogus"}),
      std::invalid_argument);
}

TEST_F(CmdConfigAclRuleTestFixture, argValidation_etherTypeNumericOrName) {
  EXPECT_NO_THROW(
      AclRuleConfigArgs({"AclTable1", "rule-1", "ethertype", "ipv4"}));
  EXPECT_NO_THROW(
      AclRuleConfigArgs({"AclTable1", "rule-1", "ethertype", "0x0800"}));
  EXPECT_NO_THROW(
      AclRuleConfigArgs({"AclTable1", "rule-1", "ethertype", "2048"}));
  EXPECT_THROW(
      AclRuleConfigArgs(
          {"AclTable1",
           "rule-1",
           "ethertype",
           std::to_string(kU16Range.max + 1)}),
      std::invalid_argument);
}

// =============================================================
// queryClient() tests — one per attribute
// =============================================================

TEST_F(CmdConfigAclRuleTestFixture, ruleAutoCreated) {
  // Missing rule + attr/value: rule is created and the attribute applied
  // in the same command (auto-upsert).
  setupTestableConfigSession(cmdPrefix_, "AclTable1 brand-new dscp 10");
  CmdConfigAclRule cmd;
  HostInfo host("testhost");
  AclRuleConfigArgs args({"AclTable1", "brand-new", "dscp", "10"});
  std::string result;
  EXPECT_NO_THROW(result = cmd.queryClient(host, args));
  // Auto-create path reports "Created and set" and echoes rule/attr/value.
  EXPECT_THAT(result, HasSubstr("Created and set"));
  EXPECT_THAT(result, HasSubstr("brand-new"));
  EXPECT_THAT(result, HasSubstr("dscp"));
  EXPECT_THAT(result, HasSubstr("10"));
  EXPECT_EQ(*getRule("brand-new").dscp(), 10);
}

TEST_F(CmdConfigAclRuleTestFixture, tableNotFound) {
  setupTestableConfigSession(cmdPrefix_, "Bogus rule-1 dscp 10");
  CmdConfigAclRule cmd;
  HostInfo host("testhost");
  AclRuleConfigArgs args({"Bogus", "rule-1", "dscp", "10"});
  EXPECT_THROW(cmd.queryClient(host, args), std::runtime_error);
}

TEST_F(CmdConfigAclRuleTestFixture, setSourceIp) {
  setupTestableConfigSession(
      cmdPrefix_, "AclTable1 rule-1 source-ip 10.0.0.0/24");
  CmdConfigAclRule cmd;
  HostInfo host("testhost");
  AclRuleConfigArgs args({"AclTable1", "rule-1", "source-ip", "10.0.0.0/24"});
  EXPECT_NO_THROW(cmd.queryClient(host, args));
  EXPECT_EQ(*getRule("rule-1").srcIp(), "10.0.0.0/24");
}

TEST_F(CmdConfigAclRuleTestFixture, setDestinationIp) {
  setupTestableConfigSession(
      cmdPrefix_, "AclTable1 rule-1 destination-ip fe80::/64");
  CmdConfigAclRule cmd;
  HostInfo host("testhost");
  AclRuleConfigArgs args(
      {"AclTable1", "rule-1", "destination-ip", "fe80::/64"});
  cmd.queryClient(host, args);
  EXPECT_EQ(*getRule("rule-1").dstIp(), "fe80::/64");
}

TEST_F(CmdConfigAclRuleTestFixture, setProtocol) {
  setupTestableConfigSession(cmdPrefix_, "AclTable1 rule-1 protocol tcp");
  CmdConfigAclRule cmd;
  HostInfo host("testhost");
  AclRuleConfigArgs args({"AclTable1", "rule-1", "protocol", "tcp"});
  cmd.queryClient(host, args);
  EXPECT_EQ(*getRule("rule-1").proto(), 6);
}

TEST_F(CmdConfigAclRuleTestFixture, setSourcePort) {
  setupTestableConfigSession(cmdPrefix_, "AclTable1 rule-1 source-port 443");
  CmdConfigAclRule cmd;
  HostInfo host("testhost");
  AclRuleConfigArgs args({"AclTable1", "rule-1", "source-port", "443"});
  cmd.queryClient(host, args);
  EXPECT_EQ(*getRule("rule-1").l4SrcPort(), 443);
}

TEST_F(CmdConfigAclRuleTestFixture, setDestinationPort) {
  setupTestableConfigSession(
      cmdPrefix_, "AclTable1 rule-1 destination-port 443");
  CmdConfigAclRule cmd;
  HostInfo host("testhost");
  AclRuleConfigArgs args({"AclTable1", "rule-1", "destination-port", "443"});
  cmd.queryClient(host, args);
  EXPECT_EQ(*getRule("rule-1").l4DstPort(), 443);
}

TEST_F(CmdConfigAclRuleTestFixture, setDscp) {
  setupTestableConfigSession(cmdPrefix_, "AclTable1 rule-1 dscp 46");
  CmdConfigAclRule cmd;
  HostInfo host("testhost");
  AclRuleConfigArgs args({"AclTable1", "rule-1", "dscp", "46"});
  // rule-1 already exists, so this is an update: "Set", not "Created and set".
  auto result = cmd.queryClient(host, args);
  EXPECT_THAT(result, HasSubstr("Set"));
  EXPECT_THAT(result, Not(HasSubstr("Created")));
  EXPECT_THAT(result, HasSubstr("46"));
  EXPECT_EQ(*getRule("rule-1").dscp(), 46);
}

TEST_F(CmdConfigAclRuleTestFixture, setTcpFlags) {
  setupTestableConfigSession(cmdPrefix_, "AclTable1 rule-1 tcp-flags 0x12");
  CmdConfigAclRule cmd;
  HostInfo host("testhost");
  AclRuleConfigArgs args({"AclTable1", "rule-1", "tcp-flags", "0x12"});
  cmd.queryClient(host, args);
  EXPECT_EQ(*getRule("rule-1").tcpFlagsBitMap(), 0x12);
}

TEST_F(CmdConfigAclRuleTestFixture, setIcmpType) {
  setupTestableConfigSession(cmdPrefix_, "AclTable1 rule-1 icmp-type 8");
  CmdConfigAclRule cmd;
  HostInfo host("testhost");
  AclRuleConfigArgs args({"AclTable1", "rule-1", "icmp-type", "8"});
  cmd.queryClient(host, args);
  EXPECT_EQ(*getRule("rule-1").icmpType(), 8);
}

TEST_F(CmdConfigAclRuleTestFixture, setIcmpCode) {
  // Use a non-zero code: 0 coincides with the thrift default for an unset
  // optional i16, so it would not prove the CLI actually wrote the field.
  setupTestableConfigSession(cmdPrefix_, "AclTable1 rule-1 icmp-code 3");
  CmdConfigAclRule cmd;
  HostInfo host("testhost");
  AclRuleConfigArgs args({"AclTable1", "rule-1", "icmp-code", "3"});
  cmd.queryClient(host, args);
  EXPECT_EQ(*getRule("rule-1").icmpCode(), 3);
}

TEST_F(CmdConfigAclRuleTestFixture, setIpFragment) {
  setupTestableConfigSession(
      cmdPrefix_, "AclTable1 rule-1 ip-fragment any-fragment");
  CmdConfigAclRule cmd;
  HostInfo host("testhost");
  AclRuleConfigArgs args(
      {"AclTable1", "rule-1", "ip-fragment", "any-fragment"});
  cmd.queryClient(host, args);
  EXPECT_EQ(*getRule("rule-1").ipFrag(), cfg::IpFragMatch::MATCH_ANY_FRAGMENT);
}

TEST_F(CmdConfigAclRuleTestFixture, setTtlDefaultMask) {
  setupTestableConfigSession(cmdPrefix_, "AclTable1 rule-1 ttl 64");
  CmdConfigAclRule cmd;
  HostInfo host("testhost");
  AclRuleConfigArgs args({"AclTable1", "rule-1", "ttl", "64"});
  cmd.queryClient(host, args);
  EXPECT_EQ(*getRule("rule-1").ttl()->value(), 64);
  EXPECT_EQ(*getRule("rule-1").ttl()->mask(), kTtlMaskDefault);
}

TEST_F(CmdConfigAclRuleTestFixture, setTtlExplicitMask) {
  setupTestableConfigSession(cmdPrefix_, "AclTable1 rule-1 ttl 64 240");
  CmdConfigAclRule cmd;
  HostInfo host("testhost");
  AclRuleConfigArgs args({"AclTable1", "rule-1", "ttl", "64", "240"});
  cmd.queryClient(host, args);
  EXPECT_EQ(*getRule("rule-1").ttl()->value(), 64);
  EXPECT_EQ(*getRule("rule-1").ttl()->mask(), 240);
}

TEST_F(CmdConfigAclRuleTestFixture, setDestinationMac) {
  setupTestableConfigSession(
      cmdPrefix_, "AclTable1 rule-1 destination-mac aa:bb:cc:dd:ee:ff");
  CmdConfigAclRule cmd;
  HostInfo host("testhost");
  AclRuleConfigArgs args(
      {"AclTable1", "rule-1", "destination-mac", "aa:bb:cc:dd:ee:ff"});
  cmd.queryClient(host, args);
  EXPECT_EQ(*getRule("rule-1").dstMac(), "aa:bb:cc:dd:ee:ff");
}

TEST_F(CmdConfigAclRuleTestFixture, setEtherType) {
  setupTestableConfigSession(cmdPrefix_, "AclTable1 rule-1 ethertype ipv4");
  CmdConfigAclRule cmd;
  HostInfo host("testhost");
  AclRuleConfigArgs args({"AclTable1", "rule-1", "ethertype", "ipv4"});
  cmd.queryClient(host, args);
  EXPECT_EQ(*getRule("rule-1").etherType(), cfg::EtherType::IPv4);
}

TEST_F(CmdConfigAclRuleTestFixture, setVlan) {
  setupTestableConfigSession(cmdPrefix_, "AclTable1 rule-1 vlan 100");
  CmdConfigAclRule cmd;
  HostInfo host("testhost");
  AclRuleConfigArgs args({"AclTable1", "rule-1", "vlan", "100"});
  cmd.queryClient(host, args);
  EXPECT_EQ(*getRule("rule-1").vlanID(), 100);
}

TEST_F(CmdConfigAclRuleTestFixture, setIpType) {
  setupTestableConfigSession(cmdPrefix_, "AclTable1 rule-1 ip-type ipv6");
  CmdConfigAclRule cmd;
  HostInfo host("testhost");
  AclRuleConfigArgs args({"AclTable1", "rule-1", "ip-type", "ipv6"});
  cmd.queryClient(host, args);
  EXPECT_EQ(*getRule("rule-1").ipType(), cfg::IpType::IP6);
}

// =============================================================
// action sub-attribute tests
// =============================================================

TEST_F(CmdConfigAclRuleTestFixture, argValidation_unknownActionSubattr) {
  EXPECT_THROW(
      AclRuleConfigArgs({"AclTable1", "rule-1", "action", "bogus"}),
      std::invalid_argument);
}

TEST_F(CmdConfigAclRuleTestFixture, argValidation_actionPermitNoExtra) {
  EXPECT_NO_THROW(
      AclRuleConfigArgs({"AclTable1", "rule-1", "action", "permit"}));
  EXPECT_THROW(
      AclRuleConfigArgs({"AclTable1", "rule-1", "action", "permit", "x"}),
      std::invalid_argument);
}

TEST_F(CmdConfigAclRuleTestFixture, argValidation_actionRequiresValue) {
  EXPECT_THROW(
      AclRuleConfigArgs({"AclTable1", "rule-1", "action", "set-dscp"}),
      std::invalid_argument);
}

TEST_F(CmdConfigAclRuleTestFixture, argValidation_actionRangeChecks) {
  EXPECT_THROW(
      AclRuleConfigArgs(
          {"AclTable1",
           "rule-1",
           "action",
           "set-dscp",
           std::to_string(kSetDscpRange.max + 1)}),
      std::invalid_argument);
  EXPECT_THROW(
      AclRuleConfigArgs(
          {"AclTable1",
           "rule-1",
           "action",
           "set-tc",
           std::to_string(kTrafficClassRange.max + 1)}),
      std::invalid_argument);
  EXPECT_THROW(
      AclRuleConfigArgs(
          {"AclTable1",
           "rule-1",
           "action",
           "send-to-queue",
           std::to_string(kSendToQueueRange.min - 1)}),
      std::invalid_argument);
}

TEST_F(CmdConfigAclRuleTestFixture, argValidation_actionRedirectShape) {
  // Missing the `nexthop` keyword.
  EXPECT_THROW(
      AclRuleConfigArgs(
          {"AclTable1", "rule-1", "action", "redirect", "10.0.0.1"}),
      std::invalid_argument);
  // Wrong keyword.
  EXPECT_THROW(
      AclRuleConfigArgs(
          {"AclTable1", "rule-1", "action", "redirect", "nope", "10.0.0.1"}),
      std::invalid_argument);
  // Bad IP.
  EXPECT_THROW(
      AclRuleConfigArgs(
          {"AclTable1", "rule-1", "action", "redirect", "nexthop", "garbage"}),
      std::invalid_argument);
  // Valid IPv4 / IPv6.
  EXPECT_NO_THROW(AclRuleConfigArgs(
      {"AclTable1", "rule-1", "action", "redirect", "nexthop", "10.0.0.1"}));
  EXPECT_NO_THROW(AclRuleConfigArgs(
      {"AclTable1", "rule-1", "action", "redirect", "nexthop", "fe80::1"}));
}

TEST_F(CmdConfigAclRuleTestFixture, setActionPermit) {
  setupTestableConfigSession(cmdPrefix_, "AclTable1 rule-2 action permit");
  CmdConfigAclRule cmd;
  HostInfo host("testhost");
  AclRuleConfigArgs args({"AclTable1", "rule-2", "action", "permit"});
  cmd.queryClient(host, args);
  EXPECT_EQ(*getRule("rule-2").actionType(), cfg::AclActionType::PERMIT);
}

TEST_F(CmdConfigAclRuleTestFixture, setActionDeny) {
  setupTestableConfigSession(cmdPrefix_, "AclTable1 rule-1 action deny");
  CmdConfigAclRule cmd;
  HostInfo host("testhost");
  AclRuleConfigArgs args({"AclTable1", "rule-1", "action", "deny"});
  cmd.queryClient(host, args);
  EXPECT_EQ(*getRule("rule-1").actionType(), cfg::AclActionType::DENY);
}

TEST_F(CmdConfigAclRuleTestFixture, setActionSendToQueue) {
  setupTestableConfigSession(
      cmdPrefix_, "AclTable1 rule-1 action send-to-queue 7");
  CmdConfigAclRule cmd;
  HostInfo host("testhost");
  AclRuleConfigArgs args(
      {"AclTable1", "rule-1", "action", "send-to-queue", "7"});
  // The success message echoes the whole value tail, so the queue id (7)
  // must appear — not just the "send-to-queue" sub-attribute.
  auto result = cmd.queryClient(host, args);
  EXPECT_THAT(result, HasSubstr("send-to-queue"));
  EXPECT_THAT(result, HasSubstr("7"));
  EXPECT_EQ(*getMatchAction("rule-1").sendToQueue()->queueId(), 7);
}

TEST_F(CmdConfigAclRuleTestFixture, setActionSetDscp) {
  setupTestableConfigSession(cmdPrefix_, "AclTable1 rule-1 action set-dscp 46");
  CmdConfigAclRule cmd;
  HostInfo host("testhost");
  AclRuleConfigArgs args({"AclTable1", "rule-1", "action", "set-dscp", "46"});
  cmd.queryClient(host, args);
  EXPECT_EQ(*getMatchAction("rule-1").setDscp()->dscpValue(), 46);
}

TEST_F(CmdConfigAclRuleTestFixture, setActionSetTc) {
  setupTestableConfigSession(cmdPrefix_, "AclTable1 rule-1 action set-tc 3");
  CmdConfigAclRule cmd;
  HostInfo host("testhost");
  AclRuleConfigArgs args({"AclTable1", "rule-1", "action", "set-tc", "3"});
  cmd.queryClient(host, args);
  EXPECT_EQ(*getMatchAction("rule-1").setTc()->tcValue(), 3);
}

TEST_F(CmdConfigAclRuleTestFixture, setActionMirrorIngress) {
  setupTestableConfigSession(
      cmdPrefix_, "AclTable1 rule-1 action mirror-ingress mirror0");
  CmdConfigAclRule cmd;
  HostInfo host("testhost");
  AclRuleConfigArgs args(
      {"AclTable1", "rule-1", "action", "mirror-ingress", "mirror0"});
  cmd.queryClient(host, args);
  EXPECT_EQ(*getMatchAction("rule-1").ingressMirror(), "mirror0");
}

TEST_F(CmdConfigAclRuleTestFixture, setActionMirrorEgress) {
  setupTestableConfigSession(
      cmdPrefix_, "AclTable1 rule-1 action mirror-egress mirror1");
  CmdConfigAclRule cmd;
  HostInfo host("testhost");
  AclRuleConfigArgs args(
      {"AclTable1", "rule-1", "action", "mirror-egress", "mirror1"});
  cmd.queryClient(host, args);
  EXPECT_EQ(*getMatchAction("rule-1").egressMirror(), "mirror1");
}

TEST_F(CmdConfigAclRuleTestFixture, setActionCounter) {
  setupTestableConfigSession(
      cmdPrefix_, "AclTable1 rule-1 action counter my-counter");
  CmdConfigAclRule cmd;
  HostInfo host("testhost");
  AclRuleConfigArgs args(
      {"AclTable1", "rule-1", "action", "counter", "my-counter"});
  cmd.queryClient(host, args);
  EXPECT_EQ(*getMatchAction("rule-1").counter(), "my-counter");
}

TEST_F(CmdConfigAclRuleTestFixture, setActionTrapToCpu) {
  setupTestableConfigSession(cmdPrefix_, "AclTable1 rule-1 action trap-to-cpu");
  CmdConfigAclRule cmd;
  HostInfo host("testhost");
  AclRuleConfigArgs args({"AclTable1", "rule-1", "action", "trap-to-cpu"});
  cmd.queryClient(host, args);
  EXPECT_EQ(*getMatchAction("rule-1").toCpuAction(), cfg::ToCpuAction::TRAP);
}

TEST_F(CmdConfigAclRuleTestFixture, setActionCopyToCpu) {
  setupTestableConfigSession(cmdPrefix_, "AclTable1 rule-1 action copy-to-cpu");
  CmdConfigAclRule cmd;
  HostInfo host("testhost");
  AclRuleConfigArgs args({"AclTable1", "rule-1", "action", "copy-to-cpu"});
  cmd.queryClient(host, args);
  EXPECT_EQ(*getMatchAction("rule-1").toCpuAction(), cfg::ToCpuAction::COPY);
}

TEST_F(CmdConfigAclRuleTestFixture, setActionRedirectNexthop) {
  setupTestableConfigSession(
      cmdPrefix_, "AclTable1 rule-1 action redirect nexthop 10.10.10.1");
  CmdConfigAclRule cmd;
  HostInfo host("testhost");
  AclRuleConfigArgs args(
      {"AclTable1", "rule-1", "action", "redirect", "nexthop", "10.10.10.1"});
  cmd.queryClient(host, args);
  auto& ma = getMatchAction("rule-1");
  ASSERT_TRUE(ma.redirectToNextHop().has_value());
  const auto& nhs = *ma.redirectToNextHop()->redirectNextHops();
  ASSERT_EQ(nhs.size(), 1u);
  EXPECT_EQ(*nhs[0].ip(), "10.10.10.1");
}

TEST_F(CmdConfigAclRuleTestFixture, setPacketLookupResult) {
  setupTestableConfigSession(
      cmdPrefix_, "AclTable1 rule-1 packet-lookup-result mpls-no-match");
  CmdConfigAclRule cmd;
  HostInfo host("testhost");
  AclRuleConfigArgs args(
      {"AclTable1", "rule-1", "packet-lookup-result", "mpls-no-match"});
  cmd.queryClient(host, args);
  EXPECT_EQ(
      *getRule("rule-1").packetLookupResult(),
      cfg::PacketLookupResultType::PACKET_LOOKUP_RESULT_MPLS_NO_MATCH);
}

// Overwriting an attribute already set on an existing rule replaces the value
// (rule-2 ships with proto=6/tcp in the seed; set it to udp/17).
TEST_F(CmdConfigAclRuleTestFixture, overwriteExistingAttr) {
  setupTestableConfigSession(cmdPrefix_, "AclTable1 rule-2 protocol udp");
  ASSERT_EQ(*getRule("rule-2").proto(), 6);
  CmdConfigAclRule cmd;
  HostInfo host("testhost");
  AclRuleConfigArgs args({"AclTable1", "rule-2", "protocol", "udp"});
  auto result = cmd.queryClient(host, args);
  EXPECT_THAT(result, HasSubstr("Set"));
  EXPECT_EQ(*getRule("rule-2").proto(), 17);
}

// Setting the same value twice is idempotent and stays an update ("Set").
TEST_F(CmdConfigAclRuleTestFixture, idempotentSameValue) {
  setupTestableConfigSession(cmdPrefix_, "AclTable1 rule-1 dscp 46");
  CmdConfigAclRule cmd;
  HostInfo host("testhost");
  AclRuleConfigArgs args({"AclTable1", "rule-1", "dscp", "46"});
  cmd.queryClient(host, args);
  EXPECT_EQ(*getRule("rule-1").dscp(), 46);
  // Second identical apply: still succeeds, value unchanged.
  AclRuleConfigArgs again({"AclTable1", "rule-1", "dscp", "46"});
  auto result = cmd.queryClient(host, again);
  EXPECT_THAT(result, HasSubstr("Set"));
  EXPECT_EQ(*getRule("rule-1").dscp(), 46);
}

// =============================================================
// findAclTable() helper — shared by config + delete acl rule
// =============================================================

TEST(AclRuleFindTableTest, throwsWhenNoAclTableGroups) {
  cfg::SwitchConfig sw;
  // No aclTableGroups at all (the deprecated field-45 form is unsupported).
  EXPECT_THROW(findAclTable(sw, "AclTable1"), std::runtime_error);
}

TEST(AclRuleFindTableTest, throwsWhenTableMissing) {
  cfg::SwitchConfig sw;
  cfg::AclTableGroup group;
  group.name() = "grp";
  cfg::AclTable table;
  table.name() = "AclTable1";
  group.aclTables() = {table};
  sw.aclTableGroups() = {group};
  EXPECT_THROW(findAclTable(sw, "does-not-exist"), std::runtime_error);
}

TEST(AclRuleFindTableTest, findsTableAndGroup) {
  cfg::SwitchConfig sw;
  cfg::AclTableGroup group;
  group.name() = "grp";
  cfg::AclTable table;
  table.name() = "AclTable1";
  group.aclTables() = {table};
  sw.aclTableGroups() = {group};
  auto [found, groupName] = findAclTable(sw, "AclTable1");
  ASSERT_NE(found, nullptr);
  EXPECT_EQ(*found->name(), "AclTable1");
  EXPECT_EQ(groupName, "grp");
}

} // namespace facebook::fboss
