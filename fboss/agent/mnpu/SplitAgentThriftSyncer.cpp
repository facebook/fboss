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
#include "fboss/agent/mnpu/LinkEventSyncer.h"
#include "fboss/agent/mnpu/OperDeltaSyncer.h"
#include "fboss/agent/mnpu/TxPktEventSyncer.h"

namespace facebook::fboss {

SplitAgentThriftSyncer::SplitAgentThriftSyncer(
    HwSwitch* hw,
    uint16_t serverPort)
    : retryThread_(std::make_shared<folly::ScopedEventBaseThread>(
          "SplitAgentThriftRetryThread")),
      switchId_(hw->getSwitchID()),
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
          std::make_unique<OperDeltaSyncer>(serverPort, switchId_, hw)) {}

void SplitAgentThriftSyncer::packetReceived(
    std::unique_ptr<RxPacket> /* pkt */) noexcept {
  // TODO - Add handler
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
    L2Entry /* l2Entry */,
    L2EntryUpdateType /* l2EntryUpdateType */) {
  // TODO - Add handler
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
}

void SplitAgentThriftSyncer::stop() {
  // Stop any started services
  linkEventSinkClient_->cancel();
  txPktEventStreamClient_->cancel();
  operDeltaClient_->stopOperSync();
}

SplitAgentThriftSyncer::~SplitAgentThriftSyncer() {
  stop();
}
} // namespace facebook::fboss
