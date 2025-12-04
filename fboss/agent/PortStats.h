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

#include "fboss/agent/types.h"

namespace facebook::fboss {

class SwitchStats;

class PortStats {
 public:
  PortStats(PortID portID, std::string portName, SwitchStats* switchStats);
  ~PortStats();

  void clearCounters() const;

  void trappedPkt() const;
  void pktDropped() const;
  void pktBogus() const;
  void pktError() const;
  void pktUnhandled() const;
  void pktToHost(uint32_t bytes) const; // number of packets forward to host

  void arpPkt() const;
  void arpUnsupported() const;
  void arpNotMine() const;
  void arpRequestRx() const;
  void arpRequestTx() const;
  void arpReplyRx() const;
  void arpReplyTx() const;
  void arpBadOp() const;

  void ipv6NdpPkt() const;
  void ipv6NdpBad() const;

  void ipv4Rx() const;
  void ipv4TooSmall() const;
  void ipv4WrongVer() const;
  void ipv4Nexthop() const;
  void ipv4Mine() const;
  void ipv4NoArp() const;
  void ipv4TtlExceeded() const;
  void udpTooSmall() const;

  void ipv6HopExceeded() const;

  void dhcpV4Pkt() const;
  void dhcpV4BadPkt() const;
  void dhcpV4DropPkt() const;
  void dhcpV6Pkt() const;
  void dhcpV6BadPkt() const;
  void dhcpV6DropPkt() const;

  void linkStateChange(
      bool isUp,
      bool isDrained,
      std::optional<bool> activeState) const;
  void linkActiveStateChange(bool isActive) const;

  void ipv4DstLookupFailure() const;
  void ipv6DstLookupFailure() const;

  void setPortStatus(bool isUp) const;
  void clearPortStatusCounter() const;

  void setPortActiveStatus(bool isActive) const;
  void clearPortActiveStatusCounter() const;

  void pktTooBig() const;

  std::string getPortName() const {
    return portName_;
  }
  PortID getPortId() const {
    return portID_;
  }

  void MkPduRecvdPkt() const;
  void MkPduSendPkt() const;
  void MkPduSendFailure() const;
  void MkPduPortNotRegistered() const;
  // Non-const as it accesses lastMkPduTime_, should only be called from the
  // stats update thread.
  void MkPduInterval();
  void MKAServiceSendFailue() const;
  void MKAServiceSendSuccess() const;
  void MKAServiceRecvSuccess() const;

  void pfcDeadlockRecoveryCount() const;
  void pfcDeadlockDetectionCount() const;

  // Fabric link monitoring stats
  // Non-const as they access previous packet counts to calculate deltas
  void fabricLinkMonitoringRxPackets(int64_t count);
  void fabricLinkMonitoringTxPackets(int64_t count);

  // Non-const as it accesses curInErrors_, should only be called from the
  // stats update thread.
  void
  inErrors(int64_t inErrors, bool isDrained, std::optional<bool> activeState);

  // Non-const as it accesses curFecUncorrectableErrors_, should only be called
  // from the stats update thread.
  void fecUncorrectableErrors(
      int64_t fecUncorrectableErrors,
      bool isDrained,
      std::optional<bool> activeState);

 private:
  // Forbidden copy constructor and assignment operator
  PortStats(PortStats const&) = delete;
  PortStats& operator=(PortStats const&) = delete;
  PortStats(PortStats&&) = delete;
  PortStats& operator=(PortStats&&) = delete;

  std::string getCounterKey(const std::string& key) const;
  void updateLoadBearingTLStatValue(
      const std::string& counter,
      bool isDrained,
      std::optional<bool> activeState,
      int64_t val) const;

  /*
   * It's useful to store this
   */
  const PortID portID_;

  /*
   * Port name format should be ethX/Y/Z
   */
  const std::string portName_;

  // Pointer to main SwitchStats object so that we can forward method calls
  // that we do not want to track ourselves.
  SwitchStats* const switchStats_;

  std::chrono::steady_clock::time_point lastMkPduTime_;
  int64_t curInErrors_{0};
  int64_t curFecUncorrectableErrors_{0};
  int64_t curFabricLinkMonitoringRxPackets_{0};
  int64_t curFabricLinkMonitoringTxPackets_{0};
};

} // namespace facebook::fboss
