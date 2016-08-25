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

namespace facebook { namespace fboss {

PortStats::PortStats(PortID portID, SwitchStats *switchStats)
  : portID_(portID),
    switchStats_(switchStats) {
}

void PortStats::trappedPkt() {
  switchStats_->trappedPkt();
}
void PortStats::pktDropped() {
  switchStats_->pktDropped();
}
void PortStats::pktBogus() {
  switchStats_->pktBogus();
}
void PortStats::pktError() {
  switchStats_->pktError();
}
void PortStats::pktUnhandled() {
  switchStats_->pktUnhandled();
}
void PortStats::pktToHost(uint32_t bytes) {
  switchStats_->pktToHost(bytes);
}

void PortStats::arpPkt() {
  switchStats_->arpPkt();
}
void PortStats::arpUnsupported() {
  switchStats_->arpUnsupported();
}
void PortStats::arpNotMine() {
  switchStats_->arpNotMine();
}
void PortStats::arpRequestRx() {
  switchStats_->arpRequestRx();
}
void PortStats::arpRequestTx() {
  switchStats_->arpRequestTx();
}
void PortStats::arpReplyRx() {
  switchStats_->arpReplyRx();
}
void PortStats::arpReplyTx() {
  switchStats_->arpReplyTx();
}
void PortStats::arpBadOp() {
  switchStats_->arpBadOp();
}

void PortStats::ipv6NdpPkt() {
  switchStats_->ipv6NdpPkt();
}
void PortStats::ipv6NdpBad() {
  switchStats_->ipv6NdpBad();
}

void PortStats::ipv4Rx() {
  switchStats_->ipv4Rx();
}
void PortStats::ipv4TooSmall() {
  switchStats_->ipv4TooSmall();
}
void PortStats::ipv4WrongVer() {
  switchStats_->ipv4WrongVer();
}
void PortStats::ipv4Nexthop() {
  switchStats_->ipv4Nexthop();
}
void PortStats::ipv4Mine() {
  switchStats_->ipv4Mine();
}
void PortStats::ipv4NoArp() {
  switchStats_->ipv4NoArp();
}
void PortStats::ipv4TtlExceeded() {
  switchStats_->ipv4TtlExceeded();
}

void PortStats::ipv6HopExceeded() {
  switchStats_->ipv6HopExceeded();
}

void PortStats::udpTooSmall() {
  switchStats_->udpTooSmall();
}

void PortStats::dhcpV4Pkt() {
  switchStats_->dhcpV4Pkt();
}
void PortStats::dhcpV4BadPkt() {
  switchStats_->dhcpV4BadPkt();
}
void PortStats::dhcpV4DropPkt() {
  switchStats_->dhcpV4DropPkt();
}

void PortStats::dhcpV6Pkt() {
  switchStats_->dhcpV6Pkt();
}
void PortStats::dhcpV6BadPkt() {
  switchStats_->dhcpV6BadPkt();
}
void PortStats::dhcpV6DropPkt() {
  switchStats_->dhcpV6DropPkt();
}

void PortStats::linkStateChange() {
  switchStats_->linkStateChange();
}

}} // facebook::fboss
