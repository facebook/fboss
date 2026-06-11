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
#include <folly/String.h>
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <set>
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

std::string aclRuleConfigHelpText() {
  // The attr/action lists are static, so the text is constant; build it once.
  static const std::string kText =
      "<table-name> <rule-name> <attr> <value> [<extra>] "
      "creates the rule if missing; <attr> is one of: " +
      aclRuleAttrsCsv() +
      ". For 'action', <value> is one of: " + aclRuleActionsCsv() +
      " (with an attr-specific final token: queue id / dscp / tc / "
      "mirror name / counter name; 'redirect' takes 'nexthop <ip>'; "
      "permit/deny/trap-to-cpu/copy-to-cpu take no value).";
  return kText;
}

namespace {

// ---- argument validation helpers ----
//
// Arity is driven by the declarative metadata in AclRuleAttrs
// (aclRuleAttrArity / aclRuleActionArity); these helpers turn an
// out-of-range token count into a message specific to the overrun.

void validateGenericArity(const std::vector<std::string>& v) {
  if (v.size() < 4 || v.size() > 6) {
    throw std::invalid_argument(
        fmt::format(
            "Expected <table-name> <rule-name> <attr> <value> [<extra>], "
            "got {} argument(s)",
            v.size()));
  }
}

void validateKnownAttr(const std::string& attr) {
  const auto& attrs = aclRuleValidAttrs();
  if (attrs.find(std::string_view(attr)) == attrs.end()) {
    throw std::invalid_argument(
        fmt::format(
            "Unknown attribute '{}' for acl rule. Valid attrs: {}",
            attr,
            aclRuleAttrsCsv()));
  }
}

void validateMatchFieldArity(
    const std::string& attr,
    const std::vector<std::string>& v) {
  const auto arity = aclRuleAttrArity(attr);
  if (v.size() >= arity.min && v.size() <= arity.max) {
    return;
  }
  // `ttl` is the only match field that accepts a second value (its mask).
  if (attr == kAclRuleAttrTtl) {
    throw std::invalid_argument(
        fmt::format(
            "Attribute 'ttl' takes a value and an optional mask "
            "('ttl <value> [<mask>]'); got {} value token(s)",
            v.size() - 3));
  }
  // Everything else is exactly one value; a 5th token is only meaningful for
  // `ttl`, so call that out specifically.
  if (v.size() == 5) {
    throw std::invalid_argument(
        fmt::format(
            "Attribute '{}' takes a single value; trailing argument '{}' "
            "is only valid for 'ttl <value> [<mask>]'",
            attr,
            v[4]));
  }
  throw std::invalid_argument(
      fmt::format(
          "Attribute '{}' takes a single value; got {} value token(s)",
          attr,
          v.size() - 3));
}

void validateKnownAction(const std::string& sub) {
  const auto& actions = aclRuleValidActions();
  if (actions.find(std::string_view(sub)) == actions.end()) {
    throw std::invalid_argument(
        fmt::format(
            "Unknown action '{}' for acl rule. Valid actions: {}",
            sub,
            aclRuleActionsCsv()));
  }
}

void validateActionArity(
    const std::string& sub,
    const std::vector<std::string>& v) {
  const auto arity = aclRuleActionArity(sub);
  if (v.size() >= arity.min && v.size() <= arity.max) {
    return;
  }
  // Phrase the error in terms of what's wrong: too few tokens means a missing
  // value, too many means trailing junk. v[0..2] are table/rule/attr, so the
  // value-token count is v.size() - 4 ('action' itself is v[3]).
  const bool tooFew = v.size() < arity.min;
  // Re-derive the action category from its arity to phrase the error.
  if (arity.max == 4) {
    // permit/deny/trap-to-cpu/copy-to-cpu: no value token expected.
    throw std::invalid_argument(
        fmt::format(
            "Action '{}' takes no value; got {} extra token(s)",
            sub,
            v.size() - 4));
  }
  if (arity.min == 6) {
    // redirect nexthop <ip>. Keep each format string a compile-time literal —
    // fmt's consteval check rejects a ternary-selected format string.
    if (tooFew) {
      throw std::invalid_argument(
          fmt::format(
              "Action 'redirect' requires 'nexthop <ip>'; missing {} token(s)",
              arity.min - v.size()));
    }
    throw std::invalid_argument(
        fmt::format(
            "Action 'redirect' requires 'nexthop <ip>'; got {} extra token(s)",
            v.size() - arity.max));
  }
  // Single-value actions (send-to-queue/set-dscp/set-tc/mirror-*/counter).
  if (tooFew) {
    throw std::invalid_argument(
        fmt::format("Action '{}' requires a value; none given", sub));
  }
  throw std::invalid_argument(
      fmt::format(
          "Action '{}' takes exactly one value; got {} extra token(s)",
          sub,
          v.size() - arity.max));
}

// Validate `value` parses as a CIDR network. folly::IPAddress::createNetwork
// accepts a bare IP (defaults to /32 or /128) or a full A.B.C.D/N form.
void validateCidrNetwork(const std::string& attr, const std::string& value) {
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

} // namespace

AclRuleConfigArgs::AclRuleConfigArgs(std::vector<std::string> v) {
  validateGenericArity(v);
  tableName_ = v[0];
  ruleName_ = v[1];
  attribute_ = v[2];
  validateKnownAttr(attribute_);
  // The value spans every token after <attr> — for most attrs that's a single
  // token (v[3]), but `ttl <value> <mask>` and the action forms (e.g.
  // `send-to-queue 7`, `redirect nexthop <ip>`) carry more. Capture the whole
  // tail so the success message echoes what the user actually set.
  rawValue_ = folly::join(" ", v.begin() + 3, v.end());

  if (attribute_ == kAclRuleAttrAction) {
    parseAction(v);
  } else {
    validateMatchFieldArity(attribute_, v);
    parseMatchField(v);
  }

  data_ = std::move(v);
}

void AclRuleConfigArgs::parseMatchField(const std::vector<std::string>& v) {
  const std::string& attr = attribute_;
  const std::string& value = v[3];

  if (attr == kAclRuleAttrSourceIp || attr == kAclRuleAttrDestinationIp) {
    validateCidrNetwork(attr, value);
    if (attr == kAclRuleAttrSourceIp) {
      applyEntryFn_ = [value](cfg::AclEntry& r) { r.srcIp() = value; };
    } else {
      applyEntryFn_ = [value](cfg::AclEntry& r) { r.dstIp() = value; };
    }
  } else if (attr == kAclRuleAttrProtocol) {
    const auto proto = static_cast<int16_t>(parseProtocol(value));
    applyEntryFn_ = [proto](cfg::AclEntry& r) { r.proto() = proto; };
  } else if (attr == kAclRuleAttrSourcePort) {
    const auto port = parseIntInRange(attr, value, kL4PortRange);
    applyEntryFn_ = [port](cfg::AclEntry& r) { r.l4SrcPort() = port; };
  } else if (attr == kAclRuleAttrDestinationPort) {
    const auto port = parseIntInRange(attr, value, kL4PortRange);
    applyEntryFn_ = [port](cfg::AclEntry& r) { r.l4DstPort() = port; };
  } else if (attr == kAclRuleAttrDscp) {
    const auto dscp =
        static_cast<int8_t>(parseIntInRange(attr, value, kDscpRange));
    applyEntryFn_ = [dscp](cfg::AclEntry& r) { r.dscp() = dscp; };
  } else if (attr == kAclRuleAttrTcpFlags) {
    const auto flags =
        static_cast<int16_t>(parseIntInRange(attr, value, kTcpFlagsRange));
    applyEntryFn_ = [flags](cfg::AclEntry& r) { r.tcpFlagsBitMap() = flags; };
  } else if (attr == kAclRuleAttrIcmpType) {
    const auto t =
        static_cast<int16_t>(parseIntInRange(attr, value, kIcmpByteRange));
    applyEntryFn_ = [t](cfg::AclEntry& r) { r.icmpType() = t; };
  } else if (attr == kAclRuleAttrIcmpCode) {
    const auto c =
        static_cast<int16_t>(parseIntInRange(attr, value, kIcmpByteRange));
    applyEntryFn_ = [c](cfg::AclEntry& r) { r.icmpCode() = c; };
  } else if (attr == kAclRuleAttrIpFragment) {
    const auto frag = parseIpFragment(value);
    applyEntryFn_ = [frag](cfg::AclEntry& r) { r.ipFrag() = frag; };
  } else if (attr == kAclRuleAttrTtl) {
    const auto val =
        static_cast<int16_t>(parseIntInRange(attr, value, kTtlRange));
    const int16_t mask = v.size() == 5
        ? static_cast<int16_t>(parseIntInRange("ttl-mask", v[4], kTtlRange))
        : kTtlMaskDefault;
    applyEntryFn_ = [val, mask](cfg::AclEntry& r) {
      cfg::Ttl ttl;
      ttl.value() = val;
      ttl.mask() = mask;
      r.ttl() = ttl;
    };
  } else if (attr == kAclRuleAttrDestinationMac) {
    try {
      (void)folly::MacAddress{value};
    } catch (const std::exception& e) {
      throw std::invalid_argument(
          fmt::format(
              "Value for 'destination-mac' must be a MAC address, got '{}': {}",
              value,
              e.what()));
    }
    applyEntryFn_ = [value](cfg::AclEntry& r) { r.dstMac() = value; };
  } else if (attr == kAclRuleAttrEtherType) {
    const auto et = parseEtherType(value);
    applyEntryFn_ = [et](cfg::AclEntry& r) { r.etherType() = et; };
  } else if (attr == kAclRuleAttrVlan) {
    const auto vlan =
        parseIntInRange(attr, value, {utils::kVlanIdMin, utils::kVlanIdMax});
    applyEntryFn_ = [vlan](cfg::AclEntry& r) { r.vlanID() = vlan; };
  } else if (attr == kAclRuleAttrIpType) {
    const auto it = parseIpType(value);
    applyEntryFn_ = [it](cfg::AclEntry& r) { r.ipType() = it; };
  } else if (attr == kAclRuleAttrPacketLookupResult) {
    const auto plr = parsePacketLookupResult(value);
    applyEntryFn_ = [plr](cfg::AclEntry& r) { r.packetLookupResult() = plr; };
  }
}

void AclRuleConfigArgs::parseAction(const std::vector<std::string>& v) {
  // v[3] is the action sub-attribute; v[4] (and v[5] for `redirect nexthop
  // <ip>`) carry the value.
  const std::string& sub = v[3];
  validateKnownAction(sub);
  actionSubattr_ = sub;
  validateActionArity(sub, v);

  if (sub == kAclRuleActionPermit) {
    applyEntryFn_ = [](cfg::AclEntry& r) {
      r.actionType() = cfg::AclActionType::PERMIT;
    };
  } else if (sub == kAclRuleActionDeny) {
    applyEntryFn_ = [](cfg::AclEntry& r) {
      r.actionType() = cfg::AclActionType::DENY;
    };
  } else if (sub == kAclRuleActionTrapToCpu) {
    applyActionFn_ = [](cfg::MatchAction& ma) {
      ma.toCpuAction() = cfg::ToCpuAction::TRAP;
    };
  } else if (sub == kAclRuleActionCopyToCpu) {
    applyActionFn_ = [](cfg::MatchAction& ma) {
      ma.toCpuAction() = cfg::ToCpuAction::COPY;
    };
  } else if (sub == kAclRuleActionRedirect) {
    if (v[4] != kAclRuleActionRedirectNexthopKeyword) {
      throw std::invalid_argument(
          fmt::format(
              "Action 'redirect' expects keyword 'nexthop', got '{}'", v[4]));
    }
    const std::string& ip = v[5];
    try {
      (void)folly::IPAddress{ip};
    } catch (const std::exception& e) {
      throw std::invalid_argument(
          fmt::format(
              "Action 'redirect nexthop' expects an IP address, got '{}': {}",
              ip,
              e.what()));
    }
    applyActionFn_ = [ip](cfg::MatchAction& ma) {
      cfg::RedirectToNextHopAction rd;
      cfg::RedirectNextHop nh;
      nh.ip() = ip;
      rd.redirectNextHops()->push_back(std::move(nh));
      ma.redirectToNextHop() = std::move(rd);
    };
  } else if (sub == kAclRuleActionSendToQueue) {
    // queue id is i16 in QueueMatchAction; SAI queue ids are small
    // non-negative integers.
    const auto q =
        static_cast<int16_t>(parseIntInRange(sub, v[4], kSendToQueueRange));
    applyActionFn_ = [q](cfg::MatchAction& ma) {
      cfg::QueueMatchAction a;
      a.queueId() = q;
      ma.sendToQueue() = a;
    };
  } else if (sub == kAclRuleActionSetDscp) {
    const auto d =
        static_cast<int8_t>(parseIntInRange(sub, v[4], kSetDscpRange));
    applyActionFn_ = [d](cfg::MatchAction& ma) {
      cfg::SetDscpMatchAction a;
      a.dscpValue() = d;
      ma.setDscp() = a;
    };
  } else if (sub == kAclRuleActionSetTc) {
    const auto t =
        static_cast<int8_t>(parseIntInRange(sub, v[4], kTrafficClassRange));
    applyActionFn_ = [t](cfg::MatchAction& ma) {
      cfg::SetTcAction a;
      a.tcValue() = t;
      ma.setTc() = a;
    };
  } else if (
      sub == kAclRuleActionMirrorIngress || sub == kAclRuleActionMirrorEgress ||
      sub == kAclRuleActionCounter) {
    const std::string& name = v[4];
    if (name.empty()) {
      throw std::invalid_argument(
          fmt::format("Action '{}' requires a non-empty name", sub));
    }
    if (sub == kAclRuleActionMirrorIngress) {
      applyActionFn_ = [name](cfg::MatchAction& ma) {
        ma.ingressMirror() = name;
      };
    } else if (sub == kAclRuleActionMirrorEgress) {
      applyActionFn_ = [name](cfg::MatchAction& ma) {
        ma.egressMirror() = name;
      };
    } else {
      applyActionFn_ = [name](cfg::MatchAction& ma) { ma.counter() = name; };
    }
  }
}

void AclRuleConfigArgs::applyTo(cfg::AclEntry& rule) const {
  // Match-field attrs and `action permit|deny` populate applyEntryFn_; other
  // actions are MatchAction-typed and leave this empty (see applyActionTo()).
  if (applyEntryFn_) {
    applyEntryFn_(rule);
  }
}

bool AclRuleConfigArgs::isMatchAction() const {
  if (attribute_ != kAclRuleAttrAction) {
    return false;
  }
  return actionSubattr_ != kAclRuleActionPermit &&
      actionSubattr_ != kAclRuleActionDeny;
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
