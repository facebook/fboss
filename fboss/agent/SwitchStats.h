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

#include <chrono>
#include <boost/container/flat_map.hpp>
#include <boost/noncopyable.hpp>
#include "common/stats/ThreadCachedServiceData.h"
#include "fboss/agent/PortStats.h"
#include "fboss/agent/types.h"

namespace facebook { namespace fboss {

class PortStats;

typedef boost::container::flat_map<PortID,
          std::unique_ptr<PortStats>> PortStatsMap;

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
  PortStats* port(PortID portID);

  /*
   * Getters.
   */
  PortStatsMap* getPortStats() {
    return &ports_;
  }
  const PortStatsMap* getPortStats() const {
    return &ports_;
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

  void stateUpdate(std::chrono::microseconds us) {
    updateState_.addValue(us.count());
  }

  void routeUpdate(std::chrono::microseconds us, uint64_t routes) {
    // As syncFib() could include no routes.
    if (routes == 0) {
      routes = 1;
    }
    routeUpdate_.addRepeatedValue(us.count() / routes, routes);
  }

 private:
  // Forbidden copy constructor and assignment operator
  SwitchStats(SwitchStats const &) = delete;
  SwitchStats& operator=(SwitchStats const &) = delete;

  typedef
  stats::ThreadCachedServiceData::ThreadLocalStatsMap ThreadLocalStatsMap;
  typedef stats::ThreadCachedServiceData::TLTimeseries TLTimeseries;
  typedef stats::ThreadCachedServiceData::TLHistogram TLHistogram;

  explicit SwitchStats(ThreadLocalStatsMap *map);

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

  /**
   * Histogram for time used for SwSwitch::updateState() (in ms)
   */
  TLHistogram updateState_;

  /**
   * Histogram for time used for route update (in microsecond)
   */
  TLHistogram routeUpdate_;

  // Create a PortStats object for the given PortID
  PortStats* createPortStats(PortID portID);

  // Individual port stats objects, indexed by PortID
  PortStatsMap ports_;
};

}} // facebook::fboss
