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
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "fboss/agent/gen-cpp2/switch_config_types.h"

// Single source of truth for the set of <attr> values accepted by both
// `config acl rule` and `delete acl rule`. The two commands must accept the
// same attributes (delete clears whatever config can set), so keeping the
// list here avoids the three-way drift the strings used to invite.

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

// Ordered list of every supported <attr>. Drives the validity check, the
// `--help` description, and the "Unknown attribute" error message.
const std::vector<std::string_view>& aclRuleAttrsOrdered();

// O(log n) lookup view over aclRuleAttrsOrdered().
const std::set<std::string_view>& aclRuleValidAttrs();

// Ordered list of every supported action sub-attribute (the token that
// follows `action`).
const std::vector<std::string_view>& aclRuleActionsOrdered();

// O(log n) lookup view over aclRuleActionsOrdered().
const std::set<std::string_view>& aclRuleValidActions();

// "source-ip, destination-ip, protocol, ..." — the ordered list joined
// with ", ".
std::string aclRuleAttrsCsv();

// "permit, deny, send-to-queue, ..." — joined with ", ".
std::string aclRuleActionsCsv();

// Inclusive [min, max] total token count for a complete `config acl rule`
// invocation (table, rule, attr, value, [extras]). Drives arity validation so
// the rules live as data next to the attribute list rather than as a special-
// case ladder in the parser.
struct AclRuleArity {
  std::size_t min;
  std::size_t max;
};

// Arity for a match-field <attr>. `ttl` accepts an optional 5th mask token;
// `action` is a placeholder ({4, 6}) refined by aclRuleActionArity(); every
// other attribute takes exactly one value (4 tokens).
AclRuleArity aclRuleAttrArity(std::string_view attr);

// Arity for an `action <sub>`: no-value actions (permit/deny/trap/copy) are 4
// tokens, `redirect nexthop <ip>` is 6, single-value actions are 5.
AclRuleArity aclRuleActionArity(std::string_view sub);

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
