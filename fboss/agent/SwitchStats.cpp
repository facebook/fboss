/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/SwitchStats.h"

#include <folly/Memory.h>
#include "fboss/agent/PortStats.h"
#include "fboss/lib/CommonUtils.h"

using facebook::fb303::AVG;
using facebook::fb303::RATE;
using facebook::fb303::SUM;

namespace facebook::fboss {

// set to empty string, we'll prepend prefix when fbagent collects counters
std::string SwitchStats::kCounterPrefix = "";

// Temporary until we get this into fb303 with D40324952
static constexpr const std::array<double, 1> kP100{{1.0}};

SwitchStats::SwitchStats()
    : SwitchStats(fb303::ThreadCachedServiceData::get()->getThreadStats()) {}

SwitchStats::SwitchStats(ThreadLocalStatsMap* map)
    : trapPkts_(map, kCounterPrefix + "trapped.pkts", SUM, RATE),
      trapPktDrops_(map, kCounterPrefix + "trapped.drops", SUM, RATE),
      trapPktBogus_(map, kCounterPrefix + "trapped.bogus", SUM, RATE),
      trapPktErrors_(map, kCounterPrefix + "trapped.error", SUM, RATE),
      trapPktUnhandled_(map, kCounterPrefix + "trapped.unhandled", SUM, RATE),
      trapPktToHost_(map, kCounterPrefix + "host.rx", SUM, RATE),
      trapPktToHostBytes_(map, kCounterPrefix + "host.rx.bytes", SUM, RATE),
      pktFromHost_(map, kCounterPrefix + "host.tx", SUM, RATE),
      pktFromHostBytes_(map, kCounterPrefix + "host.tx.bytes", SUM, RATE),
      trapPktArp_(map, kCounterPrefix + "trapped.arp", SUM, RATE),
      arpUnsupported_(map, kCounterPrefix + "arp.unsupported", SUM, RATE),
      arpNotMine_(map, kCounterPrefix + "arp.not_mine", SUM, RATE),
      arpRequestsRx_(map, kCounterPrefix + "arp.request.rx", SUM, RATE),
      arpRepliesRx_(map, kCounterPrefix + "arp.reply.rx", SUM, RATE),
      arpRequestsTx_(map, kCounterPrefix + "arp.request.tx", SUM, RATE),
      arpRepliesTx_(map, kCounterPrefix + "arp.reply.tx", SUM, RATE),
      arpBadOp_(map, kCounterPrefix + "arp.bad_op", SUM, RATE),
      trapPktNdp_(map, kCounterPrefix + "trapped.ndp", SUM, RATE),
      ipv6NdpBad_(map, kCounterPrefix + "ipv6.ndp.bad", SUM, RATE),
      ipv4Rx_(map, kCounterPrefix + "trapped.ipv4", SUM, RATE),
      ipv4TooSmall_(map, kCounterPrefix + "ipv4.too_small", SUM, RATE),
      ipv4WrongVer_(map, kCounterPrefix + "ipv4.wrong_version", SUM, RATE),
      ipv4Nexthop_(map, kCounterPrefix + "ipv4.nexthop", SUM, RATE),
      ipv4Mine_(map, kCounterPrefix + "ipv4.mine", SUM, RATE),
      ipv4NoArp_(map, kCounterPrefix + "ipv4.no_arp", SUM, RATE),
      ipv4TtlExceeded_(map, kCounterPrefix + "ipv4.ttl_exceeded", SUM, RATE),
      ipv6HopExceeded_(map, kCounterPrefix + "ipv6.hop_exceeded", SUM, RATE),
      udpTooSmall_(map, kCounterPrefix + "udp.too_small", SUM, RATE),
      dhcpV4Pkt_(map, kCounterPrefix + "dhcpV4.pkt", SUM, RATE),
      dhcpV4BadPkt_(map, kCounterPrefix + "dhcpV4.bad_pkt", SUM, RATE),
      dhcpV4DropPkt_(map, kCounterPrefix + "dhcpV4.drop_pkt", SUM, RATE),
      dhcpV6Pkt_(map, kCounterPrefix + "dhcpV6.pkt", SUM, RATE),
      dhcpV6BadPkt_(map, kCounterPrefix + "dhcpV6.bad_pkt", SUM, RATE),
      dhcpV6DropPkt_(map, kCounterPrefix + "dhcpV6.drop_pkt", SUM, RATE),
      addRouteV4_(map, kCounterPrefix + "route.v4.add", RATE),
      addRouteV6_(map, kCounterPrefix + "route.v6.add", RATE),
      delRouteV4_(map, kCounterPrefix + "route.v4.delete", RATE),
      delRouteV6_(map, kCounterPrefix + "route.v6.delete", RATE),
      dstLookupFailureV4_(
          map,
          kCounterPrefix + "ipv4.dst_lookup_failure",
          SUM,
          RATE),
      dstLookupFailureV6_(
          map,
          kCounterPrefix + "ipv6.dst_lookup_failure",
          SUM,
          RATE),
      dstLookupFailure_(
          map,
          kCounterPrefix + "ip.dst_lookup_failure",
          SUM,
          RATE),
      updateState_(map, kCounterPrefix + "state_update.us", 50000, 0, 1000000),
      routeUpdate_(map, kCounterPrefix + "route_update.us", 50, 0, 500),
      bgHeartbeatDelay_(
          map,
          kCounterPrefix + "bg_heartbeat_delay.ms",
          100,
          0,
          20000,
          AVG,
          50,
          100),
      updHeartbeatDelay_(
          map,
          kCounterPrefix + "upd_heartbeat_delay.ms",
          100,
          0,
          20000,
          AVG,
          50,
          100),
      packetTxHeartbeatDelay_(
          map,
          kCounterPrefix + "packetTx_heartbeat_delay.ms",
          100,
          0,
          20000,
          AVG,
          50,
          100),
      lacpHeartbeatDelay_(
          map,
          kCounterPrefix + "lacp_heartbeat_delay.ms",
          100,
          0,
          20000,
          AVG,
          50,
          100),
      neighborCacheHeartbeatDelay_(
          map,
          kCounterPrefix + "neighbor_cache_heartbeat_delay.ms",
          100,
          0,
          20000,
          AVG,
          50,
          100),
      bgEventBacklog_(
          map,
          kCounterPrefix + "bg_event_backlog",
          1,
          0,
          200,
          AVG,
          50,
          100),
      updEventBacklog_(
          map,
          kCounterPrefix + "upd_event_backlog",
          1,
          0,
          200,
          AVG,
          50,
          100),
      packetTxEventBacklog_(
          map,
          kCounterPrefix + "packetTx_event_backlog",
          1,
          0,
          200,
          AVG,
          50,
          100),
      lacpEventBacklog_(
          map,
          kCounterPrefix + "lacp_event_backlog",
          1,
          0,
          200,
          AVG,
          50,
          100),
      neighborCacheEventBacklog_(
          map,
          kCounterPrefix + "neighborCache_event_backlog",
          1,
          0,
          200,
          AVG,
          50,
          100),
      linkStateChange_(map, kCounterPrefix + "link_state.flap", SUM),
      pcapDistFailure_(map, kCounterPrefix + "pcap_dist_failure.error"),
      updateStatsExceptions_(
          map,
          kCounterPrefix + "update_stats_exceptions",
          SUM),
      trapPktTooBig_(map, kCounterPrefix + "trapped.packet_too_big", SUM, RATE),
      LldpRecvdPkt_(map, kCounterPrefix + "lldp.recvd", SUM, RATE),
      LldpBadPkt_(map, kCounterPrefix + "lldp.recv_bad", SUM, RATE),
      LldpValidateMisMatch_(
          map,
          kCounterPrefix + "lldp.validate_mismatch",
          SUM,
          RATE),
      LldpNeighborsSize_(map, kCounterPrefix + "lldp.neighbors_size", SUM),
      LacpRxTimeouts_(map, kCounterPrefix + "lacp.rx_timeout", SUM),
      LacpMismatchPduTeardown_(
          map,
          kCounterPrefix + "lacp.mismatched_pdu_teardown",
          SUM),
      MkPduRecvdPkts_(map, kCounterPrefix + "mkpdu.recvd", SUM, RATE),
      MkPduSendPkts_(map, kCounterPrefix + "mkpdu.send", SUM, RATE),
      MkPduSendFailure_(
          map,
          kCounterPrefix + "mkpdu.err.send_failure",
          SUM,
          RATE),
      MkPduPortNotRegistered_(
          map,
          kCounterPrefix + "mkpdu.err.port_not_regd",
          SUM,
          RATE),
      // key param is port name
      mkPduInterval_(
          "{}.mkpdu.interval.ms",
          facebook::fb303::ExportTypeConsts::kNone,
          kP100),
      MKAServiceSendFailure_(
          map,
          kCounterPrefix + "mka_service.err.send_failure",
          SUM,
          RATE),
      MKAServiceSendSuccess_(
          map,
          kCounterPrefix + "mka_service.send",
          SUM,
          RATE),
      MKAServiceRecvSuccess_(
          map,
          kCounterPrefix + "mka_service.recvd",
          SUM,
          RATE),
      pfcDeadlockDetectionCount_(
          map,
          kCounterPrefix + "pfc_deadlock_detection",
          SUM),
      pfcDeadlockRecoveryCount_(
          map,
          kCounterPrefix + "pfc_deadlock_recovery",
          SUM),
      threadHeartbeatMissCount_(
          map,
          kCounterPrefix + "thread_heartbeat_miss",
          SUM),
      fabricReachabilityMissingCount_(
          map,
          kCounterPrefix + "fabric_reachability_missing",
          SUM,
          RATE),
      fabricReachabilityMismatchCount_(
          map,
          kCounterPrefix + "fabric_reachability_mismatch",
          SUM,
          RATE),
      localSystemPort_(map, kCounterPrefix + "systemPort"),
      remoteSystemPort_(map, kCounterPrefix + "remoteSystemPort"),
      localRifs_(map, kCounterPrefix + "localRifs"),
      remoteRifs_(map, kCounterPrefix + "remoteRifs"),
      localResolvedNdp_(map, kCounterPrefix + "localResolvedNdp"),
      remoteResolvedNdp_(map, kCounterPrefix + "remoteResolvedNdp"),
      localResolvedArp_(map, kCounterPrefix + "localResolvedArp"),
      remoteResolvedArp_(map, kCounterPrefix + "remoteResolvedArp"),
      failedDsfSubscription_(map, kCounterPrefix + "failedDsfSubscription") {}

PortStats* FOLLY_NULLABLE SwitchStats::port(PortID portID) {
  auto it = ports_.find(portID);
  if (it != ports_.end()) {
    return it->second.get();
  }
  // Since PortStats needs portName from current switch state, let caller to
  // decide whether it needs createPortStats function.
  return nullptr;
}

AggregatePortStats* FOLLY_NULLABLE
SwitchStats::aggregatePort(AggregatePortID aggregatePortID) {
  auto it = aggregatePortIDToStats_.find(aggregatePortID);

  return it == aggregatePortIDToStats_.end() ? nullptr : it->second.get();
}

PortStats* SwitchStats::createPortStats(PortID portID, std::string portName) {
  auto rv = ports_.emplace(
      portID, std::make_unique<PortStats>(portID, portName, this));
  DCHECK(rv.second);
  const auto& it = rv.first;
  return it->second.get();
}

AggregatePortStats* SwitchStats::createAggregatePortStats(
    AggregatePortID id,
    std::string name) {
  auto [it, inserted] = aggregatePortIDToStats_.emplace(
      id, std::make_unique<AggregatePortStats>(id, name));
  CHECK(inserted);

  return it->second.get();
}

InterfaceStats* SwitchStats::createInterfaceStats(
    InterfaceID intfID,
    std::string intfName) {
  auto rv = intfIDToStats_.emplace(
      intfID, std::make_unique<InterfaceStats>(intfID, intfName, this));
  DCHECK(rv.second);
  const auto& it = rv.first;
  return it->second.get();
}

InterfaceStats* FOLLY_NULLABLE SwitchStats::intf(InterfaceID intfID) {
  auto it = intfIDToStats_.find(intfID);
  if (it != intfIDToStats_.end()) {
    return it->second.get();
  }
  // Since InterfaceStats needs intfName from current switch state, let caller
  // to decide whether it needs createPortStats function.
  return nullptr;
}

void SwitchStats::fillAgentStats(AgentStats& agentStats) const {
  agentStats.linkFlaps() = getCumulativeValue(linkStateChange_);
}
} // namespace facebook::fboss
