/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/mnpu/SplitAgentThriftSyncer.h"
#include "fboss/agent/HwSwitch.h"
#include "fboss/agent/mnpu/FdbEventSyncer.h"
#include "fboss/agent/mnpu/HwSwitchStatsSinkClient.h"
#include "fboss/agent/mnpu/LinkEventSyncer.h"
#include "fboss/agent/mnpu/OperDeltaSyncer.h"
#include "fboss/agent/mnpu/RxPktEventSyncer.h"
#include "fboss/agent/mnpu/TxPktEventSyncer.h"

namespace facebook::fboss {

SplitAgentThriftSyncer::SplitAgentThriftSyncer(
    HwSwitch* hw,
    uint16_t serverPort,
    SwitchID switchId,
    uint16_t switchIndex)
    : retryThread_(std::make_shared<folly::ScopedEventBaseThread>(
          "SplitAgentThriftRetryThread")),
      switchId_(switchId),
      linkEventSinkClient_(std::make_unique<LinkEventSyncer>(
          serverPort,
          switchId_,
          retryThread_->getEventBase())),
      txPktEventStreamClient_(std::make_unique<TxPktEventSyncer>(
          serverPort,
          switchId_,
          retryThread_->getEventBase(),
          hw)),
      operDeltaClient_(
          std::make_unique<OperDeltaSyncer>(serverPort, switchId_, hw)),
      fdbEventSinkClient_(std::make_unique<FdbEventSyncer>(
          serverPort,
          switchId_,
          retryThread_->getEventBase())),
      rxPktEventSinkClient_(std::make_unique<RxPktEventSyncer>(
          serverPort,
          switchId_,
          retryThread_->getEventBase())),
      hwSwitchStatsSinkClient_(std::make_unique<HwSwitchStatsSinkClient>(
          serverPort,
          switchId_,
          switchIndex,
          retryThread_->getEventBase())) {}

void SplitAgentThriftSyncer::packetReceived(
    std::unique_ptr<RxPacket> pkt) noexcept {
  multiswitch::RxPacket rxPkt;
  rxPkt.port() = pkt->getSrcPort();
  if (pkt->getSrcVlanIf()) {
    rxPkt.vlan() = pkt->getSrcVlan();
  }
  if (pkt->getSrcAggregatePort()) {
    rxPkt.aggPort() = pkt->getSrcAggregatePort();
  }
  rxPkt.data() = Packet::extractIOBuf(std::move(pkt));
  rxPktEventSinkClient_->enqueue(std::move(rxPkt));
}

void SplitAgentThriftSyncer::linkStateChanged(
    PortID port,
    bool up,
    std::optional<phy::LinkFaultStatus> iPhyFaultStatus) {
  multiswitch::LinkEvent event;
  event.port() = port;
  event.up() = up;
  if (iPhyFaultStatus) {
    event.iPhyLinkFaultStatus() = *iPhyFaultStatus;
  }
  linkEventSinkClient_->enqueue(std::move(event));
}

void SplitAgentThriftSyncer::l2LearningUpdateReceived(
    L2Entry l2Entry,
    L2EntryUpdateType l2EntryUpdateType) {
  multiswitch::FdbEvent event;
  L2EntryThrift entry;
  entry.mac() = l2Entry.getMac().toString();
  entry.vlanID() = l2Entry.getVlanID();
  entry.l2EntryType() =
      (l2Entry.getType() == L2Entry::L2EntryType::L2_ENTRY_TYPE_PENDING)
      ? L2EntryType::L2_ENTRY_TYPE_PENDING
      : L2EntryType::L2_ENTRY_TYPE_VALIDATED;
  if (l2Entry.getPort().isAggregatePort()) {
    entry.trunk() = 1;
    entry.port() = l2Entry.getPort().aggPortID();
  } else {
    entry.port() = l2Entry.getPort().phyPortID();
  }
  if (l2Entry.getClassID()) {
    entry.classID() = static_cast<int>(l2Entry.getClassID().value());
  }
  event.entry() = std::move(entry);
  event.updateType() = l2EntryUpdateType;
  fdbEventSinkClient_->enqueue(std::move(event));
}

void SplitAgentThriftSyncer::updateHwSwitchStats(
    multiswitch::HwSwitchStats stats) {
  if (hwSwitchStatsSinkClient_->isConnectedToServer()) {
    hwSwitchStatsSinkClient_->enqueue(std::move(stats));
  }
}

void SplitAgentThriftSyncer::exitFatal() const noexcept {
  // TODO - Add handler
}

void SplitAgentThriftSyncer::pfcWatchdogStateChanged(
    const PortID& /* port */,
    const bool /* deadlock */) {
  // TODO - Add handler
}

void SplitAgentThriftSyncer::registerStateObserver(
    StateObserver* /* observer */,
    const std::string& /* name */) {
  // TODO - Add handler
}

void SplitAgentThriftSyncer::unregisterStateObserver(
    StateObserver* /* observer */) {
  // TODO - Add handler
}

void SplitAgentThriftSyncer::start() {
  // Start any required services
  operDeltaClient_->startOperSync();
  isRunning_ = true;
}

void SplitAgentThriftSyncer::stop() {
  // Stop any started services
  linkEventSinkClient_->cancel();
  txPktEventStreamClient_->cancel();
  operDeltaClient_->stopOperSync();
  fdbEventSinkClient_->cancel();
  rxPktEventSinkClient_->cancel();
  hwSwitchStatsSinkClient_->cancel();
  isRunning_ = false;
}

SplitAgentThriftSyncer::~SplitAgentThriftSyncer() {
  if (isRunning_) {
    stop();
  }
}
} // namespace facebook::fboss
