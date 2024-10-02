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

  void
  linkStateChange(bool isUp, bool isDrained, std::optional<bool> activeState);
  void linkActiveStateChange(bool isActive);

  void ipv4DstLookupFailure();
  void ipv6DstLookupFailure();

  void setPortStatus(bool isUp);
  void clearPortStatusCounter();

  void setPortActiveStatus(bool isActive);
  void clearPortActiveStatusCounter();

  void pktTooBig();

  void setPortName(const std::string& portName);
  std::string getPortName() const {
    return portName_;
  }
  PortID getPortId() const {
    return portID_;
  }

  void MkPduRecvdPkt();
  void MkPduSendPkt();
  void MkPduSendFailure();
  void MkPduPortNotRegistered();
  void MkPduInterval();
  void MKAServiceSendFailue();
  void MKAServiceSendSuccess();
  void MKAServiceRecvSuccess();

  void pfcDeadlockRecoveryCount();
  void pfcDeadlockDetectionCount();
  void
  inErrors(int64_t inErrors, bool isDrained, std::optional<bool> activeState);

  void fecUncorrectableErrors(
      int64_t fecUncorrectableErrors,
      bool isDrained,
      std::optional<bool> activeState);

 private:
  // Forbidden copy constructor and assignment operator
  PortStats(PortStats const&) = delete;
  PortStats& operator=(PortStats const&) = delete;

  std::string getCounterKey(const std::string& key) const;
  void updateLoadBearingTLStatValue(
      const std::string& counter,
      bool isDrained,
      std::optional<bool> activeState,
      int64_t val) const;

  /*
   * It's useful to store this
   */
  PortID portID_;

  /*
   * Port name format should be ethX/Y/Z
   */
  std::string portName_;

  // Pointer to main SwitchStats object so that we can forward method calls
  // that we do not want to track ourselves.
  SwitchStats* switchStats_;

  std::chrono::steady_clock::time_point lastMkPduTime_;
  int64_t curInErrors_{0};
  int64_t curFecUncorrectableErrors_{0};
};

} // namespace facebook::fboss
