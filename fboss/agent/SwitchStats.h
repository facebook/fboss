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

#include <boost/container/flat_map.hpp>
#include <boost/noncopyable.hpp>
#include <fb303/ThreadCachedServiceData.h>
#include <fb303/detail/QuantileStatWrappers.h>
#include <chrono>
#include "fboss/agent/AggregatePortStats.h"
#include "fboss/agent/InterfaceStats.h"
#include "fboss/agent/PortStats.h"
#include "fboss/agent/gen-cpp2/agent_stats_types.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {

class PortStats;

typedef boost::container::flat_map<PortID, std::unique_ptr<PortStats>>
    PortStatsMap;
using AggregatePortStatsMap = boost::container::
    flat_map<AggregatePortID, std::unique_ptr<AggregatePortStats>>;
using InterfaceStatsMap =
    boost::container::flat_map<InterfaceID, std::unique_ptr<InterfaceStats>>;

class SwitchStats : public boost::noncopyable {
 public:
  /*
   * The prefix to use for our counter names
   */
  static std::string kCounterPrefix;

  SwitchStats();

  /*
   * Return the PortStats object for the given PortID.
   */
  PortStats* FOLLY_NULLABLE port(PortID portID);

  AggregatePortStats* FOLLY_NULLABLE
  aggregatePort(AggregatePortID aggregatePortID);

  InterfaceStats* FOLLY_NULLABLE intf(InterfaceID intfID);

  /*
   * Getters.
   */
  PortStatsMap* getPortStats() {
    return &ports_;
  }
  const PortStatsMap* getPortStats() const {
    return &ports_;
  }

  // Create a PortStats object for the given PortID
  PortStats* createPortStats(PortID portID, std::string portName);
  AggregatePortStats* createAggregatePortStats(
      AggregatePortID id,
      std::string name);

  void deletePortStats(PortID portID) {
    ports_.erase(portID);
  }

  InterfaceStatsMap* getInterfaceStats() {
    return &intfIDToStats_;
  }

  const InterfaceStatsMap* getInterfaceStats() const {
    return &intfIDToStats_;
  }

  InterfaceStats* createInterfaceStats(
      InterfaceID intfID,
      std::string intfName);

  void deleteInterfaceStats(InterfaceID intfID) {
    intfIDToStats_.erase(intfID);
  }

  void trappedPkt() {
    trapPkts_.addValue(1);
  }
  void pktDropped() {
    trapPktDrops_.addValue(1);
  }
  void pktBogus() {
    trapPktBogus_.addValue(1);
    trapPktDrops_.addValue(1);
  }
  void pktError() {
    trapPktErrors_.addValue(1);
    trapPktDrops_.addValue(1);
  }
  void pktUnhandled() {
    trapPktUnhandled_.addValue(1);
    trapPktDrops_.addValue(1);
  }
  void pktToHost(uint32_t bytes) {
    trapPktToHost_.addValue(1);
    trapPktToHostBytes_.addValue(bytes);
  }
  void pktFromHost(uint32_t bytes) {
    pktFromHost_.addValue(1);
    pktFromHostBytes_.addValue(bytes);
  }

  void arpPkt() {
    trapPktArp_.addValue(1);
  }
  void arpUnsupported() {
    arpUnsupported_.addValue(1);
    trapPktDrops_.addValue(1);
  }
  void arpNotMine() {
    arpNotMine_.addValue(1);
    trapPktDrops_.addValue(1);
  }
  void arpRequestRx() {
    arpRequestsRx_.addValue(1);
  }
  void arpRequestTx() {
    arpRequestsTx_.addValue(1);
  }
  void arpReplyRx() {
    arpRepliesRx_.addValue(1);
  }
  void arpReplyTx() {
    arpRepliesTx_.addValue(1);
  }
  void arpBadOp() {
    arpBadOp_.addValue(1);
    trapPktDrops_.addValue(1);
  }

  void ipv6NdpPkt() {
    trapPktNdp_.addValue(1);
  }
  void ipv6NdpBad() {
    ipv6NdpBad_.addValue(1);
    trapPktDrops_.addValue(1);
  }

  void dhcpV4Pkt() {
    dhcpV4Pkt_.addValue(1);
  }

  void dhcpV6Pkt() {
    dhcpV6Pkt_.addValue(1);
  }

  void ipv4Rx() {
    ipv4Rx_.addValue(1);
  }
  void ipv4TooSmall() {
    ipv4TooSmall_.addValue(1);
  }
  void ipv4WrongVer() {
    ipv4WrongVer_.addValue(1);
  }
  void ipv4Nexthop() {
    ipv4Nexthop_.addValue(1);
  }
  void ipv4Mine() {
    ipv4Mine_.addValue(1);
  }
  void ipv4NoArp() {
    ipv4NoArp_.addValue(1);
  }
  void ipv4TtlExceeded() {
    ipv4TtlExceeded_.addValue(1);
  }

  void ipv6HopExceeded() {
    ipv6HopExceeded_.addValue(1);
  }

  void udpTooSmall() {
    udpTooSmall_.addValue(1);
  }

  void dhcpV4BadPkt() {
    dhcpV4BadPkt_.addValue(1);
    dhcpV4DropPkt_.addValue(1);
    trapPktDrops_.addValue(1);
  }

  void dhcpV4DropPkt() {
    dhcpV4DropPkt_.addValue(1);
    trapPktDrops_.addValue(1);
  }

  void dhcpV6BadPkt() {
    dhcpV6BadPkt_.addValue(1);
    dhcpV6DropPkt_.addValue(1);
    trapPktDrops_.addValue(1);
  }

  void dhcpV6DropPkt() {
    dhcpV6DropPkt_.addValue(1);
    trapPktDrops_.addValue(1);
  }

  void addRouteV4() {
    addRouteV4_.addValue(1);
  }
  void addRouteV6() {
    addRouteV6_.addValue(1);
  }
  void delRouteV4() {
    delRouteV4_.addValue(1);
  }
  void delRouteV6() {
    delRouteV6_.addValue(1);
  }

  void addRoutesV4(uint64_t routeCount) {
    addRouteV4_.addValue(routeCount);
  }
  void addRoutesV6(uint64_t routeCount) {
    addRouteV6_.addValue(routeCount);
  }
  void delRoutesV4(uint64_t routeCount) {
    delRouteV4_.addValue(routeCount);
  }
  void delRoutesV6(uint64_t routeCount) {
    delRouteV6_.addValue(routeCount);
  }

  void ipv4DstLookupFailure() {
    dstLookupFailureV4_.addValue(1);
    dstLookupFailure_.addValue(1);
  }

  void ipv6DstLookupFailure() {
    dstLookupFailureV6_.addValue(1);
    dstLookupFailure_.addValue(1);
  }

  void stateUpdate(std::chrono::microseconds us) {
    updateState_.addValue(us.count());
  }

  void routeUpdate(std::chrono::microseconds us, uint64_t routes) {
    // As syncFib() could include no routes.
    if (routes == 0) {
      routes = 1;
    }
    // TODO: add support for addRepeatedValue() in FSDB client-side library
    routeUpdate_.addRepeatedValue(us.count() / routes, routes);
  }

  void bgHeartbeatDelay(int delay) {
    bgHeartbeatDelay_.addValue(delay);
  }

  void updHeartbeatDelay(int delay) {
    updHeartbeatDelay_.addValue(delay);
  }

  void packetTxHeartbeatDelay(int value) {
    packetTxHeartbeatDelay_.addValue(value);
  }

  void lacpHeartbeatDelay(int value) {
    lacpHeartbeatDelay_.addValue(value);
  }

  void neighborCacheHeartbeatDelay(int value) {
    neighborCacheHeartbeatDelay_.addValue(value);
  }

  void bgEventBacklog(int value) {
    bgEventBacklog_.addValue(value);
  }

  void updEventBacklog(int value) {
    updEventBacklog_.addValue(value);
  }

  void lacpEventBacklog(int value) {
    lacpEventBacklog_.addValue(value);
  }

  void packetTxEventBacklog(int value) {
    packetTxEventBacklog_.addValue(value);
  }

  void neighborCacheEventBacklog(int value) {
    neighborCacheEventBacklog_.addValue(value);
  }

  void linkStateChange() {
    linkStateChange_.addValue(1);
  }

  void pcapDistFailure() {
    pcapDistFailure_.incrementValue(1);
  }

  void updateStatsException() {
    updateStatsExceptions_.addValue(1);
  }

  void pktTooBig() {
    trapPktTooBig_.addValue(1);
  }

  void LldpRecvdPkt() {
    LldpRecvdPkt_.addValue(1);
  }
  void LldpBadPkt() {
    LldpBadPkt_.addValue(1);
  }
  void LldpValidateMisMatch() {
    LldpValidateMisMatch_.addValue(1);
  }
  void LldpNeighborsSize(int value) {
    LldpNeighborsSize_.addValue(value);
  }
  void LacpRxTimeouts() {
    LacpRxTimeouts_.addValue(1);
  }
  void LacpMismatchPduTeardown() {
    LacpMismatchPduTeardown_.addValue(1);
  }

  void MkPduRecvdPkt() {
    MkPduRecvdPkts_.addValue(1);
  }
  void MkPduSendPkt() {
    MkPduSendPkts_.addValue(1);
  }
  void MkPduSendFailure() {
    MkPduSendFailure_.addValue(1);
  }
  void MkPduPortNotRegistered() {
    MkPduPortNotRegistered_.addValue(1);
  }
  void MkPduInterval(double value, std::string_view port) {
    mkPduInterval_.addValue(value, port);
  }
  void MKAServiceSendFailue() {
    MKAServiceSendFailure_.addValue(1);
  }
  void MKAServiceSendSuccess() {
    MKAServiceSendSuccess_.addValue(1);
  }
  void MKAServiceRecvSuccess() {
    MKAServiceRecvSuccess_.addValue(1);
  }
  void PfcDeadlockDetectionCount() {
    pfcDeadlockDetectionCount_.addValue(1);
  }
  void PfcDeadlockRecoveryCount() {
    pfcDeadlockRecoveryCount_.addValue(1);
  }
  void ThreadHeartbeatMissCount() {
    threadHeartbeatMissCount_.addValue(1);
  }

  void localSystemPort(int value) {
    localSystemPort_.incrementValue(value);
  }
  void remoteSystemPort(int value) {
    remoteSystemPort_.incrementValue(value);
  }
  void localRifs(int value) {
    localRifs_.incrementValue(value);
  }
  void remoteRifs(int value) {
    remoteRifs_.incrementValue(value);
  }
  void localResolvedNdp(int value) {
    localResolvedNdp_.incrementValue(value);
  }
  void remoteResolvedNdp(int value) {
    remoteResolvedNdp_.incrementValue(value);
  }
  void localResolvedArp(int value) {
    localResolvedArp_.incrementValue(value);
  }
  void remoteResolvedArp(int value) {
    remoteResolvedArp_.incrementValue(value);
  }

  typedef fb303::ThreadCachedServiceData::ThreadLocalStatsMap
      ThreadLocalStatsMap;
  typedef fb303::ThreadCachedServiceData::TLTimeseries TLTimeseries;
  typedef fb303::ThreadCachedServiceData::TLHistogram TLHistogram;
  typedef fb303::ThreadCachedServiceData::TLCounter TLCounter;

  void fillAgentStats(AgentStats& agentStats) const;

 private:
  // Forbidden copy constructor and assignment operator
  SwitchStats(SwitchStats const&) = delete;
  SwitchStats& operator=(SwitchStats const&) = delete;

  explicit SwitchStats(ThreadLocalStatsMap* map);

  // Total number of trapped packets
  TLTimeseries trapPkts_;
  // Number of trapped packets that were intentionally dropped.
  TLTimeseries trapPktDrops_;
  // Malformed packets received
  TLTimeseries trapPktBogus_;
  // Number of times the controller encountered an error trying to process
  // a packet.
  TLTimeseries trapPktErrors_;
  // Trapped packets that the controller didn't know how to handle.
  TLTimeseries trapPktUnhandled_;
  // Trapped packets forwarded to host
  TLTimeseries trapPktToHost_;
  // Trapped packets forwarded to host in bytes
  TLTimeseries trapPktToHostBytes_;
  // Packets sent by host
  TLTimeseries pktFromHost_;
  // Packets sent by host in bytes
  TLTimeseries pktFromHostBytes_;

  // ARP Packets
  TLTimeseries trapPktArp_;
  // ARP packets dropped due to unsupported hardware or protocol settings.
  TLTimeseries arpUnsupported_;
  // ARP packets ignored because we were not the target.
  TLTimeseries arpNotMine_;
  // ARP requests destined for us
  TLTimeseries arpRequestsRx_;
  // ARP replies destined for us
  TLTimeseries arpRepliesRx_;
  // ARP requests sent from us
  TLTimeseries arpRequestsTx_;
  // ARP replies sent from us
  TLTimeseries arpRepliesTx_;
  // ARP packets with an unknown op field
  TLTimeseries arpBadOp_;

  // IPv6 Neighbor Discovery Protocol packets
  TLTimeseries trapPktNdp_;
  TLTimeseries ipv6NdpBad_;

  // IPv4 Packets
  TLTimeseries ipv4Rx_;
  // IPv4 packets dropped due to smaller packet size
  TLTimeseries ipv4TooSmall_;
  // IPv4 packets dropped due to wrong version number
  TLTimeseries ipv4WrongVer_;
  // IPv4 packets received for resolving nexthop
  TLTimeseries ipv4Nexthop_;
  // IPv4 packets received for myself
  TLTimeseries ipv4Mine_;
  // IPv4 packets received, but not able to send out ARP request
  TLTimeseries ipv4NoArp_;
  // IPv4 TTL exceeded
  TLTimeseries ipv4TtlExceeded_;

  // IPv6 hop count exceeded
  TLTimeseries ipv6HopExceeded_;

  // UDP packets dropped due to smaller packet size
  TLTimeseries udpTooSmall_;

  // Total DHCP packets received
  TLTimeseries dhcpV4Pkt_;
  /*
   *  Consolidate DHCP v4 drops under 2 heads
   *  a) Bad dhcp packet receive
   *  b) Catch all dhcp packet drop. This includes a) but also includes
   *  cases where a dhcp relay was not configured for this VLAN
   */
  // DHCPv4 packets dropped due to packet being malformed
  TLTimeseries dhcpV4BadPkt_;
  // DHCPv4 packets dropped
  TLTimeseries dhcpV4DropPkt_;
  // stats for dhcpV6
  TLTimeseries dhcpV6Pkt_;
  TLTimeseries dhcpV6BadPkt_;
  TLTimeseries dhcpV6DropPkt_;

  /**
   * Routes add/delete stats
   *
   * Modification is treated as add.
   */
  TLTimeseries addRouteV4_;
  TLTimeseries addRouteV6_;
  TLTimeseries delRouteV4_;
  TLTimeseries delRouteV6_;

  TLTimeseries dstLookupFailureV4_;
  TLTimeseries dstLookupFailureV6_;
  TLTimeseries dstLookupFailure_;

  /**
   * Histogram for time used for SwSwitch::updateState() (in ms)
   */
  TLHistogram updateState_;

  /**
   * Histogram for time used for route update (in microsecond)
   */
  TLHistogram routeUpdate_;

  /**
   * Background thread heartbeat delay (ms)
   */
  TLHistogram bgHeartbeatDelay_;

  /**
   * Update thread heartbeat delay (ms)
   */
  TLHistogram updHeartbeatDelay_;
  /**
   * Fboss packet Tx thread heartbeat delay (ms)
   */
  TLHistogram packetTxHeartbeatDelay_;
  /**
   * LACP thread heartbeat delay in milliseconds
   */
  TLHistogram lacpHeartbeatDelay_;
  /**
   * Arp Cache thread heartbeat delay in milliseconds
   */
  TLHistogram neighborCacheHeartbeatDelay_;

  /**
   * Number of events queued in background thread
   */
  TLHistogram bgEventBacklog_;

  /**
   * Number of events queued in update thread
   */
  TLHistogram updEventBacklog_;
  /**
   * Number of events queued in fboss packet TX thread
   */
  TLHistogram packetTxEventBacklog_;

  TLHistogram lacpEventBacklog_;
  /**
   * Number of events queued in fboss Arp Cache thread
   */
  TLHistogram neighborCacheEventBacklog_;

  /**
   * Link state up/down change count
   */
  TLTimeseries linkStateChange_;

  // Individual port stats objects, indexed by PortID
  PortStatsMap ports_;

  AggregatePortStatsMap aggregatePortIDToStats_;

  InterfaceStatsMap intfIDToStats_;

  // Number of packets dropped by the PCAP distribution service
  TLCounter pcapDistFailure_;

  // Number of failed updateStats callbacks do to exceptions.
  TLTimeseries updateStatsExceptions_;

  // Number of packet too big ICMPv6 triggered
  TLTimeseries trapPktTooBig_;

  // Number of LLDP packets.
  TLTimeseries LldpRecvdPkt_;
  // Number of bad LLDP packets.
  TLTimeseries LldpBadPkt_;
  // Number of LLDP packets that did not match configured, expected values.
  TLTimeseries LldpValidateMisMatch_;
  // Number of LLDP Neighbors.
  TLTimeseries LldpNeighborsSize_;

  // Number of LACP Rx timeouts
  TLTimeseries LacpRxTimeouts_;
  // Number of LACP session teardown due to mismatching PDUs
  TLTimeseries LacpMismatchPduTeardown_;
  // Number of MkPdu Received.
  TLTimeseries MkPduRecvdPkts_;
  // Number of MkPdu Send.
  TLTimeseries MkPduSendPkts_;
  // Number of MkPdu Send Failure
  TLTimeseries MkPduSendFailure_;
  // Number of pkt recv when port is not registered.
  TLTimeseries MkPduPortNotRegistered_;
  // MkPdu interval (per port)
  fb303::detail::DynamicQuantileStatWrapper<1> mkPduInterval_;
  // Number of Send Pkt to MkaService Failure.
  TLTimeseries MKAServiceSendFailure_;
  // Number of Pkts Send to MKAService.
  TLTimeseries MKAServiceSendSuccess_;
  // Number of pkts recvd from MkaService.
  TLTimeseries MKAServiceRecvSuccess_;
  // Number of timers pfc deadlock watchodg was hit
  TLTimeseries pfcDeadlockDetectionCount_;
  // Number of timers pfc deadlock recovery hit
  TLTimeseries pfcDeadlockRecoveryCount_;
  // Number of thread heartbeat misses
  TLTimeseries threadHeartbeatMissCount_;

  // Number of system ports in switch state
  TLCounter localSystemPort_;
  // Number of remote system port in switch state
  TLCounter remoteSystemPort_;
  // Number of local Rifs
  TLCounter localRifs_;
  // Number of remote Rifs
  TLCounter remoteRifs_;
  // Number of local resolved NDP
  TLCounter localResolvedNdp_;
  // Number of remote resolved NDP
  TLCounter remoteResolvedNdp_;
  // Number of local resolved ARP
  TLCounter localResolvedArp_;
  // Number of remote resolved ARP
  TLCounter remoteResolvedArp_;
};

} // namespace facebook::fboss
