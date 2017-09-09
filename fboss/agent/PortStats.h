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

#include "common/stats/ThreadCachedServiceData.h"
#include "fboss/agent/types.h"

namespace facebook { namespace fboss {
class SwitchStats;

class PortStats {
 public:
  PortStats(PortID portID, std::string portName, SwitchStats *switchStats);

  void trappedPkt();
  void pktDropped();
  void pktBogus();
  void pktError();
  void pktUnhandled();
  void pktToHost(uint32_t bytes); // number of packets forward to host

  void arpPkt();
  void arpUnsupported();
  void arpNotMine();
  void arpRequestRx();
  void arpRequestTx();
  void arpReplyRx();
  void arpReplyTx();
  void arpBadOp();

  void ipv6NdpPkt();
  void ipv6NdpBad();

  void ipv4Rx();
  void ipv4TooSmall();
  void ipv4WrongVer();
  void ipv4Nexthop();
  void ipv4Mine();
  void ipv4NoArp();
  void ipv4TtlExceeded();
  void udpTooSmall();

  void ipv6HopExceeded();

  void dhcpV4Pkt();
  void dhcpV4BadPkt();
  void dhcpV4DropPkt();
  void dhcpV6Pkt();
  void dhcpV6BadPkt();
  void dhcpV6DropPkt();

  void linkStateChange();

  void ipv4DstLookupFailure();
  void ipv6DstLookupFailure();

  void setPortStatusCounter(bool isUp);
  void clearPortStatusCounter();

  void updateName(const std::string& newName);
  std::string getName() {
    return portName_;
  }

 private:
  // Forbidden copy constructor and assignment operator
  PortStats(PortStats const &) = delete;
  PortStats& operator=(PortStats const &) = delete;

  using ThreadLocalStatsMap =
    stats::ThreadCachedServiceData::ThreadLocalStatsMap;
  using TLTimeseries = stats::ThreadCachedServiceData::TLTimeseries;

  void reinitPortStats();

  std::string getCounterKey(const std::string& key);

  /*
   * It's useful to store this
   */
  PortID portID_;

  std::string portName_;

  // Pointer to main SwitchStats object so that we can forward method calls
  // that we do not want to track ourselves.
  SwitchStats *switchStats_;

  /*
   * Port flap count for the specific port
   */
  std::unique_ptr<TLTimeseries> linkStateChange_;
};

}} // facebook::fboss
