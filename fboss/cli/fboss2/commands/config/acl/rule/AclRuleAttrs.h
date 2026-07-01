/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "fboss/agent/gen-cpp2/switch_config_types.h"

// The `config acl rule` / `delete acl rule` grammar: <attr>/<action> keywords,
// value ranges, token positions, and the parser that turns a token vector into
// a mutation (parseAclRuleSpec). The descriptor tables that drive it live in
// AclRuleAttrs.cpp; CmdConfigAclRule.cpp just wires the result into the config.

namespace facebook::fboss {

inline constexpr std::string_view kAclRuleAttrSourceIp = "source-ip";
inline constexpr std::string_view kAclRuleAttrDestinationIp = "destination-ip";
inline constexpr std::string_view kAclRuleAttrProtocol = "protocol";
inline constexpr std::string_view kAclRuleAttrSourcePort = "source-port";
inline constexpr std::string_view kAclRuleAttrDestinationPort =
    "destination-port";
inline constexpr std::string_view kAclRuleAttrDscp = "dscp";
inline constexpr std::string_view kAclRuleAttrTcpFlags = "tcp-flags";
inline constexpr std::string_view kAclRuleAttrIcmpType = "icmp-type";
inline constexpr std::string_view kAclRuleAttrIcmpCode = "icmp-code";
inline constexpr std::string_view kAclRuleAttrIpFragment = "ip-fragment";
inline constexpr std::string_view kAclRuleAttrTtl = "ttl";
inline constexpr std::string_view kAclRuleAttrDestinationMac =
    "destination-mac";
inline constexpr std::string_view kAclRuleAttrEtherType = "ethertype";
inline constexpr std::string_view kAclRuleAttrVlan = "vlan";
inline constexpr std::string_view kAclRuleAttrIpType = "ip-type";
inline constexpr std::string_view kAclRuleAttrPacketLookupResult =
    "packet-lookup-result";
inline constexpr std::string_view kAclRuleAttrAction = "action";

// `action <subattr> [<value>]` sub-attributes. permit/deny mutate
// AclEntry.actionType directly; the rest land on the MatchAction stored in
// dataPlaneTrafficPolicy.matchToAction keyed by rule name.
inline constexpr std::string_view kAclRuleActionPermit = "permit";
inline constexpr std::string_view kAclRuleActionDeny = "deny";
inline constexpr std::string_view kAclRuleActionSendToQueue = "send-to-queue";
inline constexpr std::string_view kAclRuleActionSetDscp = "set-dscp";
inline constexpr std::string_view kAclRuleActionSetTc = "set-tc";
inline constexpr std::string_view kAclRuleActionMirrorIngress =
    "mirror-ingress";
inline constexpr std::string_view kAclRuleActionMirrorEgress = "mirror-egress";
inline constexpr std::string_view kAclRuleActionCounter = "counter";
inline constexpr std::string_view kAclRuleActionTrapToCpu = "trap-to-cpu";
inline constexpr std::string_view kAclRuleActionCopyToCpu = "copy-to-cpu";
inline constexpr std::string_view kAclRuleActionRedirect = "redirect";
inline constexpr std::string_view kAclRuleActionRedirectNexthopKeyword =
    "nexthop";

// Inclusive [min, max] bound for a numeric <attr>/<value>. Names the
// otherwise-magic limits fed to parseIntInRange and documents why each
// bound is what it is (field bit-width or hardware/thrift type range).
struct AclRuleRange {
  int64_t min;
  int64_t max;
};

// Match-field value ranges.
inline constexpr AclRuleRange kProtocolRange{0, 0xFF}; // 8-bit IP protocol
inline constexpr AclRuleRange kL4PortRange{0, 0xFFFF}; // 16-bit L4 port
inline constexpr AclRuleRange kDscpRange{0, 63}; // 6-bit DSCP codepoint
inline constexpr AclRuleRange kTcpFlagsRange{0, 0xFF}; // 8-bit flag bitmask
inline constexpr AclRuleRange kIcmpByteRange{0, 0xFF}; // 8-bit ICMP type/code
inline constexpr AclRuleRange kTtlRange{0, 0xFF}; // 8-bit TTL value/mask
inline constexpr AclRuleRange kU16Range{0, 0xFFFF}; // ethertype / pkt-lookup
inline constexpr int16_t kTtlMaskDefault = 0xFF; // == thrift Ttl.mask default

// Action sub-attribute value ranges.
inline constexpr AclRuleRange kSendToQueueRange{
    0,
    32767}; // i16 QueueMatchAction
inline constexpr AclRuleRange kSetDscpRange{0, 63}; // 6-bit DSCP codepoint
inline constexpr AclRuleRange kTrafficClassRange{0, 7}; // 8 traffic classes

// Positions within a `config acl rule` argument vector:
//   match field:  <table> <rule> <attr>   <value> [<mask>]
//   action:       <table> <rule> action   <sub>   [<value> | nexthop <ip>]
inline constexpr std::size_t kAclRuleIdxTable = 0;
inline constexpr std::size_t kAclRuleIdxRule = 1;
inline constexpr std::size_t kAclRuleIdxAttr = 2;

// Fixed tokens preceding the value(s): three for a match field, four for an
// action (the extra token is the action <sub>). Value tokens follow the prefix,
// so v[kAclRule*Prefix] is always the first value.
inline constexpr std::size_t kAclRuleMatchPrefix = 3;
inline constexpr std::size_t kAclRuleActionPrefix = 4;

// The action <sub> occupies the slot a match value would (index 3).
inline constexpr std::size_t kAclRuleIdxActionSub = kAclRuleMatchPrefix;

// A parsed acl-rule mutation, captured at parse time and replayed later.
// Exactly one function is set: match fields and action permit|deny target the
// AclEntry; every other action targets a MatchAction.
struct AclRuleMutation {
  std::function<void(cfg::AclEntry&)> entryFn;
  std::function<void(cfg::MatchAction&)> actionFn;
};

// Parse a `config acl rule` token vector (<table> <rule> <attr> <value>...)
// into a mutation. Validates the attr/action keyword and its arity; throws
// std::invalid_argument on any problem. Shared entry point for the command's
// arg type.
AclRuleMutation parseAclRuleSpec(const std::vector<std::string>& tokens);

// Comma-separated accepted keywords, for `--help` and error text: the match
// fields plus `action`, and the action sub-attributes, respectively.
std::string aclRuleAttrKeysCsv();
std::string aclRuleActionKeysCsv();

// Locate the AclTable named `tableName` across every AclTableGroup in
// `swConfig` (ACL table names are unique within a group, so we walk all
// groups and the caller need not name the group). Returns the table pointer
// and its owning group's name. Throws std::runtime_error if there are no
// aclTableGroups (only field-56 is supported, not the deprecated field-45
// aclTableGroup) or if no group contains the table. Shared by
// `config acl rule` and `delete acl rule` so the two resolve tables
// identically.
std::pair<cfg::AclTable*, std::string> findAclTable(
    cfg::SwitchConfig& swConfig,
    const std::string& tableName);

} // namespace facebook::fboss
