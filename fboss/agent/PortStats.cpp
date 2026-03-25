/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <fb303/ThreadCachedServiceData.h>
#include "fboss/agent/SwitchStats.h"

using facebook::fb303::SUM;

namespace facebook::fboss {

const std::string kNameKeySeperator = ".";
const std::string kUp = "up";
const std::string kLinkStateFlap = "link_state.flap";
const std::string kActive = "active";
const std::string kLinkActiveStateFlap = "link_active_state.flap";
const std::string kPfcDeadlockDetectionCount = "pfc_deadlock_detection";
const std::string kPfcDeadlockRecoveryCount = "pfc_deadlock_recovery";
const std::string kLoadBearingInErrors = "load_bearing_in_errors";
const std::string kLoadBearingFecUncorrErrors =
    "load_bearing_fec_uncorrectable_errors";
const std::string kLoadBearingLinkStateFlap = "load_bearing_link_state.flap";
const std::string kFabricLinkMonitoringRxPackets =
    "fabric_link_monitoring_rx_packets";
const std::string kFabricLinkMonitoringTxPackets =
    "fabric_link_monitoring_tx_packets";

PortStats::PortStats(
    PortID portID,
    std::string portName,
    SwitchStats* switchStats)
    : portID_(portID), portName_(portName), switchStats_(switchStats) {
  if (!portName_.empty()) {
    tcData().addStatValue(getCounterKey(kLinkStateFlap), 0, SUM);
    tcData().addStatValue(getCounterKey(kLinkActiveStateFlap), 0, SUM);
    tcData().addStatValue(
        getCounterKey(kFabricLinkMonitoringRxPackets), 0, SUM);
    tcData().addStatValue(
        getCounterKey(kFabricLinkMonitoringTxPackets), 0, SUM);
  }
}

PortStats::~PortStats() {}

void PortStats::clearCounters() const {
  clearPortStatusCounter();
  clearPortActiveStatusCounter();
  tcData().clearCounter(getCounterKey(kLoadBearingInErrors));
  tcData().clearCounter(getCounterKey(kLoadBearingFecUncorrErrors));
}

void PortStats::trappedPkt() const {
  switchStats_->trappedPkt();
}
void PortStats::pktDropped() const {
  switchStats_->pktDropped();
}
void PortStats::pktBogus() const {
  switchStats_->pktBogus();
}
void PortStats::pktError() const {
  switchStats_->pktError();
}
void PortStats::pktUnhandled() const {
  switchStats_->pktUnhandled();
}
void PortStats::pktToHost(uint32_t bytes) const {
  switchStats_->pktToHost(bytes);
}

void PortStats::arpPkt() const {
  switchStats_->arpPkt();
}
void PortStats::arpUnsupported() const {
  switchStats_->arpUnsupported();
}
void PortStats::arpNotMine() const {
  switchStats_->arpNotMine();
}
void PortStats::arpRequestRx() const {
  switchStats_->arpRequestRx();
}
void PortStats::arpRequestTx() const {
  switchStats_->arpRequestTx();
}
void PortStats::arpReplyRx() const {
  switchStats_->arpReplyRx();
}
void PortStats::arpReplyTx() const {
  switchStats_->arpReplyTx();
}
void PortStats::arpBadOp() const {
  switchStats_->arpBadOp();
}

void PortStats::ipv6NdpPkt() const {
  switchStats_->ipv6NdpPkt();
}
void PortStats::ipv6NdpBad() const {
  switchStats_->ipv6NdpBad();
}

void PortStats::ipv4Rx() const {
  switchStats_->ipv4Rx();
}
void PortStats::ipv4TooSmall() const {
  switchStats_->ipv4TooSmall();
}
void PortStats::ipv4WrongVer() const {
  switchStats_->ipv4WrongVer();
}
void PortStats::ipv4Nexthop() const {
  switchStats_->ipv4Nexthop();
}
void PortStats::ipv4Mine() const {
  switchStats_->ipv4Mine();
}
void PortStats::ipv4NoArp() const {
  switchStats_->ipv4NoArp();
}
void PortStats::ipv4TtlExceeded() const {
  switchStats_->ipv4TtlExceeded();
}

void PortStats::ipv6HopExceeded() const {
  switchStats_->ipv6HopExceeded();
}

void PortStats::udpTooSmall() const {
  switchStats_->udpTooSmall();
}

void PortStats::dhcpV4Pkt() const {
  switchStats_->dhcpV4Pkt();
}
void PortStats::dhcpV4BadPkt() const {
  switchStats_->dhcpV4BadPkt();
}
void PortStats::dhcpV4DropPkt() const {
  switchStats_->dhcpV4DropPkt();
}

void PortStats::dhcpV6Pkt() const {
  switchStats_->dhcpV6Pkt();
}
void PortStats::dhcpV6BadPkt() const {
  switchStats_->dhcpV6BadPkt();
}
void PortStats::dhcpV6DropPkt() const {
  switchStats_->dhcpV6DropPkt();
}

void PortStats::updateLoadBearingTLStatValue(
    const std::string& counter,
    bool isDrained,
    std::optional<bool> activeState,
    int64_t val) const {
  if (activeState.has_value()) {
    if (!isDrained && *activeState) {
      // not drained and active (peer not drained) - log value
      tcData().addStatValue(getCounterKey(counter), val, SUM);
    } else {
      // local drain or peer drained. No load bearing impact
      tcData().addStatValue(getCounterKey(counter), 0, SUM);
    }
  }
}

void PortStats::linkStateChange(
    bool isUp,
    bool isDrained,
    std::optional<bool> activeState) const {
  // We decided not to maintain the TLTimeseries in PortStats and use tcData()
  // to addStatValue based on the key name, because:
  // 1) each thread has its own SwitchStats and PortStats
  // 2) update PortName need to delete old TLTimeseries in the thread which
  // can recognize the name changed.
  // 3) w/o a global lock like the one in ThreadLocalStats, we might have
  // race condition issue when two threads want to delete the same TLTimeseries
  // from the same thread.
  // Using tcData() can make sure we don't have to maintain the lifecycle of
  // TLTimeseries and leave ThreadLocalStats do it for us.
  if (!portName_.empty()) {
    tcData().addStatValue(getCounterKey(kLinkStateFlap), 1, SUM);
    updateLoadBearingTLStatValue(
        kLoadBearingLinkStateFlap, isDrained, activeState, 1);
  }
  switchStats_->linkStateChange();
}

void PortStats::linkActiveStateChange(bool isActive) const {
  if (!portName_.empty()) {
    tcData().addStatValue(getCounterKey(kLinkActiveStateFlap), 1, SUM);
  }
  switchStats_->linkActiveStateChange();
}

void PortStats::pfcDeadlockDetectionCount() const {
  if (!portName_.empty()) {
    tcData().addStatValue(getCounterKey(kPfcDeadlockDetectionCount), 1, SUM);
  }
  switchStats_->PfcDeadlockDetectionCount();
}

void PortStats::pfcDeadlockRecoveryCount() const {
  if (!portName_.empty()) {
    tcData().addStatValue(getCounterKey(kPfcDeadlockRecoveryCount), 1, SUM);
  }
  switchStats_->PfcDeadlockRecoveryCount();
}

void PortStats::fabricLinkMonitoringRxPackets(int64_t count) {
  if (!portName_.empty()) {
    tcData().addStatValue(
        getCounterKey(kFabricLinkMonitoringRxPackets),
        count - curFabricLinkMonitoringRxPackets_,
        SUM);
  }
  curFabricLinkMonitoringRxPackets_ = count;
}

void PortStats::fabricLinkMonitoringTxPackets(int64_t count) {
  if (!portName_.empty()) {
    tcData().addStatValue(
        getCounterKey(kFabricLinkMonitoringTxPackets),
        count - curFabricLinkMonitoringTxPackets_,
        SUM);
  }
  curFabricLinkMonitoringTxPackets_ = count;
}

void PortStats::ipv4DstLookupFailure() const {
  switchStats_->ipv4DstLookupFailure();
}

void PortStats::ipv6DstLookupFailure() const {
  switchStats_->ipv6DstLookupFailure();
}

void PortStats::setPortStatus(bool isUp) const {
  if (!portName_.empty()) {
    tcData().setCounter(getCounterKey(kUp), isUp);
  }
}

void PortStats::clearPortStatusCounter() const {
  if (!portName_.empty()) {
    tcData().clearCounter(getCounterKey(kUp));
  }
}

void PortStats::setPortActiveStatus(bool isActive) const {
  if (!portName_.empty()) {
    tcData().setCounter(getCounterKey(kActive), isActive);
  }
}

void PortStats::clearPortActiveStatusCounter() const {
  if (!portName_.empty()) {
    tcData().clearCounter(getCounterKey(kActive));
  }
}

void PortStats::pktTooBig() const {
  switchStats_->pktTooBig();
}

void PortStats::MkPduRecvdPkt() const {
  switchStats_->MkPduRecvdPkt();
}

void PortStats::MkPduSendPkt() const {
  switchStats_->MkPduSendPkt();
}
void PortStats::MkPduSendFailure() const {
  switchStats_->MkPduSendFailure();
}
void PortStats::MkPduPortNotRegistered() const {
  switchStats_->MkPduPortNotRegistered();
}

void PortStats::MkPduInterval() {
  auto now = std::chrono::steady_clock::now();
  std::chrono::steady_clock::time_point epoch;
  if (lastMkPduTime_ != epoch) {
    auto diffMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                      now - lastMkPduTime_)
                      .count();
    if (!portName_.empty()) {
      switchStats_->MkPduInterval(diffMs, portName_);
    }
  }

  lastMkPduTime_ = now;
}

void PortStats::MKAServiceSendFailue() const {
  switchStats_->MKAServiceSendFailue();
}
void PortStats::MKAServiceSendSuccess() const {
  switchStats_->MKAServiceSendSuccess();
}
void PortStats::MKAServiceRecvSuccess() const {
  switchStats_->MKAServiceRecvSuccess();
}

std::string PortStats::getCounterKey(const std::string& key) const {
  return folly::to<std::string>(portName_, kNameKeySeperator, key);
}

void PortStats::inErrors(
    int64_t inErrors,
    bool isDrained,
    std::optional<bool> activeState) {
  updateLoadBearingTLStatValue(
      kLoadBearingInErrors, isDrained, activeState, inErrors - curInErrors_);
  curInErrors_ = inErrors;
}

void PortStats::fecUncorrectableErrors(
    int64_t fecUncorrectableErrors,
    bool isDrained,
    std::optional<bool> activeState) {
  updateLoadBearingTLStatValue(
      kLoadBearingFecUncorrErrors,
      isDrained,
      activeState,
      fecUncorrectableErrors - curFecUncorrectableErrors_);
  curFecUncorrectableErrors_ = fecUncorrectableErrors;
}

} // namespace facebook::fboss
