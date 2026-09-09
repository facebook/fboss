/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/protocol/bgp/peer-group/CmdConfigProtocolBgpPeerGroup.h"

#include "fboss/cli/fboss2/CmdHandler.cpp"

#include <fmt/core.h>
#include <neteng/fboss/bgp/public_tld/configerator/structs/neteng/fboss/bgp/gen-cpp2/bgp_config_types.h>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <ostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include "configerator/structs/neteng/fboss/bgp/if/gen-cpp2/bgp_attr_types.h"
#include "fboss/cli/fboss2/commands/config/protocol/bgp/BgpCliAttrHandlers.h"
#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/utils/CmdUtilsCommon.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"
#include "fmt/format.h"

namespace facebook::fboss {

namespace {

// CLI attribute names, exactly as documented (two-token attributes keep their
// space). Kept here (not as raw string literals at the dispatch sites) so the
// valid-attribute set and the handler table stay in sync.
constexpr std::string_view kRemoteAsn = "remote-asn";
constexpr std::string_view kLocalAsn = "local-asn";
constexpr std::string_view kDescription = "description";
constexpr std::string_view kPeerTag = "peer-tag";
constexpr std::string_view kIngressPolicy = "ingress-policy";
constexpr std::string_view kEgressPolicy = "egress-policy";
constexpr std::string_view kRrClient = "rr-client";
constexpr std::string_view kRedistributePeer = "redistribute-peer";
constexpr std::string_view kEnhancedRouteRefresh = "enhanced-route-refresh";
constexpr std::string_view kPassive = "passive";
constexpr std::string_view kAddPathSend = "add-path send";
constexpr std::string_view kAddPathReceive = "add-path receive";
constexpr std::string_view kAfiDisableIpv4Afi = "afi disable-ipv4-afi";
constexpr std::string_view kAfiDisableIpv6Afi = "afi disable-ipv6-afi";
constexpr std::string_view kAfiIpv4OverIpv6Nh = "afi ipv4-over-ipv6-nh";
constexpr std::string_view kGracefulRestartTime =
    "graceful-restart restart-time";
constexpr std::string_view kGracefulRestartStatefulHa =
    "graceful-restart stateful-ha";
constexpr std::string_view kMaxRoutePreFilter = "max-route pre-filter";
constexpr std::string_view kMaxRoutePostFilter = "max-route post-filter";
constexpr std::string_view kMaxRoutePostWarningThreshold =
    "max-route post-warning-threshold";
constexpr std::string_view kTimersHoldTime = "timers hold-time";
constexpr std::string_view kTimersKeepalive = "timers keepalive";
constexpr std::string_view kTimersOutDelay = "timers out-delay";
// Parity attributes: not in the documented peer-group grammar, but supported
// by the per-attribute peer-group commands this dispatcher replaces (and by
// real PeerGroup thrift fields), so dropping them would regress existing
// usage.
constexpr std::string_view kConfedPeer = "confed-peer";
constexpr std::string_view kNextHopSelf = "next-hop-self";
constexpr std::string_view kTimersWithdrawUnprogDelay =
    "timers withdraw-unprog-delay";
constexpr std::string_view kMaxRoutePreWarningThreshold =
    "max-route pre-warning-threshold";
constexpr std::string_view kMaxRoutePreWarningOnly =
    "max-route pre-warning-only";
constexpr std::string_view kMaxRoutePostWarningOnly =
    "max-route post-warning-only";

// Protocol ranges bgpd enforces (or the wire format silently truncates to):
// the OPEN hold time is 16 bits and bgpd refuses 1-2s; the graceful-restart
// restart time is the 12-bit field of RFC 4724.
constexpr int32_t kHoldTimeMinSeconds = 3;
constexpr int32_t kHoldTimeMaxSeconds = 65535;
constexpr int32_t kGracefulRestartMaxSeconds = 4095;

using BgpConfig = bgp::thrift::BgpConfig;
using PeerGroup = bgp::thrift::PeerGroup;
using bgpcli::asnAttr;
using bgpcli::AttrHandler;
using bgpcli::boolAttr;
using bgpcli::err;
using bgpcli::joinedStringAttr;
using bgpcli::ok;
using bgpcli::parseBool;
using bgpcli::Result;
using bgpcli::routeCountAttr;
using bgpcli::secondsAttr;
using bgpcli::SecondsRange;
using bgpcli::stringAttr;
using bgpcli::Tokens;
using facebook::neteng::fboss::bgp_attr::AddPath;

// ---- per-attribute setters -------------------------------------------------
// Pure thrift assignment: no validation, no messages. Parsing, bounds and the
// user-facing text all live in the shared factories in BgpCliAttrHandlers.h,
// so a setter is just the field this attribute writes. Field names map 1:1 to
// bgp_config.thrift, so the compiler validates them.

void setRemoteAsn(PeerGroup& g, int64_t v) {
  g.remote_as_4_byte() = v;
}

void setLocalAsn(PeerGroup& g, int64_t v) {
  g.local_as_4_byte() = v;
}

void setDescription(PeerGroup& g, const std::string& v) {
  g.description() = v;
}

void setPeerTag(PeerGroup& g, const std::string& v) {
  g.peer_tag() = v;
}

void setIngressPolicy(PeerGroup& g, const std::string& v) {
  g.ingress_policy_name() = v;
}

void setEgressPolicy(PeerGroup& g, const std::string& v) {
  g.egress_policy_name() = v;
}

void setRrClient(PeerGroup& g, bool v) {
  g.is_rr_client() = v;
}

void setConfedPeer(PeerGroup& g, bool v) {
  g.is_confed_peer() = v;
}

void setRedistributePeer(PeerGroup& g, bool v) {
  g.is_redistribute_peer() = v;
}

void setEnhancedRouteRefresh(PeerGroup& g, bool v) {
  g.enhanced_route_refresh() = v;
}

// bgpd reads is_passive=true as PASSIVE_ONLY (listen) and false as
// PASSIVE_ACTIVE (listen and connect); there is no active-only mode, so the
// CLI exposes the flag itself rather than inventing connect-mode names.
void setPassive(PeerGroup& g, bool v) {
  g.is_passive() = v;
}

void setDisableIpv4Afi(PeerGroup& g, bool v) {
  g.disable_ipv4_afi() = v;
}

void setDisableIpv6Afi(PeerGroup& g, bool v) {
  g.disable_ipv6_afi() = v;
}

void setIpv4OverIpv6Nh(PeerGroup& g, bool v) {
  g.v4_over_v6_nexthop() = v;
}

void setGracefulRestartTime(PeerGroup& g, int32_t v) {
  g.bgp_peer_timers().ensure().graceful_restart_seconds() = v;
}

void setGracefulRestartStatefulHa(PeerGroup& g, bool v) {
  g.enable_stateful_ha() = v;
}

void setMaxRoutePreFilter(PeerGroup& g, int64_t v) {
  g.pre_filter().ensure().max_routes() = v;
}

void setMaxRoutePostFilter(PeerGroup& g, int64_t v) {
  g.post_filter().ensure().max_routes() = v;
}

// RouteLimit.warning_limit is an absolute route count in the thrift schema,
// not a percentage.
void setMaxRoutePreWarningThreshold(PeerGroup& g, int64_t v) {
  g.pre_filter().ensure().warning_limit() = v;
}

void setMaxRoutePostWarningThreshold(PeerGroup& g, int64_t v) {
  g.post_filter().ensure().warning_limit() = v;
}

void setMaxRoutePreWarningOnly(PeerGroup& g, bool v) {
  g.pre_filter().ensure().warning_only() = v;
}

void setMaxRoutePostWarningOnly(PeerGroup& g, bool v) {
  g.post_filter().ensure().warning_only() = v;
}

void setTimersHoldTime(PeerGroup& g, int32_t v) {
  g.bgp_peer_timers().ensure().hold_time_seconds() = v;
}

void setTimersKeepalive(PeerGroup& g, int32_t v) {
  g.bgp_peer_timers().ensure().keep_alive_seconds() = v;
}

void setTimersOutDelay(PeerGroup& g, int32_t v) {
  g.bgp_peer_timers().ensure().out_delay_seconds() = v;
}

void setTimersWithdrawUnprogDelay(PeerGroup& g, int32_t v) {
  g.bgp_peer_timers().ensure().withdraw_unprog_delay_seconds() = v;
}

void setNextHopSelf(PeerGroup& g, bool v) {
  g.next_hop_self() = v;
}

// ---- inherited values ------------------------------------------------------
// bgpd applies a peer group's bgp_peer_timers to its members as a WHOLE
// struct: once the group carries one, every member takes hold, keepalive and
// out-delay from it, and hold_time_seconds / keep_alive_seconds are
// non-optional i32 (default 0). Hold time 0 means no keepalives, so a timers
// struct created for one field would silently disable dead-peer detection for
// the whole group. Before the first timer attribute is set on a group, the
// struct is therefore seeded from what its members use today -- the global
// hold time, with the conventional hold/3 keepalive -- and the requested field
// is then changed on top of it. pre_filter / post_filter need no seeding: a
// group has nothing above it to inherit a RouteLimit from, and the setter's
// ensure() yields the thrift defaults, which is also what bgpd applies to a
// peer without one.

void seedTimers(const BgpConfig& cfg, PeerGroup& group) {
  if (group.bgp_peer_timers()) {
    return;
  }
  bgp::thrift::BgpPeerTimers timers;
  timers.hold_time_seconds() = *cfg.hold_time();
  timers.keep_alive_seconds() = *cfg.hold_time() / 3;
  group.bgp_peer_timers() = std::move(timers);
}

// A peer-group handler sees the whole config (for the global defaults) plus
// the group being edited. The shared factories only know the group, so they
// are adapted; the adapter for timer attributes seeds first.
using PeerGroupHandler =
    std::function<Result(const BgpConfig&, PeerGroup&, const Tokens&)>;

PeerGroupHandler groupOnly(AttrHandler<PeerGroup> handler) {
  return [handler = std::move(handler)](
             const BgpConfig&, PeerGroup& group, const Tokens& values) {
    return handler(group, values);
  };
}

PeerGroupHandler withTimers(AttrHandler<PeerGroup> handler) {
  return [handler = std::move(handler)](
             const BgpConfig& cfg, PeerGroup& group, const Tokens& values) {
    seedTimers(cfg, group);
    return handler(group, values);
  };
}

// ---- hand-written handlers -------------------------------------------------
// Only for value shapes no factory covers, because the shape is unique to this
// attribute rather than reusable. Mirrors the neighbor dispatcher's version,
// which writes the identically-named BgpPeer field.

// add-path send/receive: a boolean at the CLI, but the two attributes share
// one thrift field — add_path is a single enum whose values form a bitmask by
// design (RECEIVE=1, SEND=2, BOTH=3). Each takes the boolAttr text but merges
// its direction into the current value instead of overwriting it; clearing the
// last direction unsets the field.
Result applyAddPath(PeerGroup& group, bool send, const Tokens& values) {
  const std::string_view name = send ? kAddPathSend : kAddPathReceive;
  if (values.size() != 1) {
    return err(fmt::format("Error: {} requires <true|false>", name));
  }
  auto enable = parseBool(values[0]);
  if (!enable) {
    return err(
        fmt::format(
            "Error: Invalid {} value '{}'; expected true or false",
            name,
            values[0]));
  }
  int bits = group.add_path() ? static_cast<int>(*group.add_path()) : 0;
  const int bit = send ? static_cast<int>(AddPath::SEND)
                       : static_cast<int>(AddPath::RECEIVE);
  bits = *enable ? (bits | bit) : (bits & ~bit);
  if (bits == 0) {
    group.add_path().reset();
  } else {
    group.add_path() = static_cast<AddPath>(bits);
  }
  return ok(
      fmt::format(
          "Successfully {} {}", *enable ? "enabled" : "disabled", name));
}

Result addPathSend(PeerGroup& group, const Tokens& values) {
  return applyAddPath(group, /* send */ true, values);
}

Result addPathReceive(PeerGroup& group, const Tokens& values) {
  return applyAddPath(group, /* send */ false, values);
}

// ---- the dispatch table ----------------------------------------------------
// One line per attribute: dispatch key, value shape, setter. There are no
// handler bodies here by design — if an attribute appears to need one, its
// value shape is missing a factory and the fix is to add the factory.

const std::map<std::string, PeerGroupHandler, std::less<>>& attrHandlers() {
  auto plain = [](AttrHandler<PeerGroup> h) { return groupOnly(std::move(h)); };
  auto timers = [](AttrHandler<PeerGroup> h) {
    return withTimers(std::move(h));
  };
  static const std::map<std::string, PeerGroupHandler, std::less<>> kHandlers =
      {
          {std::string(kRemoteAsn),
           plain(asnAttr<PeerGroup>(kRemoteAsn, setRemoteAsn))},
          {std::string(kLocalAsn),
           plain(asnAttr<PeerGroup>(kLocalAsn, setLocalAsn))},
          {std::string(kDescription),
           plain(joinedStringAttr<PeerGroup>(kDescription, setDescription))},
          {std::string(kPeerTag),
           plain(stringAttr<PeerGroup>(kPeerTag, "string", setPeerTag))},
          {std::string(kIngressPolicy),
           plain(
               stringAttr<PeerGroup>(
                   kIngressPolicy, "policy-name", setIngressPolicy))},
          {std::string(kEgressPolicy),
           plain(
               stringAttr<PeerGroup>(
                   kEgressPolicy, "policy-name", setEgressPolicy))},
          {std::string(kRrClient),
           plain(boolAttr<PeerGroup>(kRrClient, setRrClient))},
          {std::string(kConfedPeer),
           plain(boolAttr<PeerGroup>(kConfedPeer, setConfedPeer))},
          {std::string(kRedistributePeer),
           plain(boolAttr<PeerGroup>(kRedistributePeer, setRedistributePeer))},
          {std::string(kEnhancedRouteRefresh),
           plain(
               boolAttr<PeerGroup>(
                   kEnhancedRouteRefresh, setEnhancedRouteRefresh))},
          {std::string(kPassive),
           plain(boolAttr<PeerGroup>(kPassive, setPassive))},
          {std::string(kAddPathSend), plain(addPathSend)},
          {std::string(kAddPathReceive), plain(addPathReceive)},
          {std::string(kAfiDisableIpv4Afi),
           plain(boolAttr<PeerGroup>(kAfiDisableIpv4Afi, setDisableIpv4Afi))},
          {std::string(kAfiDisableIpv6Afi),
           plain(boolAttr<PeerGroup>(kAfiDisableIpv6Afi, setDisableIpv6Afi))},
          {std::string(kAfiIpv4OverIpv6Nh),
           plain(boolAttr<PeerGroup>(kAfiIpv4OverIpv6Nh, setIpv4OverIpv6Nh))},
          {std::string(kGracefulRestartTime),
           timers(
               secondsAttr<PeerGroup>(
                   kGracefulRestartTime,
                   setGracefulRestartTime,
                   SecondsRange{0, kGracefulRestartMaxSeconds}))},
          {std::string(kGracefulRestartStatefulHa),
           plain(
               boolAttr<PeerGroup>(
                   kGracefulRestartStatefulHa, setGracefulRestartStatefulHa))},
          {std::string(kMaxRoutePreFilter),
           plain(
               routeCountAttr<PeerGroup>(
                   kMaxRoutePreFilter, setMaxRoutePreFilter))},
          {std::string(kMaxRoutePostFilter),
           plain(
               routeCountAttr<PeerGroup>(
                   kMaxRoutePostFilter, setMaxRoutePostFilter))},
          {std::string(kMaxRoutePreWarningThreshold),
           plain(
               routeCountAttr<PeerGroup>(
                   kMaxRoutePreWarningThreshold,
                   setMaxRoutePreWarningThreshold))},
          {std::string(kMaxRoutePostWarningThreshold),
           plain(
               routeCountAttr<PeerGroup>(
                   kMaxRoutePostWarningThreshold,
                   setMaxRoutePostWarningThreshold))},
          {std::string(kMaxRoutePreWarningOnly),
           plain(
               boolAttr<PeerGroup>(
                   kMaxRoutePreWarningOnly, setMaxRoutePreWarningOnly))},
          {std::string(kMaxRoutePostWarningOnly),
           plain(
               boolAttr<PeerGroup>(
                   kMaxRoutePostWarningOnly, setMaxRoutePostWarningOnly))},
          {std::string(kTimersHoldTime),
           timers(
               secondsAttr<PeerGroup>(
                   kTimersHoldTime,
                   setTimersHoldTime,
                   SecondsRange{kHoldTimeMinSeconds, kHoldTimeMaxSeconds}))},
          {std::string(kTimersKeepalive),
           timers(
               secondsAttr<PeerGroup>(kTimersKeepalive, setTimersKeepalive))},
          {std::string(kTimersOutDelay),
           timers(secondsAttr<PeerGroup>(kTimersOutDelay, setTimersOutDelay))},
          {std::string(kTimersWithdrawUnprogDelay),
           timers(
               secondsAttr<PeerGroup>(
                   kTimersWithdrawUnprogDelay, setTimersWithdrawUnprogDelay))},
          {std::string(kNextHopSelf),
           plain(boolAttr<PeerGroup>(kNextHopSelf, setNextHopSelf))},
      };
  return kHandlers;
}

std::string validAttrList() {
  std::string out;
  for (const auto& [name, _] : attrHandlers()) {
    if (!out.empty()) {
      out += ", ";
    }
    out += name;
  }
  return out;
}

// The group named `groupName`, or end(). Setting an attribute on a
// not-yet-created group implicitly creates it, so command ordering stays
// forgiving; a bare `peer-group <name>` creates one explicitly. Unlike a
// BgpPeer, PeerGroup's only non-optional field is `name`, so a new group needs
// no address seeding.
std::vector<PeerGroup>::iterator findPeerGroup(
    std::vector<PeerGroup>& groups,
    const std::string& groupName) {
  for (auto it = groups.begin(); it != groups.end(); ++it) {
    if (*it->name() == groupName) {
      return it;
    }
  }
  return groups.end();
}

} // namespace

// Parse + validate at construction so queryClient stays a thin dispatch.
// Throwing std::invalid_argument is how the framework surfaces arg parse
// errors (same mechanism as BgpNeighborConfig).
BgpPeerGroupConfig::BgpPeerGroupConfig(std::vector<std::string> v)
    : utils::BaseObjectArgType<std::string>(v) {
  if (v.empty()) {
    throw std::invalid_argument(
        "Error: peer-group <name> is required, followed by an optional "
        "<attribute> <value>");
  }
  if (v[0].empty()) {
    throw std::invalid_argument("Error: peer-group name must not be empty");
  }
  groupName_ = v[0];

  if (v.size() == 1) {
    return; // bare `peer-group <name>`: create the group
  }

  // Attributes may span two tokens (e.g. `timers hold-time`); match the
  // longest prefix of the remaining tokens against the dispatch table.
  const auto& handlers = attrHandlers();
  if (v.size() >= 3) {
    std::string twoToken = v[1] + " " + v[2];
    if (handlers.find(twoToken) != handlers.end()) {
      attr_ = std::move(twoToken);
      values_.assign(v.begin() + 3, v.end());
      return;
    }
  }
  if (handlers.find(v[1]) != handlers.end()) {
    attr_ = v[1];
    values_.assign(v.begin() + 2, v.end());
    return;
  }
  throw std::invalid_argument(
      fmt::format(
          "Error: unknown peer-group attribute '{}'. Valid attributes: {}",
          v[1],
          validAttrList()));
}

CmdConfigProtocolBgpPeerGroupTraits::RetType
CmdConfigProtocolBgpPeerGroup::queryClient(
    const HostInfo& /* hostInfo */,
    const ObjectArgType& args) {
  auto& session = ConfigSession::getInstance();
  auto& cfg = session.getBgpConfig();
  auto& groups = cfg.peer_groups().ensure();
  auto existing = findPeerGroup(groups, args.groupName());

  // Edit a copy: a rejected value (or a struct seeded for it) must leave the
  // in-memory config exactly as it was, since later lookups in the same
  // process see it whether or not it was saved.
  PeerGroup group;
  if (existing != groups.end()) {
    group = *existing;
  } else {
    group.name() = args.groupName();
  }
  Result result = args.attr().empty()
      ? ok(fmt::format(
            "Successfully created BGP peer-group {}", args.groupName()))
      // The attribute is guaranteed valid: BgpPeerGroupConfig's constructor
      // rejects an unknown attribute before we get here.
      : attrHandlers().find(args.attr())->second(cfg, group, args.values());
  if (!result.ok) {
    return result.message;
  }
  if (existing != groups.end()) {
    *existing = std::move(group);
  } else {
    groups.push_back(std::move(group));
  }
  if (!args.attr().empty()) {
    result.message += fmt::format(" for peer-group {}", args.groupName());
  }
  session.saveBgpConfig();
  result.message +=
      fmt::format("\nConfig saved to: {}", session.getBgpSessionConfigPath());
  return result.message;
}

void CmdConfigProtocolBgpPeerGroup::printOutput(const RetType& output) {
  std::cout << output << std::endl;
}

// Explicit template instantiation
template void CmdHandler<
    CmdConfigProtocolBgpPeerGroup,
    CmdConfigProtocolBgpPeerGroupTraits>::run();

} // namespace facebook::fboss
