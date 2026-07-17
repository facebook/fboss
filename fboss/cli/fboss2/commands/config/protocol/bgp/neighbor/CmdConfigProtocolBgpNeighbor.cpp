/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/protocol/bgp/neighbor/CmdConfigProtocolBgpNeighbor.h"

#include "fboss/cli/fboss2/CmdHandler.cpp"

#include <fmt/core.h>
#include <folly/IPAddress.h>
#include <neteng/fboss/bgp/public_tld/configerator/structs/neteng/fboss/bgp/gen-cpp2/bgp_config_types.h>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <optional>
#include <ostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include "configerator/structs/neteng/fboss/bgp/if/gen-cpp2/bgp_attr_types.h"
#include "fboss/cli/fboss2/commands/config/protocol/bgp/BgpCliValueParsers.h"
#include "fboss/cli/fboss2/commands/config/protocol/bgp/BgpPeerAddr.h"
#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/utils/CmdUtilsCommon.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"
#include "fmt/format.h"
#include "thrift/lib/cpp/Thrift.h"

namespace facebook::fboss {

namespace {

// CLI attribute names, exactly as documented (two-token attributes keep their
// space). Kept here (not as raw string literals at the dispatch sites) so the
// valid-attribute set and the handler table stay in sync.
constexpr std::string_view kRemoteAsn = "remote-asn";
constexpr std::string_view kLocalAsn = "local-asn";
constexpr std::string_view kDescription = "description";
constexpr std::string_view kPeerTag = "peer-tag";
constexpr std::string_view kPeerGroup = "peer-group";
constexpr std::string_view kIngressPolicy = "ingress-policy";
constexpr std::string_view kEgressPolicy = "egress-policy";
constexpr std::string_view kRrClient = "rr-client";
constexpr std::string_view kConfedPeer = "confed-peer";
constexpr std::string_view kRedistributePeer = "redistribute-peer";
constexpr std::string_view kEnhancedRouteRefresh = "enhanced-route-refresh";
constexpr std::string_view kConnectMode = "connect-mode";
constexpr std::string_view kPeerPort = "peer-port";
constexpr std::string_view kAddPathSend = "add-path send";
constexpr std::string_view kAddPathReceive = "add-path receive";
constexpr std::string_view kAfiDisableIpv4Afi = "afi disable-ipv4-afi";
constexpr std::string_view kAfiDisableIpv6Afi = "afi disable-ipv6-afi";
constexpr std::string_view kAfiIpv4OverIpv6Nh = "afi ipv4-over-ipv6-nh";
constexpr std::string_view kAfiIpv4LabeledUnicast = "afi ipv4-labeled-unicast";
constexpr std::string_view kAfiIpv6LabeledUnicast = "afi ipv6-labeled-unicast";
constexpr std::string_view kBindAddrAddress = "bind-addr address";
constexpr std::string_view kBindAddrPort = "bind-addr port";
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
// Parity attributes: not in the documented neighbor grammar, but supported by
// the per-attribute peer commands this dispatcher replaces (and by real
// BgpPeer thrift fields), so dropping them would regress existing usage.
constexpr std::string_view kNextHop4 = "next-hop4";
constexpr std::string_view kNextHop6 = "next-hop6";
constexpr std::string_view kNextHopSelf = "next-hop-self";
constexpr std::string_view kPeerId = "peer-id";
constexpr std::string_view kPeerType = "type";
constexpr std::string_view kLinkBandwidth = "link-bandwidth";
constexpr std::string_view kAdvertiseLbw = "advertise-lbw";
constexpr std::string_view kReceiveLbw = "receive-lbw";
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

using Tokens = std::vector<std::string>;
using BgpPeer = bgp::thrift::BgpPeer;
using bgpcli::err;
using bgpcli::ok;
using bgpcli::parseBool;
using bgpcli::parseInt;
using bgpcli::parseNonNegInt32;
using bgpcli::parseNonNegInt64;
using bgpcli::Result;
using facebook::neteng::fboss::bgp_attr::AddPath;
using facebook::neteng::fboss::bgp_attr::AdvertiseLinkBandwidth;
using facebook::neteng::fboss::bgp_attr::ReceiveLinkBandwidth;

// ---- per-attribute handlers ------------------------------------------------
// Each handler mutates the typed bgp::thrift::BgpPeer directly. Field names
// map 1:1 to bgp_config.thrift, so the compiler validates them. Handlers are
// built from the factories below; only attributes with non-trivial parsing
// (connect-mode, add-path) have bespoke functions.

using AttrHandler = std::function<Result(BgpPeer&, const Tokens&)>;

AttrHandler boolAttr(
    std::string_view name,
    std::function<void(BgpPeer&, bool)> set) {
  return [name, set = std::move(set)](
             BgpPeer& peer, const Tokens& values) -> Result {
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
    set(peer, *enable);
    return ok(
        fmt::format(
            "Successfully {} {}", *enable ? "enabled" : "disabled", name));
  };
}

// A single-token string value (policy names, tags, group names).
AttrHandler stringAttr(
    std::string_view name,
    std::string_view valueName,
    std::function<void(BgpPeer&, const std::string&)> set) {
  return [name, valueName, set = std::move(set)](
             BgpPeer& peer, const Tokens& values) -> Result {
    if (values.size() != 1) {
      return err(fmt::format("Error: {} requires <{}>", name, valueName));
    }
    set(peer, values[0]);
    return ok(fmt::format("Successfully set {} to: {}", name, values[0]));
  };
}

// A 4-byte ASN (RFC 6793): unsigned, bounded to [0, 2^32-1].
AttrHandler asnAttr(
    std::string_view name,
    std::function<void(BgpPeer&, int64_t)> set) {
  return [name, set = std::move(set)](
             BgpPeer& peer, const Tokens& values) -> Result {
    if (values.size() != 1) {
      return err(fmt::format("Error: {} requires <asn>", name));
    }
    auto asn = bgpcli::parseAsn4Byte(values[0]);
    if (!asn) {
      return err(
          fmt::format(
              "Error: Invalid {} value '{}'; expected an unsigned 4-byte ASN",
              name,
              values[0]));
    }
    set(peer, *asn);
    return ok(fmt::format("Successfully set {} to: {}", name, *asn));
  };
}

// A validated IP address value, optionally restricted to one family, stored
// normalized.
AttrHandler ipAttr(
    std::string_view name,
    std::function<void(BgpPeer&, const std::string&)> set,
    std::optional<bool> requireV6 = std::nullopt) {
  return [name, set = std::move(set), requireV6](
             BgpPeer& peer, const Tokens& values) -> Result {
    if (values.size() != 1) {
      return err(fmt::format("Error: {} requires <ip-address>", name));
    }
    auto addr = folly::IPAddress::tryFromString(values[0]);
    if (!addr.hasValue()) {
      return err(
          fmt::format("Error: Invalid {} address '{}'", name, values[0]));
    }
    if (requireV6.has_value() && addr->isV6() != *requireV6) {
      return err(
          fmt::format(
              "Error: {} requires an IPv{} address, got '{}'",
              name,
              *requireV6 ? 6 : 4,
              values[0]));
    }
    set(peer, addr->str());
    return ok(fmt::format("Successfully set {} to: {}", name, addr->str()));
  };
}

// A thrift enum attribute. Accepts the enum value name (e.g. BEST_PATH), the
// integer value, or true/false as aliases for 1/0 (the replaced per-attribute
// peer commands took booleans, so existing usage keeps working); membership is
// validated so an unknown mode is rejected rather than persisted.
template <typename EnumT>
AttrHandler enumAttr(
    std::string_view name,
    std::function<void(BgpPeer&, EnumT)> set) {
  return [name, set = std::move(set)](
             BgpPeer& peer, const Tokens& values) -> Result {
    if (values.size() != 1) {
      return err(fmt::format("Error: {} requires <mode>", name));
    }
    EnumT mode;
    bool valid = false;
    if (auto parsed = parseInt<int32_t>(values[0])) {
      mode = static_cast<EnumT>(*parsed);
      valid = apache::thrift::TEnumTraits<EnumT>::findName(mode) != nullptr;
    } else if (auto enable = parseBool(values[0])) {
      mode = static_cast<EnumT>(*enable ? 1 : 0);
      valid = true;
    } else {
      valid = apache::thrift::TEnumTraits<EnumT>::findValue(
          values[0].c_str(), &mode);
    }
    if (!valid) {
      std::string names;
      for (auto n : apache::thrift::TEnumTraits<EnumT>::names) {
        names += names.empty() ? "" : ", ";
        names += std::string(n);
      }
      return err(
          fmt::format(
              "Error: {} value '{}' is not a valid mode; expected one of: {}",
              name,
              values[0],
              names));
    }
    set(peer, mode);
    return ok(
        fmt::format(
            "Successfully set {} to: {}", name, static_cast<int32_t>(mode)));
  };
}

// A non-negative int32 seconds value (timers).
AttrHandler secondsAttr(
    std::string_view name,
    std::function<void(BgpPeer&, int32_t)> set) {
  return [name, set = std::move(set)](
             BgpPeer& peer, const Tokens& values) -> Result {
    if (values.size() != 1) {
      return err(fmt::format("Error: {} requires <seconds>", name));
    }
    auto seconds = parseNonNegInt32(values[0]);
    if (!seconds) {
      return err(
          fmt::format(
              "Error: {} must be a non-negative integer that fits in int32, "
              "got '{}'",
              name,
              values[0]));
    }
    set(peer, *seconds);
    return ok(
        fmt::format("Successfully set {} to: {} seconds", name, *seconds));
  };
}

// A non-negative int64 route-count value (RouteLimit fields).
AttrHandler routeCountAttr(
    std::string_view name,
    std::function<void(BgpPeer&, int64_t)> set) {
  return [name, set = std::move(set)](
             BgpPeer& peer, const Tokens& values) -> Result {
    if (values.size() != 1) {
      return err(fmt::format("Error: {} requires <value>", name));
    }
    auto limit = parseNonNegInt64(values[0]);
    if (!limit) {
      return err(
          fmt::format(
              "Error: {} must be a non-negative integer, got '{}'",
              name,
              values[0]));
    }
    set(peer, *limit);
    return ok(fmt::format("Successfully set {} to: {}", name, *limit));
  };
}

// An attribute the CLI must refuse: either bgp_config.thrift has no field for
// it (persisting it would stage dead config the daemon ignores) or the knob is
// not per-neighbor. The reason is surfaced to the user instead.
AttrHandler rejectedAttr(std::string_view name, std::string_view reason) {
  return [name, reason](
             BgpPeer& /* peer */, const Tokens& /* values */) -> Result {
    return err(fmt::format("Error: {} is not supported: {}", name, reason));
  };
}

Result applyDescription(BgpPeer& peer, const Tokens& values) {
  if (values.empty()) {
    return err(fmt::format("Error: {} requires <string>", kDescription));
  }
  // The description may span multiple CLI tokens; re-join them.
  std::string description = values[0];
  for (size_t i = 1; i < values.size(); i++) {
    description += " " + values[i];
  }
  peer.description() = description;
  return ok(
      fmt::format("Successfully set {} to: {}", kDescription, description));
}

Result applyConnectMode(BgpPeer& peer, const Tokens& values) {
  // The thrift model only has is_passive (listen vs actively connect); there
  // is no representation for BOTH, so it is rejected rather than silently
  // mapped.
  if (values.size() != 1) {
    return err(
        fmt::format(
            "Error: {} requires <{}|{}>",
            kConnectMode,
            kConnectModePassive,
            kConnectModeActive));
  }
  if (values[0] == kConnectModePassive) {
    peer.is_passive() = true;
  } else if (values[0] == kConnectModeActive) {
    peer.is_passive() = false;
  } else if (values[0] == kConnectModeBoth) {
    return err(
        fmt::format(
            "Error: {} {} has no representation in bgp_config.thrift "
            "(BgpPeer only models is_passive); use {} or {}",
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

// add_path is a single enum whose values form a bitmask by design:
// RECEIVE=1, SEND=2, BOTH=3(=RECEIVE|SEND). Merge the requested direction
// into the current value; clearing the last direction unsets the field.
Result applyAddPath(BgpPeer& peer, bool send, const Tokens& values) {
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
  int bits = peer.add_path() ? static_cast<int>(*peer.add_path()) : 0;
  const int bit = send ? static_cast<int>(AddPath::SEND)
                       : static_cast<int>(AddPath::RECEIVE);
  bits = *enable ? (bits | bit) : (bits & ~bit);
  if (bits == 0) {
    peer.add_path().reset();
  } else {
    peer.add_path() = static_cast<AddPath>(bits);
  }
  return ok(
      fmt::format(
          "Successfully {} {}", *enable ? "enabled" : "disabled", name));
}

const std::map<std::string, AttrHandler, std::less<>>& attrHandlers() {
  static const std::map<std::string, AttrHandler, std::less<>> kHandlers = {
      {std::string(kRemoteAsn),
       asnAttr(
           kRemoteAsn,
           [](BgpPeer& p, int64_t v) { p.remote_as_4_byte() = v; })},
      {std::string(kLocalAsn),
       asnAttr(
           kLocalAsn, [](BgpPeer& p, int64_t v) { p.local_as_4_byte() = v; })},
      {std::string(kDescription), applyDescription},
      {std::string(kPeerTag),
       stringAttr(
           kPeerTag,
           "string",
           [](BgpPeer& p, const std::string& v) { p.peer_tag() = v; })},
      {std::string(kPeerGroup),
       stringAttr(
           kPeerGroup,
           "name",
           [](BgpPeer& p, const std::string& v) { p.peer_group_name() = v; })},
      {std::string(kIngressPolicy),
       stringAttr(
           kIngressPolicy,
           "policy-name",
           [](BgpPeer& p, const std::string& v) {
             p.ingress_policy_name() = v;
           })},
      {std::string(kEgressPolicy),
       stringAttr(
           kEgressPolicy,
           "policy-name",
           [](BgpPeer& p, const std::string& v) {
             p.egress_policy_name() = v;
           })},
      {std::string(kRrClient),
       boolAttr(kRrClient, [](BgpPeer& p, bool v) { p.is_rr_client() = v; })},
      {std::string(kConfedPeer),
       boolAttr(
           kConfedPeer, [](BgpPeer& p, bool v) { p.is_confed_peer() = v; })},
      {std::string(kRedistributePeer),
       boolAttr(
           kRedistributePeer,
           [](BgpPeer& p, bool v) { p.is_redistribute_peer() = v; })},
      {std::string(kEnhancedRouteRefresh),
       boolAttr(
           kEnhancedRouteRefresh,
           [](BgpPeer& p, bool v) { p.enhanced_route_refresh() = v; })},
      {std::string(kConnectMode), applyConnectMode},
      {std::string(kPeerPort),
       rejectedAttr(
           kPeerPort,
           "BgpPeer has no per-neighbor port field; the daemon listens on "
           "the global listen_port")},
      {std::string(kAddPathSend),
       [](BgpPeer& p, const Tokens& v) { return applyAddPath(p, true, v); }},
      {std::string(kAddPathReceive),
       [](BgpPeer& p, const Tokens& v) { return applyAddPath(p, false, v); }},
      {std::string(kAfiDisableIpv4Afi),
       boolAttr(
           kAfiDisableIpv4Afi,
           [](BgpPeer& p, bool v) { p.disable_ipv4_afi() = v; })},
      {std::string(kAfiDisableIpv6Afi),
       boolAttr(
           kAfiDisableIpv6Afi,
           [](BgpPeer& p, bool v) { p.disable_ipv6_afi() = v; })},
      {std::string(kAfiIpv4OverIpv6Nh),
       boolAttr(
           kAfiIpv4OverIpv6Nh,
           [](BgpPeer& p, bool v) { p.v4_over_v6_nexthop() = v; })},
      {std::string(kAfiIpv4LabeledUnicast),
       rejectedAttr(
           kAfiIpv4LabeledUnicast,
           "bgp_config.thrift has no labeled-unicast field on BgpPeer")},
      {std::string(kAfiIpv6LabeledUnicast),
       rejectedAttr(
           kAfiIpv6LabeledUnicast,
           "bgp_config.thrift has no labeled-unicast field on BgpPeer")},
      {std::string(kBindAddrAddress),
       ipAttr(
           kBindAddrAddress,
           [](BgpPeer& p, const std::string& v) { p.local_addr() = v; })},
      {std::string(kBindAddrPort),
       rejectedAttr(
           kBindAddrPort,
           "BgpPeer has no per-neighbor port field; the daemon listens on "
           "the global listen_port")},
      {std::string(kGracefulRestartTime),
       secondsAttr(
           kGracefulRestartTime,
           [](BgpPeer& p, int32_t v) {
             p.bgp_peer_timers().ensure().graceful_restart_seconds() = v;
           })},
      {std::string(kGracefulRestartStatefulHa),
       boolAttr(
           kGracefulRestartStatefulHa,
           [](BgpPeer& p, bool v) { p.enable_stateful_ha() = v; })},
      {std::string(kMaxRoutePreFilter),
       routeCountAttr(
           kMaxRoutePreFilter,
           [](BgpPeer& p, int64_t v) {
             p.pre_filter().ensure().max_routes() = v;
           })},
      {std::string(kMaxRoutePostFilter),
       routeCountAttr(
           kMaxRoutePostFilter,
           [](BgpPeer& p, int64_t v) {
             p.post_filter().ensure().max_routes() = v;
           })},
      // RouteLimit.warning_limit is an absolute route count in the thrift
      // schema, not a percentage.
      {std::string(kMaxRoutePostWarningThreshold),
       routeCountAttr(
           kMaxRoutePostWarningThreshold,
           [](BgpPeer& p, int64_t v) {
             p.post_filter().ensure().warning_limit() = v;
           })},
      {std::string(kTimersHoldTime),
       secondsAttr(
           kTimersHoldTime,
           [](BgpPeer& p, int32_t v) {
             p.bgp_peer_timers().ensure().hold_time_seconds() = v;
           })},
      {std::string(kTimersKeepalive),
       secondsAttr(
           kTimersKeepalive,
           [](BgpPeer& p, int32_t v) {
             p.bgp_peer_timers().ensure().keep_alive_seconds() = v;
           })},
      {std::string(kTimersOutDelay),
       secondsAttr(
           kTimersOutDelay,
           [](BgpPeer& p, int32_t v) {
             p.bgp_peer_timers().ensure().out_delay_seconds() = v;
           })},
      {std::string(kTimersWithdrawUnprogDelay),
       secondsAttr(
           kTimersWithdrawUnprogDelay,
           [](BgpPeer& p, int32_t v) {
             p.bgp_peer_timers().ensure().withdraw_unprog_delay_seconds() = v;
           })},
      {std::string(kNextHop4),
       ipAttr(
           kNextHop4,
           [](BgpPeer& p, const std::string& v) { p.next_hop4() = v; },
           /* requireV6 */ false)},
      {std::string(kNextHop6),
       ipAttr(
           kNextHop6,
           [](BgpPeer& p, const std::string& v) { p.next_hop6() = v; },
           /* requireV6 */ true)},
      {std::string(kNextHopSelf),
       boolAttr(
           kNextHopSelf, [](BgpPeer& p, bool v) { p.next_hop_self() = v; })},
      {std::string(kPeerId),
       stringAttr(
           kPeerId,
           "string",
           [](BgpPeer& p, const std::string& v) { p.peer_id() = v; })},
      {std::string(kPeerType),
       stringAttr(
           kPeerType,
           "string",
           [](BgpPeer& p, const std::string& v) { p.type() = v; })},
      // Bits (not bytes) per second, with optional K/M/G suffix (e.g. "100G"),
      // validated so garbage never reaches bgpd's config parser.
      {std::string(kLinkBandwidth),
       [](BgpPeer& p, const Tokens& values) -> Result {
         if (values.size() != 1) {
           return err(
               fmt::format(
                   "Error: {} requires <bits-per-second>[K|M|G]",
                   kLinkBandwidth));
         }
         const std::string& v = values[0];
         const size_t digits =
             v.find_first_not_of("0123456789") == std::string::npos
             ? v.size()
             : v.find_first_not_of("0123456789");
         const bool valid = digits > 0 &&
             (digits == v.size() ||
              (digits == v.size() - 1 &&
               (v.back() == 'K' || v.back() == 'M' || v.back() == 'G')));
         if (!valid) {
           return err(
               fmt::format(
                   "Error: Invalid {} value '{}'; expected digits with an "
                   "optional K/M/G suffix (e.g. 100G)",
                   kLinkBandwidth,
                   v));
         }
         p.link_bandwidth_bps() = v;
         return ok(
             fmt::format("Successfully set {} to: {}", kLinkBandwidth, v));
       }},
      {std::string(kAdvertiseLbw),
       enumAttr<AdvertiseLinkBandwidth>(
           kAdvertiseLbw,
           [](BgpPeer& p, AdvertiseLinkBandwidth v) {
             p.advertise_link_bandwidth() = v;
           })},
      {std::string(kReceiveLbw),
       enumAttr<ReceiveLinkBandwidth>(
           kReceiveLbw,
           [](BgpPeer& p, ReceiveLinkBandwidth v) {
             p.receive_link_bandwidth() = v;
           })},
      {std::string(kMaxRoutePreWarningThreshold),
       routeCountAttr(
           kMaxRoutePreWarningThreshold,
           [](BgpPeer& p, int64_t v) {
             p.pre_filter().ensure().warning_limit() = v;
           })},
      {std::string(kMaxRoutePreWarningOnly),
       boolAttr(
           kMaxRoutePreWarningOnly,
           [](BgpPeer& p, bool v) {
             p.pre_filter().ensure().warning_only() = v;
           })},
      {std::string(kMaxRoutePostWarningOnly),
       boolAttr(
           kMaxRoutePostWarningOnly,
           [](BgpPeer& p, bool v) {
             p.post_filter().ensure().warning_only() = v;
           })},
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

// Find the peer keyed by (normalized) peer_addr, creating it if absent.
// Setting an attribute on a not-yet-created neighbor implicitly creates it,
// so command ordering stays forgiving; a bare `neighbor <ip>` creates one
// explicitly.
bgp::thrift::BgpPeer& findOrCreatePeer(
    bgp::thrift::BgpConfig& cfg,
    const std::string& peerAddr) {
  auto& peers = *cfg.peers();
  if (auto it = bgpcli::findBgpPeer(cfg, peerAddr); it != peers.end()) {
    return *it;
  }
  peers.emplace_back();
  auto& peer = peers.back();
  peer.peer_addr() = peerAddr;
  // local_addr / next_hop4 / next_hop6 are non-optional thrift fields that
  // bgpd parses with folly::IPAddress at config load — an empty string makes
  // the daemon abort (IPAddressFormatException) before serving any config.
  // Seed them with the unspecified address of the right family; the user
  // overrides them via `bind-addr address` / `next-hop4` / `next-hop6`.
  const bool isV6 = peerAddr.find(':') != std::string::npos;
  peer.local_addr() = isV6 ? "::" : "0.0.0.0";
  peer.next_hop4() = "0.0.0.0";
  peer.next_hop6() = "::";
  return peer;
}

} // namespace

// Parse + validate at construction so queryClient stays a thin dispatch.
// Throwing std::invalid_argument is how the framework surfaces arg parse
// errors (same mechanism as BgpGlobalConfig).
BgpNeighborConfig::BgpNeighborConfig(std::vector<std::string> v)
    : utils::BaseObjectArgType<std::string>(v) {
  if (v.empty()) {
    throw std::invalid_argument(
        "Error: neighbor <ip-address> is required, followed by an optional "
        "<attribute> <value>");
  }
  auto normalized = bgpcli::normalizeBgpPeerAddr(v[0]);
  if (!normalized) {
    throw std::invalid_argument(
        fmt::format("Error: Invalid neighbor address '{}'", v[0]));
  }
  peerAddr_ = std::move(*normalized);

  if (v.size() == 1) {
    return; // bare `neighbor <ip>`: create the peer
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
          "Error: unknown neighbor attribute '{}'. Valid attributes: {}",
          v[1],
          validAttrList()));
}

CmdConfigProtocolBgpNeighborTraits::RetType
CmdConfigProtocolBgpNeighbor::queryClient(
    const HostInfo& /* hostInfo */,
    const ObjectArgType& args) {
  auto& session = ConfigSession::getInstance();
  auto& cfg = session.getBgpConfig();
  const bool created =
      bgpcli::findBgpPeer(cfg, args.peerAddr()) == cfg.peers()->end();
  auto& peer = findOrCreatePeer(cfg, args.peerAddr());

  Result result = args.attr().empty()
      ? ok(fmt::format("Successfully created BGP neighbor {}", args.peerAddr()))
      // The attribute is guaranteed valid: BgpNeighborConfig's constructor
      // rejects an unknown attribute before we get here.
      : attrHandlers().find(args.attr())->second(peer, args.values());
  if (result.ok) {
    if (!args.attr().empty()) {
      result.message += fmt::format(" for neighbor {}", args.peerAddr());
    }
    session.saveBgpConfig();
    result.message +=
        fmt::format("\nConfig saved to: {}", session.getBgpSessionConfigPath());
  } else if (created) {
    // A rejected value must not leave a half-created peer in the in-memory
    // config (visible to later lookups in the same process, e.g. tests).
    cfg.peers()->pop_back();
  }
  return result.message;
}

void CmdConfigProtocolBgpNeighbor::printOutput(const RetType& output) {
  std::cout << output << std::endl;
}

// Explicit template instantiation
template void CmdHandler<
    CmdConfigProtocolBgpNeighbor,
    CmdConfigProtocolBgpNeighborTraits>::run();

} // namespace facebook::fboss
