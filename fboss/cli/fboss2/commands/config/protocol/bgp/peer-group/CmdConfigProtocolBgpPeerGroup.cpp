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
constexpr std::string_view kConnectMode = "connect-mode";
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

// connect-mode values
constexpr std::string_view kConnectModePassive = "PASSIVE";
constexpr std::string_view kConnectModeActive = "ACTIVE";
constexpr std::string_view kConnectModeBoth = "BOTH";

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

// ---- hand-written handlers -------------------------------------------------
// Only for value shapes no factory covers, because the shape is unique to this
// attribute rather than reusable. Both mirror the neighbor dispatcher's
// versions, which write the identically-named BgpPeer fields.

// connect-mode: the thrift model only has is_passive (listen vs actively
// connect), so this is not an enum mapping — BOTH has no representation and is
// rejected with an explanation rather than silently mapped.
Result connectMode(PeerGroup& group, const Tokens& values) {
  if (values.size() != 1) {
    return err(
        fmt::format(
            "Error: {} requires <{}|{}>",
            kConnectMode,
            kConnectModePassive,
            kConnectModeActive));
  }
  if (values[0] == kConnectModePassive) {
    group.is_passive() = true;
  } else if (values[0] == kConnectModeActive) {
    group.is_passive() = false;
  } else if (values[0] == kConnectModeBoth) {
    return err(
        fmt::format(
            "Error: {} {} has no representation in bgp_config.thrift "
            "(PeerGroup only models is_passive); use {} or {}",
            kConnectMode,
            kConnectModeBoth,
            kConnectModePassive,
            kConnectModeActive));
  } else {
    return err(
        fmt::format(
            "Error: Invalid {} value '{}'; expected {} or {}",
            kConnectMode,
            values[0],
            kConnectModePassive,
            kConnectModeActive));
  }
  return ok(fmt::format("Successfully set {} to: {}", kConnectMode, values[0]));
}

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

const std::map<std::string, AttrHandler<PeerGroup>, std::less<>>&
attrHandlers() {
  static const std::map<std::string, AttrHandler<PeerGroup>, std::less<>>
      kHandlers = {
          {std::string(kRemoteAsn),
           asnAttr<PeerGroup>(kRemoteAsn, setRemoteAsn)},
          {std::string(kLocalAsn), asnAttr<PeerGroup>(kLocalAsn, setLocalAsn)},
          {std::string(kDescription),
           joinedStringAttr<PeerGroup>(kDescription, setDescription)},
          {std::string(kPeerTag),
           stringAttr<PeerGroup>(kPeerTag, "string", setPeerTag)},
          {std::string(kIngressPolicy),
           stringAttr<PeerGroup>(
               kIngressPolicy, "policy-name", setIngressPolicy)},
          {std::string(kEgressPolicy),
           stringAttr<PeerGroup>(
               kEgressPolicy, "policy-name", setEgressPolicy)},
          {std::string(kRrClient), boolAttr<PeerGroup>(kRrClient, setRrClient)},
          {std::string(kConfedPeer),
           boolAttr<PeerGroup>(kConfedPeer, setConfedPeer)},
          {std::string(kRedistributePeer),
           boolAttr<PeerGroup>(kRedistributePeer, setRedistributePeer)},
          {std::string(kEnhancedRouteRefresh),
           boolAttr<PeerGroup>(kEnhancedRouteRefresh, setEnhancedRouteRefresh)},
          {std::string(kConnectMode), connectMode},
          {std::string(kAddPathSend), addPathSend},
          {std::string(kAddPathReceive), addPathReceive},
          {std::string(kAfiDisableIpv4Afi),
           boolAttr<PeerGroup>(kAfiDisableIpv4Afi, setDisableIpv4Afi)},
          {std::string(kAfiDisableIpv6Afi),
           boolAttr<PeerGroup>(kAfiDisableIpv6Afi, setDisableIpv6Afi)},
          {std::string(kAfiIpv4OverIpv6Nh),
           boolAttr<PeerGroup>(kAfiIpv4OverIpv6Nh, setIpv4OverIpv6Nh)},
          {std::string(kGracefulRestartTime),
           secondsAttr<PeerGroup>(
               kGracefulRestartTime, setGracefulRestartTime)},
          {std::string(kGracefulRestartStatefulHa),
           boolAttr<PeerGroup>(
               kGracefulRestartStatefulHa, setGracefulRestartStatefulHa)},
          {std::string(kMaxRoutePreFilter),
           routeCountAttr<PeerGroup>(kMaxRoutePreFilter, setMaxRoutePreFilter)},
          {std::string(kMaxRoutePostFilter),
           routeCountAttr<PeerGroup>(
               kMaxRoutePostFilter, setMaxRoutePostFilter)},
          {std::string(kMaxRoutePreWarningThreshold),
           routeCountAttr<PeerGroup>(
               kMaxRoutePreWarningThreshold, setMaxRoutePreWarningThreshold)},
          {std::string(kMaxRoutePostWarningThreshold),
           routeCountAttr<PeerGroup>(
               kMaxRoutePostWarningThreshold, setMaxRoutePostWarningThreshold)},
          {std::string(kMaxRoutePreWarningOnly),
           boolAttr<PeerGroup>(
               kMaxRoutePreWarningOnly, setMaxRoutePreWarningOnly)},
          {std::string(kMaxRoutePostWarningOnly),
           boolAttr<PeerGroup>(
               kMaxRoutePostWarningOnly, setMaxRoutePostWarningOnly)},
          {std::string(kTimersHoldTime),
           secondsAttr<PeerGroup>(kTimersHoldTime, setTimersHoldTime)},
          {std::string(kTimersKeepalive),
           secondsAttr<PeerGroup>(kTimersKeepalive, setTimersKeepalive)},
          {std::string(kTimersOutDelay),
           secondsAttr<PeerGroup>(kTimersOutDelay, setTimersOutDelay)},
          {std::string(kTimersWithdrawUnprogDelay),
           secondsAttr<PeerGroup>(
               kTimersWithdrawUnprogDelay, setTimersWithdrawUnprogDelay)},
          {std::string(kNextHopSelf),
           boolAttr<PeerGroup>(kNextHopSelf, setNextHopSelf)},
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

// Find the peer-group keyed by name, creating it if absent. Setting an
// attribute on a not-yet-created group implicitly creates it, so command
// ordering stays forgiving; a bare `peer-group <name>` creates one explicitly.
// Unlike a BgpPeer, PeerGroup's only non-optional field is `name`, so no
// address seeding is needed.
bgp::thrift::PeerGroup& findOrCreatePeerGroup(
    bgp::thrift::BgpConfig& cfg,
    const std::string& groupName) {
  auto& groups = cfg.peer_groups().ensure();
  for (auto& group : groups) {
    if (*group.name() == groupName) {
      return group;
    }
  }
  groups.emplace_back();
  auto& group = groups.back();
  group.name() = groupName;
  return group;
}

// Whether a peer-group with the given name already exists.
bool peerGroupExists(
    const bgp::thrift::BgpConfig& cfg,
    const std::string& groupName) {
  if (!cfg.peer_groups().has_value()) {
    return false;
  }
  for (const auto& group : *cfg.peer_groups()) {
    if (*group.name() == groupName) {
      return true;
    }
  }
  return false;
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
  const bool created = !peerGroupExists(cfg, args.groupName());
  auto& group = findOrCreatePeerGroup(cfg, args.groupName());

  Result result = args.attr().empty()
      ? ok(fmt::format(
            "Successfully created BGP peer-group {}", args.groupName()))
      // The attribute is guaranteed valid: BgpPeerGroupConfig's constructor
      // rejects an unknown attribute before we get here.
      : attrHandlers().find(args.attr())->second(group, args.values());
  if (result.ok) {
    if (!args.attr().empty()) {
      result.message += fmt::format(" for peer-group {}", args.groupName());
    }
    session.saveBgpConfig();
    result.message +=
        fmt::format("\nConfig saved to: {}", session.getBgpSessionConfigPath());
  } else if (created) {
    // A rejected value must not leave a half-created group in the in-memory
    // config (visible to later lookups in the same process, e.g. tests).
    cfg.peer_groups()->pop_back();
  }
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
