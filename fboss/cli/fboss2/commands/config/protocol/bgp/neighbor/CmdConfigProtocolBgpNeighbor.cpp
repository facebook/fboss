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
#include "fboss/cli/fboss2/commands/config/protocol/bgp/BgpCliAttrHandlers.h"
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

using bgpcli::Tokens;
using BgpPeer = bgp::thrift::BgpPeer;
using bgpcli::asnAttr;
using bgpcli::AttrHandler;
using bgpcli::bitRateAttr;
using bgpcli::boolAttr;
using bgpcli::err;
using bgpcli::ipAttr;
using bgpcli::joinedStringAttr;
using bgpcli::ok;
using bgpcli::parseBool;
using bgpcli::rejectedAttr;
using bgpcli::Result;
using bgpcli::routeCountAttr;
using bgpcli::secondsAttr;
using bgpcli::stringAttr;
using bgpcli::thriftEnumAttr;
using facebook::neteng::fboss::bgp_attr::AddPath;
using facebook::neteng::fboss::bgp_attr::AdvertiseLinkBandwidth;
using facebook::neteng::fboss::bgp_attr::ReceiveLinkBandwidth;

// ---- per-attribute setters -------------------------------------------------
// Pure thrift assignment: no validation, no messages. Parsing, bounds and the
// user-facing text all live in the shared factories in BgpCliAttrHandlers.h,
// so a setter is just the field this attribute writes. Field names map 1:1 to
// bgp_config.thrift, so the compiler validates them.

void setRemoteAsn(BgpPeer& p, int64_t v) {
  p.remote_as_4_byte() = v;
}

void setLocalAsn(BgpPeer& p, int64_t v) {
  p.local_as_4_byte() = v;
}

void setDescription(BgpPeer& p, const std::string& v) {
  p.description() = v;
}

void setPeerTag(BgpPeer& p, const std::string& v) {
  p.peer_tag() = v;
}

void setPeerGroup(BgpPeer& p, const std::string& v) {
  p.peer_group_name() = v;
}

void setIngressPolicy(BgpPeer& p, const std::string& v) {
  p.ingress_policy_name() = v;
}

void setEgressPolicy(BgpPeer& p, const std::string& v) {
  p.egress_policy_name() = v;
}

void setRrClient(BgpPeer& p, bool v) {
  p.is_rr_client() = v;
}

void setConfedPeer(BgpPeer& p, bool v) {
  p.is_confed_peer() = v;
}

void setRedistributePeer(BgpPeer& p, bool v) {
  p.is_redistribute_peer() = v;
}

void setEnhancedRouteRefresh(BgpPeer& p, bool v) {
  p.enhanced_route_refresh() = v;
}

void setDisableIpv4Afi(BgpPeer& p, bool v) {
  p.disable_ipv4_afi() = v;
}

void setDisableIpv6Afi(BgpPeer& p, bool v) {
  p.disable_ipv6_afi() = v;
}

void setIpv4OverIpv6Nh(BgpPeer& p, bool v) {
  p.v4_over_v6_nexthop() = v;
}

void setBindAddrAddress(BgpPeer& p, const std::string& v) {
  p.local_addr() = v;
}

void setGracefulRestartTime(BgpPeer& p, int32_t v) {
  p.bgp_peer_timers().ensure().graceful_restart_seconds() = v;
}

void setGracefulRestartStatefulHa(BgpPeer& p, bool v) {
  p.enable_stateful_ha() = v;
}

void setMaxRoutePreFilter(BgpPeer& p, int64_t v) {
  p.pre_filter().ensure().max_routes() = v;
}

void setMaxRoutePostFilter(BgpPeer& p, int64_t v) {
  p.post_filter().ensure().max_routes() = v;
}

// RouteLimit.warning_limit is an absolute route count in the thrift schema,
// not a percentage.
void setMaxRoutePreWarningThreshold(BgpPeer& p, int64_t v) {
  p.pre_filter().ensure().warning_limit() = v;
}

void setMaxRoutePostWarningThreshold(BgpPeer& p, int64_t v) {
  p.post_filter().ensure().warning_limit() = v;
}

void setMaxRoutePreWarningOnly(BgpPeer& p, bool v) {
  p.pre_filter().ensure().warning_only() = v;
}

void setMaxRoutePostWarningOnly(BgpPeer& p, bool v) {
  p.post_filter().ensure().warning_only() = v;
}

void setTimersHoldTime(BgpPeer& p, int32_t v) {
  p.bgp_peer_timers().ensure().hold_time_seconds() = v;
}

void setTimersKeepalive(BgpPeer& p, int32_t v) {
  p.bgp_peer_timers().ensure().keep_alive_seconds() = v;
}

void setTimersOutDelay(BgpPeer& p, int32_t v) {
  p.bgp_peer_timers().ensure().out_delay_seconds() = v;
}

void setTimersWithdrawUnprogDelay(BgpPeer& p, int32_t v) {
  p.bgp_peer_timers().ensure().withdraw_unprog_delay_seconds() = v;
}

void setNextHop4(BgpPeer& p, const std::string& v) {
  p.next_hop4() = v;
}

void setNextHop6(BgpPeer& p, const std::string& v) {
  p.next_hop6() = v;
}

void setNextHopSelf(BgpPeer& p, bool v) {
  p.next_hop_self() = v;
}

void setPeerId(BgpPeer& p, const std::string& v) {
  p.peer_id() = v;
}

void setPeerType(BgpPeer& p, const std::string& v) {
  p.type() = v;
}

void setLinkBandwidth(BgpPeer& p, const std::string& v) {
  p.link_bandwidth_bps() = v;
}

void setAdvertiseLbw(BgpPeer& p, AdvertiseLinkBandwidth v) {
  p.advertise_link_bandwidth() = v;
}

void setReceiveLbw(BgpPeer& p, ReceiveLinkBandwidth v) {
  p.receive_link_bandwidth() = v;
}

// ---- hand-written handlers -------------------------------------------------
// Only for value shapes no factory covers, because the shape is unique to this
// attribute rather than reusable.

// connect-mode: the thrift model only has is_passive (listen vs actively
// connect), so this is not an enum mapping — BOTH has no representation and is
// rejected with an explanation rather than silently mapped.
Result connectMode(BgpPeer& peer, const Tokens& values) {
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

// add-path send/receive: a boolean at the CLI, but the two attributes share
// one thrift field — add_path is a single enum whose values form a bitmask by
// design (RECEIVE=1, SEND=2, BOTH=3). Each takes the boolAttr text but merges
// its direction into the current value instead of overwriting it; clearing the
// last direction unsets the field.
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

Result addPathSend(BgpPeer& peer, const Tokens& values) {
  return applyAddPath(peer, /* send */ true, values);
}

Result addPathReceive(BgpPeer& peer, const Tokens& values) {
  return applyAddPath(peer, /* send */ false, values);
}

// ---- the dispatch table ----------------------------------------------------
// One line per attribute: dispatch key, value shape, setter. There are no
// handler bodies here by design — if an attribute appears to need one, its
// value shape is missing a factory and the fix is to add the factory.

constexpr std::string_view kNoPeerPortField =
    "BgpPeer has no per-neighbor port field; the daemon listens on the global "
    "listen_port";
constexpr std::string_view kNoLabeledUnicastField =
    "bgp_config.thrift has no labeled-unicast field on BgpPeer";

const std::map<std::string, AttrHandler<BgpPeer>, std::less<>>& attrHandlers() {
  static const std::map<std::string, AttrHandler<BgpPeer>, std::less<>>
      kHandlers = {
          {std::string(kRemoteAsn), asnAttr<BgpPeer>(kRemoteAsn, setRemoteAsn)},
          {std::string(kLocalAsn), asnAttr<BgpPeer>(kLocalAsn, setLocalAsn)},
          {std::string(kDescription),
           joinedStringAttr<BgpPeer>(kDescription, setDescription)},
          {std::string(kPeerTag),
           stringAttr<BgpPeer>(kPeerTag, "string", setPeerTag)},
          {std::string(kPeerGroup),
           stringAttr<BgpPeer>(kPeerGroup, "name", setPeerGroup)},
          {std::string(kIngressPolicy),
           stringAttr<BgpPeer>(
               kIngressPolicy, "policy-name", setIngressPolicy)},
          {std::string(kEgressPolicy),
           stringAttr<BgpPeer>(kEgressPolicy, "policy-name", setEgressPolicy)},
          {std::string(kRrClient), boolAttr<BgpPeer>(kRrClient, setRrClient)},
          {std::string(kConfedPeer),
           boolAttr<BgpPeer>(kConfedPeer, setConfedPeer)},
          {std::string(kRedistributePeer),
           boolAttr<BgpPeer>(kRedistributePeer, setRedistributePeer)},
          {std::string(kEnhancedRouteRefresh),
           boolAttr<BgpPeer>(kEnhancedRouteRefresh, setEnhancedRouteRefresh)},
          {std::string(kConnectMode), connectMode},
          {std::string(kPeerPort),
           rejectedAttr<BgpPeer>(kPeerPort, kNoPeerPortField)},
          {std::string(kAddPathSend), addPathSend},
          {std::string(kAddPathReceive), addPathReceive},
          {std::string(kAfiDisableIpv4Afi),
           boolAttr<BgpPeer>(kAfiDisableIpv4Afi, setDisableIpv4Afi)},
          {std::string(kAfiDisableIpv6Afi),
           boolAttr<BgpPeer>(kAfiDisableIpv6Afi, setDisableIpv6Afi)},
          {std::string(kAfiIpv4OverIpv6Nh),
           boolAttr<BgpPeer>(kAfiIpv4OverIpv6Nh, setIpv4OverIpv6Nh)},
          {std::string(kAfiIpv4LabeledUnicast),
           rejectedAttr<BgpPeer>(
               kAfiIpv4LabeledUnicast, kNoLabeledUnicastField)},
          {std::string(kAfiIpv6LabeledUnicast),
           rejectedAttr<BgpPeer>(
               kAfiIpv6LabeledUnicast, kNoLabeledUnicastField)},
          {std::string(kBindAddrAddress),
           ipAttr<BgpPeer>(kBindAddrAddress, setBindAddrAddress)},
          {std::string(kBindAddrPort),
           rejectedAttr<BgpPeer>(kBindAddrPort, kNoPeerPortField)},
          {std::string(kGracefulRestartTime),
           secondsAttr<BgpPeer>(kGracefulRestartTime, setGracefulRestartTime)},
          {std::string(kGracefulRestartStatefulHa),
           boolAttr<BgpPeer>(
               kGracefulRestartStatefulHa, setGracefulRestartStatefulHa)},
          {std::string(kMaxRoutePreFilter),
           routeCountAttr<BgpPeer>(kMaxRoutePreFilter, setMaxRoutePreFilter)},
          {std::string(kMaxRoutePostFilter),
           routeCountAttr<BgpPeer>(kMaxRoutePostFilter, setMaxRoutePostFilter)},
          {std::string(kMaxRoutePreWarningThreshold),
           routeCountAttr<BgpPeer>(
               kMaxRoutePreWarningThreshold, setMaxRoutePreWarningThreshold)},
          {std::string(kMaxRoutePostWarningThreshold),
           routeCountAttr<BgpPeer>(
               kMaxRoutePostWarningThreshold, setMaxRoutePostWarningThreshold)},
          {std::string(kMaxRoutePreWarningOnly),
           boolAttr<BgpPeer>(
               kMaxRoutePreWarningOnly, setMaxRoutePreWarningOnly)},
          {std::string(kMaxRoutePostWarningOnly),
           boolAttr<BgpPeer>(
               kMaxRoutePostWarningOnly, setMaxRoutePostWarningOnly)},
          {std::string(kTimersHoldTime),
           secondsAttr<BgpPeer>(kTimersHoldTime, setTimersHoldTime)},
          {std::string(kTimersKeepalive),
           secondsAttr<BgpPeer>(kTimersKeepalive, setTimersKeepalive)},
          {std::string(kTimersOutDelay),
           secondsAttr<BgpPeer>(kTimersOutDelay, setTimersOutDelay)},
          {std::string(kTimersWithdrawUnprogDelay),
           secondsAttr<BgpPeer>(
               kTimersWithdrawUnprogDelay, setTimersWithdrawUnprogDelay)},
          {std::string(kNextHop4),
           ipAttr<BgpPeer>(kNextHop4, setNextHop4, /* requireV6 */ false)},
          {std::string(kNextHop6),
           ipAttr<BgpPeer>(kNextHop6, setNextHop6, /* requireV6 */ true)},
          {std::string(kNextHopSelf),
           boolAttr<BgpPeer>(kNextHopSelf, setNextHopSelf)},
          {std::string(kPeerId),
           stringAttr<BgpPeer>(kPeerId, "string", setPeerId)},
          {std::string(kPeerType),
           stringAttr<BgpPeer>(kPeerType, "string", setPeerType)},
          {std::string(kLinkBandwidth),
           bitRateAttr<BgpPeer>(kLinkBandwidth, setLinkBandwidth)},
          {std::string(kAdvertiseLbw),
           thriftEnumAttr<BgpPeer, AdvertiseLinkBandwidth>(
               kAdvertiseLbw, setAdvertiseLbw)},
          {std::string(kReceiveLbw),
           thriftEnumAttr<BgpPeer, ReceiveLinkBandwidth>(
               kReceiveLbw, setReceiveLbw)},
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
