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
#include <folly/Range.h>
#include <folly/Utility.h>
#include "fboss/agent/PortStats.h"
#include "fboss/lib/CommonUtils.h"

using facebook::fb303::AVG;
using facebook::fb303::RATE;
using facebook::fb303::SUM;

namespace {
std::string fabricOverdrainCounter(int16_t switchIndex) {
  return folly::to<std::string>(
      "switch.", switchIndex, ".fabric_overdrain_pct");
}
} // namespace

namespace facebook::fboss {

// set to empty string, we'll prepend prefix when fbagent collects counters
std::string SwitchStats::kCounterPrefix = "";

// Temporary until we get this into fb303 with D40324952
static constexpr const std::array<double, 1> kP100{{1.0}};

SwitchStats::SwitchStats(int numSwitches)
    : SwitchStats(
          fb303::ThreadCachedServiceData::get()->getThreadStats(),
          numSwitches) {}

SwitchStats::SwitchStats(ThreadLocalStatsMap* map, int numSwitches)
    : numSwitches_(numSwitches),
      trapPkts_(map, kCounterPrefix + "trapped.pkts", SUM, RATE),
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
      ipv6NdpNotMine_(map, kCounterPrefix + "ipv6.ndp.not_mine", SUM, RATE),
      ipv4Rx_(map, kCounterPrefix + "trapped.ipv4", SUM, RATE),
      ipv4TooSmall_(map, kCounterPrefix + "ipv4.too_small", SUM, RATE),
      ipv4WrongVer_(map, kCounterPrefix + "ipv4.wrong_version", SUM, RATE),
      ipv4Nexthop_(map, kCounterPrefix + "ipv4.nexthop", SUM, RATE),
      ipv4Mine_(map, kCounterPrefix + "ipv4.mine", SUM, RATE),
      ipv4NoArp_(map, kCounterPrefix + "ipv4.no_arp", SUM, RATE),
      ipv4TtlExceeded_(map, kCounterPrefix + "ipv4.ttl_exceeded", SUM, RATE),
      ipv4Ttl1Mine_(map, kCounterPrefix + "ipv4.ttl1_mine", SUM, RATE),
      ipv6HopExceeded_(map, kCounterPrefix + "ipv6.hop_exceeded", SUM, RATE),
      ipv6HopLimit1Mine_(
          map,
          kCounterPrefix + "ipv6.hop_limit1_mine",
          SUM,
          RATE),
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
      packetRxHeartbeatDelay_(
          map,
          kCounterPrefix + "packetRx_heartbeat_delay.ms",
          100,
          0,
          30000,
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
      maxNumOfPhysicalHostsPerQueue_(
          map,
          kCounterPrefix + "max_num_of_physical_hosts_per_queue",
          1,
          0,
          200,
          AVG),
      linkStateChange_(map, kCounterPrefix + "link_state.flap", SUM),
      linkActiveStateChange_(
          map,
          kCounterPrefix + "link_active_state.flap",
          SUM),
      switchReachabilityChangeProcessed_(
          map,
          kCounterPrefix + "switch_reachability_change_processed",
          SUM),

      pcapDistFailure_(map, kCounterPrefix + "pcap_dist_failure.error"),
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
      localSystemPort_(map, kCounterPrefix + "systemPort"),
      remoteSystemPort_(map, kCounterPrefix + "remoteSystemPort"),
      localRifs_(map, kCounterPrefix + "localRifs"),
      remoteRifs_(map, kCounterPrefix + "remoteRifs"),
      localResolvedNdp_(map, kCounterPrefix + "localResolvedNdp"),
      remoteResolvedNdp_(map, kCounterPrefix + "remoteResolvedNdp"),
      localResolvedArp_(map, kCounterPrefix + "localResolvedArp"),
      remoteResolvedArp_(map, kCounterPrefix + "remoteResolvedArp"),
      failedDsfSubscription_(map, kCounterPrefix + "failedDsfSubscription"),
      coldBoot_(map, kCounterPrefix + "cold_boot", SUM, RATE),
      warmBoot_(map, kCounterPrefix + "warm_boot", SUM, RATE),
      switchConfiguredMs_(
          map,
          kCounterPrefix + "switch_configured_ms",
          SUM,
          RATE),
      dsfGrExpired_(map, kCounterPrefix + "dsfsession_gr_expired", SUM, RATE),
      dsfUpdateFailed_(map, kCounterPrefix + "dsf_update_failed", SUM, RATE),
      multiSwitchStatus_(map, kCounterPrefix + "multi_switch", SUM, RATE)

{
  for (auto switchIndex = 0; switchIndex < numSwitches; switchIndex++) {
    hwAgentConnectionStatus_.emplace_back(TLCounter(
        map,
        folly::to<std::string>(
            kCounterPrefix, "switch.", switchIndex, ".", "connection_status")));
    hwAgentUpdateTimeouts_.emplace_back(TLTimeseries(
        map,
        folly::to<std::string>(
            kCounterPrefix, "switch.", switchIndex, ".", "hwupdate_timeouts"),
        SUM,
        RATE));
    thriftStreamConnectionStatus_.emplace_back(map, switchIndex);
  }
}

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
  agentStats.trappedPktsDropped() = getCumulativeValue(trapPktDrops_);
  agentStats.threadHeartBeatMiss() =
      getCumulativeValue(threadHeartbeatMissCount_);
  int16_t switchIndex = 0;
  for (const auto& stats : hwAgentUpdateTimeouts_) {
    agentStats.hwagentOperSyncTimeoutCount()->insert(
        {switchIndex, getCumulativeValue(stats)});
    switchIndex++;
  }
  getHwAgentStatus(*agentStats.hwAgentEventSyncStatusMap());
  for (auto swIndex = 0; swIndex < numSwitches_; swIndex++) {
    auto overdrainPct = fb303::fbData->getCounterIfExists(
        fabricOverdrainCounter(folly::to_narrow(swIndex)));
    if (overdrainPct.has_value()) {
      agentStats.fabricOverdrainPct()->insert({swIndex, *overdrainPct});
    }
  }
}

void SwitchStats::getHwAgentStatus(
    std::map<int16_t, HwAgentEventSyncStatus>& statusMap) const {
  int16_t switchIndex = 0;
  for (const auto& stats : thriftStreamConnectionStatus_) {
    HwAgentEventSyncStatus syncStatus;
    syncStatus.statsEventSyncActive() = stats.getStatsEventSinkStatus();
    syncStatus.linkEventSyncActive() = stats.getLinkEventSinkStatus();
    syncStatus.fdbEventSyncActive() = stats.getFdbEventSinkStatus();
    syncStatus.rxPktEventSyncActive() = stats.getRxPktEventSinkStatus();
    syncStatus.txPktEventSyncActive() = stats.getTxPktEventStreamStatus();
    syncStatus.switchReachabilityChangeEventSyncActive() =
        stats.getSwitchReachabilityChangeEventSinkStatus();
    syncStatus.statsEventSyncDisconnects() =
        stats.getStatsEventSinkDisconnectCount();
    syncStatus.fdbEventSyncDisconnects() =
        stats.getFdbEventSinkDisconnectCount();
    syncStatus.linkEventSyncDisconnects() =
        stats.getLinkEventSinkDisconnectCount();
    syncStatus.rxPktEventSyncDisconnects() =
        stats.getRxPktEventSinkDisconnectCount();
    syncStatus.txPktEventSyncDisconnects() =
        stats.getTxPktEventStreamDisconnectCount();
    syncStatus.switchReachabilityChangeEventSyncDisconnects() =
        stats.getSwitchReachabilityChangeEventSinkDisconnectCount();

    statusMap.insert({switchIndex, std::move(syncStatus)});
    switchIndex++;
  }
}

SwitchStats::HwAgentStreamConnectionStatus::HwAgentStreamConnectionStatus(
    fb303::ThreadCachedServiceData::ThreadLocalStatsMap* map,
    int16_t switchIndex)
    : statsEventSinkStatus_(TLCounter(
          map,
          folly::to<std::string>(
              kCounterPrefix,
              "switch.",
              switchIndex,
              ".",
              "stats_event_sync_active"))),
      linkEventSinkStatus_(TLCounter(
          map,
          folly::to<std::string>(
              kCounterPrefix,
              "switch.",
              switchIndex,
              ".",
              "link_event_sync_active"))),
      fdbEventSinkStatus_(TLCounter(
          map,
          folly::to<std::string>(
              kCounterPrefix,
              "switch.",
              switchIndex,
              ".",
              "fdb_event_sync_active"))),
      rxPktEventSinkStatus_(TLCounter(
          map,
          folly::to<std::string>(
              kCounterPrefix,
              "switch.",
              switchIndex,
              ".",
              "rx_pkt_event_sync_active"))),
      txPktEventStreamStatus_(TLCounter(
          map,
          folly::to<std::string>(
              kCounterPrefix,
              "switch.",
              switchIndex,
              ".",
              "tx_pkt_event_sync_active"))),
      switchReachabilityChangeEventSinkStatus_(TLCounter(
          map,
          folly::to<std::string>(
              kCounterPrefix,
              "switch.",
              switchIndex,
              ".",
              "switch_reachability_change_event_sync_active"))),
      statsEventSinkDisconnects_(TLTimeseries(
          map,
          folly::to<std::string>(
              kCounterPrefix,
              "switch.",
              switchIndex,
              ".",
              "stats_event_sync_disconnects"),
          SUM,
          RATE)),
      linkEventSinkDisconnects_(TLTimeseries(
          map,
          folly::to<std::string>(
              kCounterPrefix,
              "switch.",
              switchIndex,
              ".",
              "link_event_sync_disconnects"),
          SUM,
          RATE)),
      fdbEventSinkDisconnects_(TLTimeseries(
          map,
          folly::to<std::string>(
              kCounterPrefix,
              "switch.",
              switchIndex,
              ".",
              "fdb_event_sync_disconnects"),
          SUM,
          RATE)),
      rxPktEventSinkDisconnects_(TLTimeseries(
          map,
          folly::to<std::string>(
              kCounterPrefix,
              "switch.",
              switchIndex,
              ".",
              "rx_pkt_event_sync_disconnects"),
          SUM,
          RATE)),
      txPktEventStreamDisconnects_(TLTimeseries(
          map,
          folly::to<std::string>(
              kCounterPrefix,
              "switch.",
              switchIndex,
              ".",
              "tx_pkt_event_sync_disconnects"),
          SUM,
          RATE)),
      switchReachabilityChangeEventSinkDisconnects_(TLTimeseries(
          map,
          folly::to<std::string>(
              kCounterPrefix,
              "switch.",
              switchIndex,
              ".",
              "switch_reachability_change_event_sync_disconnects"),
          SUM,
          RATE)),
      statsEventsReceived_(TLTimeseries(
          map,
          folly::to<std::string>(
              kCounterPrefix,
              "switch.",
              switchIndex,
              ".",
              "stats_event_received"),
          SUM,
          RATE)),
      linkEventsReceived_(TLTimeseries(
          map,
          folly::to<std::string>(
              kCounterPrefix,
              "switch.",
              switchIndex,
              ".",
              "link_event_received"),
          SUM,
          RATE)),
      fdbEventsReceived_(TLTimeseries(
          map,
          folly::to<std::string>(
              kCounterPrefix,
              "switch.",
              switchIndex,
              ".",
              "fdb_event_received"),
          SUM,
          RATE)),
      rxPktEventsReceived_(TLTimeseries(
          map,
          folly::to<std::string>(
              kCounterPrefix,
              "switch.",
              switchIndex,
              ".",
              "rx_pkt_event_received"),
          SUM,
          RATE)),
      txPktEventsSent_(TLTimeseries(
          map,
          folly::to<std::string>(
              kCounterPrefix,
              "switch.",
              switchIndex,
              ".",
              "tx_pkt_event_sent"),
          SUM,
          RATE)),
      switchReachabilityChangeEventsReceived_(TLTimeseries(
          map,
          folly::to<std::string>(
              kCounterPrefix,
              "switch.",
              switchIndex,
              ".",
              "switch_reachability_change_event_received"),
          SUM,
          RATE)),
      rxBadPktReceived_(TLTimeseries(
          map,
          folly::to<std::string>(
              kCounterPrefix,
              "switch.",
              switchIndex,
              ".",
              "rx_bad_pkt_received"),
          SUM,
          RATE)) {}

void SwitchStats::setFabricOverdrainPct(
    int16_t switchIndex,
    int16_t overdrainPct) {
  // We directly set the overdrain pct counter here as its a single
  // value that needs to set globally and not a thread local value
  // that needs to accumulated over multiple threads.
  // Alternatively using a TLCounter would have required us to do something
  // like
  // counter.addValue(overdrainPct - getCumuativeValue(counter));
  // i.e. collect global value from all threads and then compute delta
  // against the current value. Instead set the global value directly
  fb303::fbData->setCounter(fabricOverdrainCounter(switchIndex), overdrainPct);
  updateFabricOverdrainWatermark(switchIndex, overdrainPct);
}

void SwitchStats::failedDsfSubscription(
    const std::string& peerName,
    int value) {
  failedDsfSubscription_.incrementValue(value);
  if (failedDsfSubscriptionByPeerSwitchName_.find(peerName) ==
      failedDsfSubscriptionByPeerSwitchName_.end()) {
    failedDsfSubscriptionByPeerSwitchName_.emplace(
        peerName,
        TLCounter(
            fb303::ThreadCachedServiceData::get()->getThreadStats(),
            folly::to<std::string>(
                kCounterPrefix, "failedDsfSubscriptionTo.", peerName)));
  }
  auto counter = failedDsfSubscriptionByPeerSwitchName_.find(peerName);
  counter->second.incrementValue(value);
}
} // namespace facebook::fboss
