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

#include <folly/IPAddress.h>
#include <thrift/lib/cpp2/async/PooledRequestChannel.h>
#include <thrift/lib/cpp2/async/RocketClientChannel.h>

static constexpr folly::StringPiece kClientName = "mnpu-syncer-client";

namespace facebook::fboss {

SplitAgentThriftSyncer::SplitAgentThriftSyncer(
    HwSwitch* hw,
    uint16_t serverPort)
    : hw_(hw), serverPort_(serverPort) {}

void SplitAgentThriftSyncer::packetReceived(
    std::unique_ptr<RxPacket> /* pkt */) noexcept {
  // TODO - Add handler
}

void SplitAgentThriftSyncer::linkStateChanged(
    PortID /* port */,
    bool /* up */,
    std::optional<phy::LinkFaultStatus> /* iPhyFaultStatus */) {
  // TODO - Add handler
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

void SplitAgentThriftSyncer::connect() {
  evbThread_ = std::make_shared<folly::ScopedEventBaseThread>(kClientName);
  auto channel = apache::thrift::PooledRequestChannel::newChannel(
      evbThread_->getEventBase(),
      evbThread_,
      [this](folly::EventBase& evb) mutable {
        return apache::thrift::RocketClientChannel::newChannel(
            folly::AsyncSocket::UniquePtr(
                new folly::AsyncSocket(&evb, "::1", serverPort_)));
      });
  multiSwitchClient_ =
      std::make_unique<apache::thrift::Client<multiswitch::MultiSwitchCtrl>>(
          std::move(channel));
}

} // namespace facebook::fboss
