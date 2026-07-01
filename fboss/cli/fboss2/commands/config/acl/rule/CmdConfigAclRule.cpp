/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/acl/rule/CmdConfigAclRule.h"

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/packet/IPProto.h"
#include "fboss/cli/fboss2/CmdHandler.cpp"
#include "fboss/cli/fboss2/commands/config/acl/rule/AclRuleAttrs.h"

#include <fmt/format.h>
#include <folly/Conv.h>
#include <folly/IPAddress.h>
#include <folly/MacAddress.h>
#include <folly/Range.h>
#include <folly/String.h>
#include <algorithm>
#include <cstdint>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include "fboss/cli/fboss2/gen-cpp2/cli_metadata_types.h"
#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"

namespace facebook::fboss {

namespace {

// Parse an integer that may be hex (0x...) or decimal. Used for tcp-flags,
// ethertype, and packet-lookup-result. Range-checks against [min, max].
int32_t parseIntInRange(
    std::string_view attr,
    const std::string& s,
    int64_t min,
    int64_t max) {
  int64_t parsed = 0;
  try {
    if (s.size() > 2 && (s[0] == '0') && (s[1] == 'x' || s[1] == 'X')) {
      parsed = std::stoll(s, nullptr, 16);
    } else {
      parsed = folly::to<int64_t>(s);
    }
  } catch (const std::exception&) {
    throw std::invalid_argument(
        fmt::format("Value for '{}' must be an integer, got '{}'", attr, s));
  }
  if (parsed < min || parsed > max) {
    throw std::invalid_argument(
        fmt::format(
            "Value for '{}' out of range [{}, {}], got {}",
            attr,
            min,
            max,
            parsed));
  }
  return static_cast<int32_t>(parsed);
}

// Range-checked overload taking a named AclRuleRange.
int32_t parseIntInRange(
    std::string_view attr,
    const std::string& s,
    AclRuleRange range) {
  return parseIntInRange(attr, s, range.min, range.max);
}

// protocol: tcp/udp/icmp/icmpv6 keyword or numeric 0-255.
int32_t parseProtocol(const std::string& s) {
  if (s == "tcp") {
    return static_cast<int32_t>(IP_PROTO::IP_PROTO_TCP);
  }
  if (s == "udp") {
    return static_cast<int32_t>(IP_PROTO::IP_PROTO_UDP);
  }
  if (s == "icmp") {
    return static_cast<int32_t>(IP_PROTO::IP_PROTO_ICMP);
  }
  if (s == "icmpv6") {
    return static_cast<int32_t>(IP_PROTO::IP_PROTO_IPV6_ICMP);
  }
  return parseIntInRange(kAclRuleAttrProtocol, s, kProtocolRange);
}

cfg::IpFragMatch parseIpFragment(const std::string& s) {
  if (s == "not-fragmented") {
    return cfg::IpFragMatch::MATCH_NOT_FRAGMENTED;
  }
  if (s == "first-fragment") {
    return cfg::IpFragMatch::MATCH_FIRST_FRAGMENT;
  }
  if (s == "non-first-fragment") {
    return cfg::IpFragMatch::MATCH_NOT_FIRST_FRAGMENT;
  }
  if (s == "any-fragment") {
    return cfg::IpFragMatch::MATCH_ANY_FRAGMENT;
  }
  throw std::invalid_argument(
      fmt::format(
          "Value for 'ip-fragment' must be one of: not-fragmented, "
          "first-fragment, non-first-fragment, any-fragment; got '{}'",
          s));
}

cfg::IpType parseIpType(const std::string& s) {
  if (s == "ipv4") {
    return cfg::IpType::IP4;
  }
  if (s == "ipv6") {
    return cfg::IpType::IP6;
  }
  if (s == "any") {
    return cfg::IpType::ANY;
  }
  throw std::invalid_argument(
      fmt::format(
          "Value for 'ip-type' must be one of: ipv4, ipv6, any; got '{}'", s));
}

cfg::EtherType parseEtherType(const std::string& s) {
  if (s == "ipv4") {
    return cfg::EtherType::IPv4;
  }
  if (s == "ipv6") {
    return cfg::EtherType::IPv6;
  }
  if (s == "arp") {
    return cfg::EtherType::ARP;
  }
  if (s == "lldp") {
    return cfg::EtherType::LLDP;
  }
  if (s == "lacp") {
    return cfg::EtherType::LACP;
  }
  if (s == "macsec") {
    return cfg::EtherType::MACSEC;
  }
  if (s == "eapol") {
    return cfg::EtherType::EAPOL;
  }
  if (s == "any") {
    return cfg::EtherType::ANY;
  }
  // Numeric form (decimal or 0x-hex). EtherType is 16 bits.
  auto numeric = parseIntInRange(kAclRuleAttrEtherType, s, kU16Range);
  return static_cast<cfg::EtherType>(numeric);
}

cfg::PacketLookupResultType parsePacketLookupResult(const std::string& s) {
  if (s == "mpls-no-match") {
    return cfg::PacketLookupResultType::PACKET_LOOKUP_RESULT_MPLS_NO_MATCH;
  }
  // Numeric form. PacketLookupResultType currently has only one defined
  // value (= 1), but we accept any positive int and let the agent reject
  // unknown codes.
  auto numeric = parseIntInRange(kAclRuleAttrPacketLookupResult, s, kU16Range);
  return static_cast<cfg::PacketLookupResultType>(numeric);
}

} // namespace

namespace {

// A parsed mutation captured at parse time and replayed later by applyTo() /
// applyActionTo(). Exactly one function is set: match fields and action
// permit|deny target the AclEntry; every other action targets a MatchAction.
struct AclRuleMutation {
  std::function<void(cfg::AclEntry&)> entryFn;
  std::function<void(cfg::MatchAction&)> actionFn;
};

AclRuleMutation onEntry(std::function<void(cfg::AclEntry&)> fn) {
  return {std::move(fn), nullptr};
}
AclRuleMutation onAction(std::function<void(cfg::MatchAction&)> fn) {
  return {nullptr, std::move(fn)};
}

// The value tokens following the fixed prefix (<table> <rule> <attr>, or
// ... <sub> for actions).
using Values = folly::Range<const std::string*>;

// Value-token arities used by the grammar rows below.
constexpr std::size_t kNoValue = 0;
constexpr std::size_t kOneValue = 1;
constexpr std::size_t kTwoValues = 2;

// Positions within a row's value span.
constexpr std::size_t kValue0 =
    0; // single value / ttl value / redirect keyword
constexpr std::size_t kValue1 = 1; // ttl mask / redirect ip

// Validate `value` parses as a CIDR network (bare IP → /32 or /128).
void validateCidrNetwork(std::string_view attr, const std::string& value) {
  try {
    folly::IPAddress::createNetwork(value);
  } catch (const std::exception& e) {
    throw std::invalid_argument(
        fmt::format(
            "Value for '{}' must be a CIDR network (A.B.C.D/N or "
            "A:B::C/N), got '{}': {}",
            attr,
            value,
            e.what()));
  }
}

std::string requireName(std::string_view key, const std::string& name) {
  if (name.empty()) {
    throw std::invalid_argument(
        fmt::format("Action '{}' requires a non-empty name", key));
  }
  return name;
}

// One row of the grammar: a keyword, its value-token arity [minVals, maxVals],
// a form string for --help/errors ("" = no value), and a builder turning the
// value tokens into a mutation. build is a non-capturing function pointer, so
// the table is plain data; only the returned mutation captures parsed values.
struct AclRuleRow {
  std::string_view key;
  std::size_t minVals;
  std::size_t maxVals;
  std::string_view form;
  AclRuleMutation (*build)(std::string_view key, Values vals);
};

const std::vector<AclRuleRow>& matchFieldRows() {
  static const std::vector<AclRuleRow> kRows = {
      {kAclRuleAttrSourceIp,
       kOneValue,
       kOneValue,
       "<value>",
       [](std::string_view key, Values v) {
         validateCidrNetwork(key, v[kValue0]);
         std::string ip = v[kValue0];
         return onEntry([ip](cfg::AclEntry& r) { r.srcIp() = ip; });
       }},
      {kAclRuleAttrDestinationIp,
       kOneValue,
       kOneValue,
       "<value>",
       [](std::string_view key, Values v) {
         validateCidrNetwork(key, v[kValue0]);
         std::string ip = v[kValue0];
         return onEntry([ip](cfg::AclEntry& r) { r.dstIp() = ip; });
       }},
      {kAclRuleAttrProtocol,
       kOneValue,
       kOneValue,
       "<value>",
       [](std::string_view, Values v) {
         auto p = static_cast<int16_t>(parseProtocol(v[kValue0]));
         return onEntry([p](cfg::AclEntry& r) { r.proto() = p; });
       }},
      {kAclRuleAttrSourcePort,
       kOneValue,
       kOneValue,
       "<value>",
       [](std::string_view key, Values v) {
         auto p = parseIntInRange(key, v[kValue0], kL4PortRange);
         return onEntry([p](cfg::AclEntry& r) { r.l4SrcPort() = p; });
       }},
      {kAclRuleAttrDestinationPort,
       kOneValue,
       kOneValue,
       "<value>",
       [](std::string_view key, Values v) {
         auto p = parseIntInRange(key, v[kValue0], kL4PortRange);
         return onEntry([p](cfg::AclEntry& r) { r.l4DstPort() = p; });
       }},
      {kAclRuleAttrDscp,
       kOneValue,
       kOneValue,
       "<value>",
       [](std::string_view key, Values v) {
         auto d =
             static_cast<int8_t>(parseIntInRange(key, v[kValue0], kDscpRange));
         return onEntry([d](cfg::AclEntry& r) { r.dscp() = d; });
       }},
      {kAclRuleAttrTcpFlags,
       kOneValue,
       kOneValue,
       "<value>",
       [](std::string_view key, Values v) {
         auto f = static_cast<int16_t>(
             parseIntInRange(key, v[kValue0], kTcpFlagsRange));
         return onEntry([f](cfg::AclEntry& r) { r.tcpFlagsBitMap() = f; });
       }},
      {kAclRuleAttrIcmpType,
       kOneValue,
       kOneValue,
       "<value>",
       [](std::string_view key, Values v) {
         auto t = static_cast<int16_t>(
             parseIntInRange(key, v[kValue0], kIcmpByteRange));
         return onEntry([t](cfg::AclEntry& r) { r.icmpType() = t; });
       }},
      {kAclRuleAttrIcmpCode,
       kOneValue,
       kOneValue,
       "<value>",
       [](std::string_view key, Values v) {
         auto c = static_cast<int16_t>(
             parseIntInRange(key, v[kValue0], kIcmpByteRange));
         return onEntry([c](cfg::AclEntry& r) { r.icmpCode() = c; });
       }},
      {kAclRuleAttrIpFragment,
       kOneValue,
       kOneValue,
       "<value>",
       [](std::string_view, Values v) {
         auto frag = parseIpFragment(v[kValue0]);
         return onEntry([frag](cfg::AclEntry& r) { r.ipFrag() = frag; });
       }},
      {kAclRuleAttrTtl,
       kOneValue,
       kTwoValues,
       "<value> [<mask>]",
       [](std::string_view key, Values v) {
         auto val =
             static_cast<int16_t>(parseIntInRange(key, v[kValue0], kTtlRange));
         int16_t mask = v.size() == kTwoValues
             ? static_cast<int16_t>(
                   parseIntInRange("ttl-mask", v[kValue1], kTtlRange))
             : kTtlMaskDefault;
         return onEntry([val, mask](cfg::AclEntry& r) {
           cfg::Ttl ttl;
           ttl.value() = val;
           ttl.mask() = mask;
           r.ttl() = ttl;
         });
       }},
      {kAclRuleAttrDestinationMac,
       kOneValue,
       kOneValue,
       "<value>",
       [](std::string_view, Values v) {
         std::string mac = v[kValue0];
         try {
           (void)folly::MacAddress{mac};
         } catch (const std::exception& e) {
           throw std::invalid_argument(
               fmt::format(
                   "Value for 'destination-mac' must be a MAC address, "
                   "got '{}': {}",
                   mac,
                   e.what()));
         }
         return onEntry([mac](cfg::AclEntry& r) { r.dstMac() = mac; });
       }},
      {kAclRuleAttrEtherType,
       kOneValue,
       kOneValue,
       "<value>",
       [](std::string_view, Values v) {
         auto et = parseEtherType(v[kValue0]);
         return onEntry([et](cfg::AclEntry& r) { r.etherType() = et; });
       }},
      {kAclRuleAttrVlan,
       kOneValue,
       kOneValue,
       "<value>",
       [](std::string_view key, Values v) {
         auto vlan = parseIntInRange(
             key, v[kValue0], {utils::kVlanIdMin, utils::kVlanIdMax});
         return onEntry([vlan](cfg::AclEntry& r) { r.vlanID() = vlan; });
       }},
      {kAclRuleAttrIpType,
       kOneValue,
       kOneValue,
       "<value>",
       [](std::string_view, Values v) {
         auto it = parseIpType(v[kValue0]);
         return onEntry([it](cfg::AclEntry& r) { r.ipType() = it; });
       }},
      {kAclRuleAttrPacketLookupResult,
       kOneValue,
       kOneValue,
       "<value>",
       [](std::string_view, Values v) {
         auto plr = parsePacketLookupResult(v[kValue0]);
         return onEntry(
             [plr](cfg::AclEntry& r) { r.packetLookupResult() = plr; });
       }},
  };
  return kRows;
}

const std::vector<AclRuleRow>& actionRows() {
  static const std::vector<AclRuleRow> kRows = {
      {kAclRuleActionPermit,
       kNoValue,
       kNoValue,
       "",
       [](std::string_view, Values) {
         return onEntry([](cfg::AclEntry& r) {
           r.actionType() = cfg::AclActionType::PERMIT;
         });
       }},
      {kAclRuleActionDeny,
       kNoValue,
       kNoValue,
       "",
       [](std::string_view, Values) {
         return onEntry([](cfg::AclEntry& r) {
           r.actionType() = cfg::AclActionType::DENY;
         });
       }},
      {kAclRuleActionSendToQueue,
       kOneValue,
       kOneValue,
       "<value>",
       [](std::string_view key, Values v) {
         // queue id is i16 in QueueMatchAction; SAI queue ids are small.
         auto q = static_cast<int16_t>(
             parseIntInRange(key, v[kValue0], kSendToQueueRange));
         return onAction([q](cfg::MatchAction& ma) {
           cfg::QueueMatchAction a;
           a.queueId() = q;
           ma.sendToQueue() = a;
         });
       }},
      {kAclRuleActionSetDscp,
       kOneValue,
       kOneValue,
       "<value>",
       [](std::string_view key, Values v) {
         auto d = static_cast<int8_t>(
             parseIntInRange(key, v[kValue0], kSetDscpRange));
         return onAction([d](cfg::MatchAction& ma) {
           cfg::SetDscpMatchAction a;
           a.dscpValue() = d;
           ma.setDscp() = a;
         });
       }},
      {kAclRuleActionSetTc,
       kOneValue,
       kOneValue,
       "<value>",
       [](std::string_view key, Values v) {
         auto t = static_cast<int8_t>(
             parseIntInRange(key, v[kValue0], kTrafficClassRange));
         return onAction([t](cfg::MatchAction& ma) {
           cfg::SetTcAction a;
           a.tcValue() = t;
           ma.setTc() = a;
         });
       }},
      {kAclRuleActionMirrorIngress,
       kOneValue,
       kOneValue,
       "<value>",
       [](std::string_view key, Values v) {
         auto name = requireName(key, v[kValue0]);
         return onAction(
             [name](cfg::MatchAction& ma) { ma.ingressMirror() = name; });
       }},
      {kAclRuleActionMirrorEgress,
       kOneValue,
       kOneValue,
       "<value>",
       [](std::string_view key, Values v) {
         auto name = requireName(key, v[kValue0]);
         return onAction(
             [name](cfg::MatchAction& ma) { ma.egressMirror() = name; });
       }},
      {kAclRuleActionCounter,
       kOneValue,
       kOneValue,
       "<value>",
       [](std::string_view key, Values v) {
         auto name = requireName(key, v[kValue0]);
         return onAction([name](cfg::MatchAction& ma) { ma.counter() = name; });
       }},
      {kAclRuleActionTrapToCpu,
       kNoValue,
       kNoValue,
       "",
       [](std::string_view, Values) {
         return onAction([](cfg::MatchAction& ma) {
           ma.toCpuAction() = cfg::ToCpuAction::TRAP;
         });
       }},
      {kAclRuleActionCopyToCpu,
       kNoValue,
       kNoValue,
       "",
       [](std::string_view, Values) {
         return onAction([](cfg::MatchAction& ma) {
           ma.toCpuAction() = cfg::ToCpuAction::COPY;
         });
       }},
      {kAclRuleActionRedirect,
       kTwoValues,
       kTwoValues,
       "nexthop <ip>",
       [](std::string_view, Values v) {
         if (v[kValue0] != kAclRuleActionRedirectNexthopKeyword) {
           throw std::invalid_argument(
               fmt::format(
                   "Action 'redirect' expects keyword 'nexthop', got '{}'",
                   v[kValue0]));
         }
         std::string ip = v[kValue1];
         try {
           (void)folly::IPAddress{ip};
         } catch (const std::exception& e) {
           throw std::invalid_argument(
               fmt::format(
                   "Action 'redirect nexthop' expects an IP address, "
                   "got '{}': {}",
                   ip,
                   e.what()));
         }
         return onAction([ip](cfg::MatchAction& ma) {
           cfg::RedirectToNextHopAction rd;
           cfg::RedirectNextHop nh;
           nh.ip() = ip;
           rd.redirectNextHops()->push_back(std::move(nh));
           ma.redirectToNextHop() = std::move(rd);
         });
       }},
  };
  return kRows;
}

const AclRuleRow* findRow(
    const std::vector<AclRuleRow>& rows,
    std::string_view key) {
  for (const auto& row : rows) {
    if (row.key == key) {
      return &row;
    }
  }
  return nullptr;
}

std::string rowKeysCsv(const std::vector<AclRuleRow>& rows) {
  std::string out;
  for (const auto& row : rows) {
    if (!out.empty()) {
      out += ", ";
    }
    out += row.key;
  }
  return out;
}

// Attr-level keywords are the match fields plus `action`.
std::string attrKeysCsv() {
  return rowKeysCsv(matchFieldRows()) + ", " + std::string(kAclRuleAttrAction);
}

void checkArity(std::string_view kind, const AclRuleRow& row, Values vals) {
  if (vals.size() >= row.minVals && vals.size() <= row.maxVals) {
    return;
  }
  // Order the branches by which bound was crossed so the subtraction is always
  // guarded (no size_t underflow regardless of the row's arity).
  if (vals.size() < row.minVals) {
    throw std::invalid_argument(
        fmt::format(
            "{} '{}' expects '{}'; missing {} token(s)",
            kind,
            row.key,
            row.form,
            row.minVals - vals.size()));
  }
  const auto extra = vals.size() - row.maxVals;
  if (row.form.empty()) {
    throw std::invalid_argument(
        fmt::format(
            "{} '{}' takes no value; got {} extra token(s)",
            kind,
            row.key,
            extra));
  }
  throw std::invalid_argument(
      fmt::format(
          "{} '{}' expects '{}'; got {} extra token(s)",
          kind,
          row.key,
          row.form,
          extra));
}

AclRuleMutation buildMatchField(
    const std::string& attr,
    const std::vector<std::string>& v) {
  const auto* row = findRow(matchFieldRows(), attr);
  if (!row) {
    throw std::invalid_argument(
        fmt::format(
            "Unknown attribute '{}' for acl rule. Valid attrs: {}",
            attr,
            attrKeysCsv()));
  }
  Values vals(v.data() + kAclRuleMatchPrefix, v.size() - kAclRuleMatchPrefix);
  checkArity("Attribute", *row, vals);
  return row->build(row->key, vals);
}

AclRuleMutation buildAction(const std::vector<std::string>& v) {
  if (v.size() <= kAclRuleIdxActionSub) {
    throw std::invalid_argument(
        fmt::format(
            "Action requires a sub-attribute. Valid actions: {}",
            rowKeysCsv(actionRows())));
  }
  const auto* row = findRow(actionRows(), v[kAclRuleIdxActionSub]);
  if (!row) {
    throw std::invalid_argument(
        fmt::format(
            "Unknown action '{}' for acl rule. Valid actions: {}",
            v[kAclRuleIdxActionSub],
            rowKeysCsv(actionRows())));
  }
  Values vals(v.data() + kAclRuleActionPrefix, v.size() - kAclRuleActionPrefix);
  checkArity("Action", *row, vals);
  return row->build(row->key, vals);
}

} // namespace

std::string aclRuleConfigHelpText() {
  static const std::string kText =
      "<table-name> <rule-name> <attr> <value> [<extra>] "
      "creates the rule if missing; <attr> is one of: " +
      attrKeysCsv() +
      ". For 'action', <value> is one of: " + rowKeysCsv(actionRows()) +
      " (with an attr-specific final token: queue id / dscp / tc / "
      "mirror name / counter name; 'redirect' takes 'nexthop <ip>'; "
      "permit/deny/trap-to-cpu/copy-to-cpu take no value).";
  return kText;
}

AclRuleConfigArgs::AclRuleConfigArgs(std::vector<std::string> v) {
  if (v.size() < kAclRuleMatchPrefix) {
    throw std::invalid_argument(
        fmt::format(
            "Expected <table-name> <rule-name> <attr> <value> [<extra>], "
            "got {} argument(s)",
            v.size()));
  }
  tableName_ = v[kAclRuleIdxTable];
  ruleName_ = v[kAclRuleIdxRule];
  attribute_ = v[kAclRuleIdxAttr];
  // Echo every token after <attr> so the success message shows exactly what
  // was set (ttl mask, action value, redirect ip, ...).
  rawValue_ = folly::join(" ", v.begin() + kAclRuleMatchPrefix, v.end());

  AclRuleMutation m;
  if (attribute_ == kAclRuleAttrAction) {
    m = buildAction(v);
  } else {
    m = buildMatchField(attribute_, v);
  }
  applyEntryFn_ = std::move(m.entryFn);
  applyActionFn_ = std::move(m.actionFn);

  data_ = std::move(v);
}

void AclRuleConfigArgs::applyTo(cfg::AclEntry& rule) const {
  // Match-field attrs and `action permit|deny` populate applyEntryFn_; other
  // actions are MatchAction-typed and leave this empty (see applyActionTo()).
  if (applyEntryFn_) {
    applyEntryFn_(rule);
  }
}

bool AclRuleConfigArgs::isMatchAction() const {
  // MatchAction-typed actions populate applyActionFn_; match fields and action
  // permit|deny populate applyEntryFn_.
  return static_cast<bool>(applyActionFn_);
}

void AclRuleConfigArgs::applyActionTo(cfg::MatchAction& ma) const {
  if (applyActionFn_) {
    applyActionFn_(ma);
  }
}

CmdConfigAclRuleTraits::RetType CmdConfigAclRule::queryClient(
    const HostInfo& /* hostInfo */,
    const ObjectArgType& args) {
  auto& session = ConfigSession::getInstance();
  auto& config = session.getAgentConfig();
  auto& swConfig = *config.sw();

  auto [matchingTable, matchingGroupName] =
      findAclTable(swConfig, args.getTableName());

  auto& entries = *matchingTable->aclEntries();
  auto eit =
      std::find_if(entries.begin(), entries.end(), [&](const cfg::AclEntry& e) {
        return *e.name() == args.getRuleName();
      });
  bool created = false;
  if (eit == entries.end()) {
    cfg::AclEntry fresh;
    fresh.name() = args.getRuleName();
    entries.push_back(std::move(fresh));
    eit = std::prev(entries.end());
    created = true;
  }

  args.applyTo(*eit);

  // MatchAction-typed action sub-attrs (send-to-queue, set-dscp, set-tc,
  // mirror-ingress, mirror-egress, counter, trap/copy-to-cpu, redirect)
  // live on dataPlaneTrafficPolicy.matchToAction, keyed by rule name —
  // not on the AclEntry. Locate or create the MatchToAction for this
  // rule and apply the action to it.
  if (args.isMatchAction()) {
    if (!swConfig.dataPlaneTrafficPolicy()) {
      swConfig.dataPlaneTrafficPolicy() = cfg::TrafficPolicyConfig{};
    }
    auto& mtaList = *swConfig.dataPlaneTrafficPolicy()->matchToAction();
    auto mit = std::find_if(
        mtaList.begin(), mtaList.end(), [&](const cfg::MatchToAction& mta) {
          return *mta.matcher() == args.getRuleName();
        });
    if (mit == mtaList.end()) {
      cfg::MatchToAction fresh;
      fresh.matcher() = args.getRuleName();
      mtaList.push_back(std::move(fresh));
      mit = std::prev(mtaList.end());
    }
    args.applyActionTo(*mit->action());
  }

  // AclEntry mutations are applied at runtime via processAclTableGroupDelta
  // in SaiAclTableManager; SaiSwitch has no warmboot-prohibited guard for
  // ACL entry attributes (or for MatchAction edits applied alongside the
  // entry), so every supported attribute is HITLESS.
  session.saveConfig(cli::ServiceType::AGENT, cli::ConfigActionLevel::HITLESS);

  return fmt::format(
      "{} acl rule '{}' (table '{}', group '{}') {} to {}",
      created ? "Created and set" : "Set",
      args.getRuleName(),
      args.getTableName(),
      matchingGroupName,
      args.getAttribute(),
      args.getRawValue());
}

void CmdConfigAclRule::printOutput(const RetType& logMsg) {
  std::cout << logMsg << std::endl;
}

// Explicit template instantiation
template void CmdHandler<CmdConfigAclRule, CmdConfigAclRuleTraits>::run();

} // namespace facebook::fboss
