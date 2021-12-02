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
#include <chrono>
#include "fboss/agent/AggregatePortStats.h"
#include "fboss/agent/PortStats.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {

class PortStats;

typedef boost::container::flat_map<PortID, std::unique_ptr<PortStats>>
    PortStatsMap;
using AggregatePortStatsMap = boost::container::
    flat_map<AggregatePortID, std::unique_ptr<AggregatePortStats>>;

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

  void trappedPkt() {
    addValue(*trapPkts_, 1);
  }
  void pktDropped() {
    addValue(*trapPktDrops_, 1);
  }
  void pktBogus() {
    addValue(*trapPktBogus_, 1);
    addValue(*trapPktDrops_, 1);
  }
  void pktError() {
    addValue(*trapPktErrors_, 1);
    addValue(*trapPktDrops_, 1);
  }
  void pktUnhandled() {
    addValue(*trapPktUnhandled_, 1);
    addValue(*trapPktDrops_, 1);
  }
  void pktToHost(uint32_t bytes) {
    addValue(*trapPktToHost_, 1);
    addValue(*trapPktToHostBytes_, bytes);
  }
  void pktFromHost(uint32_t bytes) {
    addValue(*pktFromHost_, 1);
    addValue(*pktFromHostBytes_, bytes);
  }

  void arpPkt() {
    addValue(*trapPktArp_, 1);
  }
  void arpUnsupported() {
    addValue(*arpUnsupported_, 1);
    addValue(*trapPktDrops_, 1);
  }
  void arpNotMine() {
    addValue(*arpNotMine_, 1);
    addValue(*trapPktDrops_, 1);
  }
  void arpRequestRx() {
    addValue(*arpRequestsRx_, 1);
  }
  void arpRequestTx() {
    addValue(*arpRequestsTx_, 1);
  }
  void arpReplyRx() {
    addValue(*arpRepliesRx_, 1);
  }
  void arpReplyTx() {
    addValue(*arpRepliesTx_, 1);
  }
  void arpBadOp() {
    addValue(*arpBadOp_, 1);
    addValue(*trapPktDrops_, 1);
  }

  void ipv6NdpPkt() {
    addValue(*trapPktNdp_, 1);
  }
  void ipv6NdpBad() {
    addValue(*ipv6NdpBad_, 1);
    addValue(*trapPktDrops_, 1);
  }

  void dhcpV4Pkt() {
    addValue(*dhcpV4Pkt_, 1);
  }

  void dhcpV6Pkt() {
    addValue(*dhcpV6Pkt_, 1);
  }

  void ipv4Rx() {
    addValue(*ipv4Rx_, 1);
  }
  void ipv4TooSmall() {
    addValue(*ipv4TooSmall_, 1);
  }
  void ipv4WrongVer() {
    addValue(*ipv4WrongVer_, 1);
  }
  void ipv4Nexthop() {
    addValue(*ipv4Nexthop_, 1);
  }
  void ipv4Mine() {
    addValue(*ipv4Mine_, 1);
  }
  void ipv4NoArp() {
    addValue(*ipv4NoArp_, 1);
  }
  void ipv4TtlExceeded() {
    addValue(*ipv4TtlExceeded_, 1);
  }

  void ipv6HopExceeded() {
    addValue(*ipv6HopExceeded_, 1);
  }

  void udpTooSmall() {
    addValue(*udpTooSmall_, 1);
  }

  void dhcpV4BadPkt() {
    addValue(*dhcpV4BadPkt_, 1);
    addValue(*dhcpV4DropPkt_, 1);
    addValue(*trapPktDrops_, 1);
  }

  void dhcpV4DropPkt() {
    addValue(*dhcpV4DropPkt_, 1);
    addValue(*trapPktDrops_, 1);
  }

  void dhcpV6BadPkt() {
    addValue(*dhcpV6BadPkt_, 1);
    addValue(*dhcpV6DropPkt_, 1);
    addValue(*trapPktDrops_, 1);
  }

  void dhcpV6DropPkt() {
    addValue(*dhcpV6DropPkt_, 1);
    addValue(*trapPktDrops_, 1);
  }

  void addRouteV4() {
    addValue(*addRouteV4_, 1);
  }
  void addRouteV6() {
    addValue(*addRouteV6_, 1);
  }
  void delRouteV4() {
    addValue(*delRouteV4_, 1);
  }
  void delRouteV6() {
    addValue(*delRouteV6_, 1);
  }

  void addRoutesV4(uint64_t routeCount) {
    addValue(*addRouteV4_, routeCount);
  }
  void addRoutesV6(uint64_t routeCount) {
    addValue(*addRouteV6_, routeCount);
  }
  void delRoutesV4(uint64_t routeCount) {
    addValue(*delRouteV4_, routeCount);
  }
  void delRoutesV6(uint64_t routeCount) {
    addValue(*delRouteV6_, routeCount);
  }

  void ipv4DstLookupFailure() {
    addValue(*dstLookupFailureV4_, 1);
    addValue(*dstLookupFailure_, 1);
  }

  void ipv6DstLookupFailure() {
    addValue(*dstLookupFailureV6_, 1);
    addValue(*dstLookupFailure_, 1);
  }

  void stateUpdate(std::chrono::microseconds us) {
    addValue(*updateState_, us.count());
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
    addValue(*bgHeartbeatDelay_, delay);
  }

  void updHeartbeatDelay(int delay) {
    addValue(*updHeartbeatDelay_, delay);
  }

  void packetTxHeartbeatDelay(int value) {
    addValue(*packetTxHeartbeatDelay_, value);
  }

  void lacpHeartbeatDelay(int value) {
    addValue(*lacpHeartbeatDelay_, value);
  }

  void neighborCacheHeartbeatDelay(int value) {
    addValue(*neighborCacheHeartbeatDelay_, value);
  }

  void bgEventBacklog(int value) {
    addValue(*bgEventBacklog_, value);
  }

  void updEventBacklog(int value) {
    addValue(*updEventBacklog_, value);
  }

  void lacpEventBacklog(int value) {
    addValue(*lacpEventBacklog_, value);
  }

  void packetTxEventBacklog(int value) {
    addValue(*packetTxEventBacklog_, value);
  }

  void neighborCacheEventBacklog(int value) {
    addValue(*neighborCacheEventBacklog_, value);
  }

  void linkStateChange() {
    addValue(*linkStateChange_, 1);
  }

  void pcapDistFailure() {
    pcapDistFailure_.incrementValue(1);
  }

  void updateStatsException() {
    addValue(*updateStatsExceptions_, 1);
  }

  void pktTooBig() {
    addValue(*trapPktTooBig_, 1);
  }

  void LldpRecvdPkt() {
    addValue(*LldpRecvdPkt_, 1);
  }
  void LldpBadPkt() {
    addValue(*LldpBadPkt_, 1);
  }
  void LldpValidateMisMatch() {
    addValue(*LldpValidateMisMatch_, 1);
  }
  void LldpNeighborsSize(int value) {
    addValue(*LldpNeighborsSize_, value);
  }
  void LacpRxTimeouts() {
    addValue(*LacpRxTimeouts_, 1);
  }
  void LacpMismatchPduTeardown() {
    addValue(*LacpMismatchPduTeardown_, 1);
  }

  void MkPduRecvdPkt() {
    addValue(*MkPduRecvdPkts_, 1);
  }
  void MkPduSendPkt() {
    addValue(*MkPduSendPkts_, 1);
  }
  void MkPduSendFailure() {
    addValue(*MkPduSendFailure_, 1);
  }
  void MkPduPortNotRegistered() {
    addValue(*MkPduPortNotRegistered_, 1);
  }
  void MKAServiceSendFailue() {
    addValue(*MKAServiceSendFailure_, 1);
  }
  void MKAServiceSendSuccess() {
    addValue(*MKAServiceSendSuccess_, 1);
  }
  void MKAServiceRecvSuccess() {
    addValue(*MKAServiceRecvSuccess_, 1);
  }
  void PfcDeadlockDetectionCount() {
    addValue(*pfcDeadlockDetectionCount_, 1);
  }
  void PfcDeadlockRecoveryCount() {
    addValue(*pfcDeadlockRecoveryCount_, 1);
  }
  void ThreadHeartbeatMissCount() {
    addValue(*threadHeartbeatMissCount_, 1);
  }

  typedef fb303::ThreadCachedServiceData::ThreadLocalStatsMap
      ThreadLocalStatsMap;
  typedef fb303::ThreadCachedServiceData::TLTimeseries TLTimeseries;
  typedef fb303::ThreadCachedServiceData::TLHistogram TLHistogram;
  typedef fb303::ThreadCachedServiceData::TLCounter TLCounter;

  using TLTimeseriesPtr = std::unique_ptr<TLTimeseries>;
  using TLHistogramPtr = std::unique_ptr<TLHistogram>;

  // TODO: use template parameter pack once FSDB is open sourced instead of
  // method overloading
  static std::unique_ptr<TLTimeseries> makeTLTimeseries(
      ThreadLocalStatsMap* map,
      std::string&& key,
      fb303::ExportType exportType);
  static std::unique_ptr<TLTimeseries> makeTLTimeseries(
      ThreadLocalStatsMap* map,
      std::string&& key,
      fb303::ExportType exportType1,
      fb303::ExportType exportType2);
  static std::unique_ptr<TLHistogram> makeTLTHistogram(
      ThreadLocalStatsMap* map,
      std::string&& key,
      int64_t bucketWidth,
      int64_t min,
      int64_t max);
  static std::unique_ptr<TLHistogram> makeTLTHistogram(
      ThreadLocalStatsMap* map,
      std::string&& key,
      int64_t bucketWidth,
      int64_t min,
      int64_t max,
      fb303::ExportType exportType,
      int percentile1,
      int percentile2);

  static void addValue(TLTimeseries& stats, int64_t value);
  static void addValue(TLHistogram& stats, int64_t value);

 private:
  // Forbidden copy constructor and assignment operator
  SwitchStats(SwitchStats const&) = delete;
  SwitchStats& operator=(SwitchStats const&) = delete;

  explicit SwitchStats(ThreadLocalStatsMap* map);

  // Total number of trapped packets
  TLTimeseriesPtr trapPkts_;
  // Number of trapped packets that were intentionally dropped.
  TLTimeseriesPtr trapPktDrops_;
  // Malformed packets received
  TLTimeseriesPtr trapPktBogus_;
  // Number of times the controller encountered an error trying to process
  // a packet.
  TLTimeseriesPtr trapPktErrors_;
  // Trapped packets that the controller didn't know how to handle.
  TLTimeseriesPtr trapPktUnhandled_;
  // Trapped packets forwarded to host
  TLTimeseriesPtr trapPktToHost_;
  // Trapped packets forwarded to host in bytes
  TLTimeseriesPtr trapPktToHostBytes_;
  // Packets sent by host
  TLTimeseriesPtr pktFromHost_;
  // Packets sent by host in bytes
  TLTimeseriesPtr pktFromHostBytes_;

  // ARP Packets
  TLTimeseriesPtr trapPktArp_;
  // ARP packets dropped due to unsupported hardware or protocol settings.
  TLTimeseriesPtr arpUnsupported_;
  // ARP packets ignored because we were not the target.
  TLTimeseriesPtr arpNotMine_;
  // ARP requests destined for us
  TLTimeseriesPtr arpRequestsRx_;
  // ARP replies destined for us
  TLTimeseriesPtr arpRepliesRx_;
  // ARP requests sent from us
  TLTimeseriesPtr arpRequestsTx_;
  // ARP replies sent from us
  TLTimeseriesPtr arpRepliesTx_;
  // ARP packets with an unknown op field
  TLTimeseriesPtr arpBadOp_;

  // IPv6 Neighbor Discovery Protocol packets
  TLTimeseriesPtr trapPktNdp_;
  TLTimeseriesPtr ipv6NdpBad_;

  // IPv4 Packets
  TLTimeseriesPtr ipv4Rx_;
  // IPv4 packets dropped due to smaller packet size
  TLTimeseriesPtr ipv4TooSmall_;
  // IPv4 packets dropped due to wrong version number
  TLTimeseriesPtr ipv4WrongVer_;
  // IPv4 packets received for resolving nexthop
  TLTimeseriesPtr ipv4Nexthop_;
  // IPv4 packets received for myself
  TLTimeseriesPtr ipv4Mine_;
  // IPv4 packets received, but not able to send out ARP request
  TLTimeseriesPtr ipv4NoArp_;
  // IPv4 TTL exceeded
  TLTimeseriesPtr ipv4TtlExceeded_;

  // IPv6 hop count exceeded
  TLTimeseriesPtr ipv6HopExceeded_;

  // UDP packets dropped due to smaller packet size
  TLTimeseriesPtr udpTooSmall_;

  // Total DHCP packets received
  TLTimeseriesPtr dhcpV4Pkt_;
  /*
   *  Consolidate DHCP v4 drops under 2 heads
   *  a) Bad dhcp packet receive
   *  b) Catch all dhcp packet drop. This includes a) but also includes
   *  cases where a dhcp relay was not configured for this VLAN
   */
  // DHCPv4 packets dropped due to packet being malformed
  TLTimeseriesPtr dhcpV4BadPkt_;
  // DHCPv4 packets dropped
  TLTimeseriesPtr dhcpV4DropPkt_;
  // stats for dhcpV6
  TLTimeseriesPtr dhcpV6Pkt_;
  TLTimeseriesPtr dhcpV6BadPkt_;
  TLTimeseriesPtr dhcpV6DropPkt_;

  /**
   * Routes add/delete stats
   *
   * Modification is treated as add.
   */
  TLTimeseriesPtr addRouteV4_;
  TLTimeseriesPtr addRouteV6_;
  TLTimeseriesPtr delRouteV4_;
  TLTimeseriesPtr delRouteV6_;

  TLTimeseriesPtr dstLookupFailureV4_;
  TLTimeseriesPtr dstLookupFailureV6_;
  TLTimeseriesPtr dstLookupFailure_;

  /**
   * Histogram for time used for SwSwitch::updateState() (in ms)
   */
  TLHistogramPtr updateState_;

  /**
   * Histogram for time used for route update (in microsecond)
   */
  TLHistogram routeUpdate_;

  /**
   * Background thread heartbeat delay (ms)
   */
  TLHistogramPtr bgHeartbeatDelay_;

  /**
   * Update thread heartbeat delay (ms)
   */
  TLHistogramPtr updHeartbeatDelay_;
  /**
   * Fboss packet Tx thread heartbeat delay (ms)
   */
  TLHistogramPtr packetTxHeartbeatDelay_;
  /**
   * LACP thread heartbeat delay in milliseconds
   */
  TLHistogramPtr lacpHeartbeatDelay_;
  /**
   * Arp Cache thread heartbeat delay in milliseconds
   */
  TLHistogramPtr neighborCacheHeartbeatDelay_;

  /**
   * Number of events queued in background thread
   */
  TLHistogramPtr bgEventBacklog_;

  /**
   * Number of events queued in update thread
   */
  TLHistogramPtr updEventBacklog_;
  /**
   * Number of events queued in fboss packet TX thread
   */
  TLHistogramPtr packetTxEventBacklog_;

  TLHistogramPtr lacpEventBacklog_;
  /**
   * Number of events queued in fboss Arp Cache thread
   */
  TLHistogramPtr neighborCacheEventBacklog_;

  /**
   * Link state up/down change count
   */
  TLTimeseriesPtr linkStateChange_;

  // Individual port stats objects, indexed by PortID
  PortStatsMap ports_;

  AggregatePortStatsMap aggregatePortIDToStats_;

  // Number of packets dropped by the PCAP distribution service
  TLCounter pcapDistFailure_;

  // Number of failed updateStats callbacks do to exceptions.
  TLTimeseriesPtr updateStatsExceptions_;

  // Number of packet too big ICMPv6 triggered
  TLTimeseriesPtr trapPktTooBig_;

  // Number of LLDP packets.
  TLTimeseriesPtr LldpRecvdPkt_;
  // Number of bad LLDP packets.
  TLTimeseriesPtr LldpBadPkt_;
  // Number of LLDP packets that did not match configured, expected values.
  TLTimeseriesPtr LldpValidateMisMatch_;
  // Number of LLDP Neighbors.
  TLTimeseriesPtr LldpNeighborsSize_;

  // Number of LACP Rx timeouts
  TLTimeseriesPtr LacpRxTimeouts_;
  // Number of LACP session teardown due to mismatching PDUs
  TLTimeseriesPtr LacpMismatchPduTeardown_;
  // Number of MkPdu Received.
  TLTimeseriesPtr MkPduRecvdPkts_;
  // Number of MkPdu Send.
  TLTimeseriesPtr MkPduSendPkts_;
  // Number of MkPdu Send Failure
  TLTimeseriesPtr MkPduSendFailure_;
  // Number of pkt recv when port is not registered.
  TLTimeseriesPtr MkPduPortNotRegistered_;
  // Number of Send Pkt to MkaService Failure.
  TLTimeseriesPtr MKAServiceSendFailure_;
  // Number of Pkts Send to MKAService.
  TLTimeseriesPtr MKAServiceSendSuccess_;
  // Number of pkts recvd from MkaService.
  TLTimeseriesPtr MKAServiceRecvSuccess_;
  // Number of timers pfc deadlock watchodg was hit
  TLTimeseriesPtr pfcDeadlockDetectionCount_;
  // Number of timers pfc deadlock recovery hit
  TLTimeseriesPtr pfcDeadlockRecoveryCount_;
  // Number of thread heartbeat misses
  TLTimeseriesPtr threadHeartbeatMissCount_;
};

} // namespace facebook::fboss
