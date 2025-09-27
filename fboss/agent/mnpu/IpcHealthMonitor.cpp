/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/mnpu/IpcHealthMonitor.h"

#include <fb303/ServiceData.h>
#include <folly/logging/xlog.h>

namespace facebook::fboss {

IpcHealthMonitor::IpcHealthMonitor(
    const std::string& clientId,
    const std::string& counterPrefix,
    folly::EventBase* evb)
    : clientId_(clientId),
      counterPrefix_(counterPrefix),
      evb_(evb),
      // Initialize fb303 TimeseriesWrapper objects for efficient metric
      // tracking
      eventsSent_(counterPrefix + ".ipc.events_sent", fb303::SUM, fb303::RATE),
      eventsDropped_(
          counterPrefix + ".ipc.events_dropped",
          fb303::SUM,
          fb303::RATE),
      eventsReceived_(
          counterPrefix + ".ipc.events_received",
          fb303::SUM,
          fb303::RATE),
      queueDepth_(counterPrefix + ".ipc.queue_depth", fb303::AVG) {}

IpcHealthMonitor::~IpcHealthMonitor() {
  stop();
}

void IpcHealthMonitor::start() {
  if (running_.exchange(true)) {
    return; // Already running
  }

  XLOG(INFO) << "Starting IPC health monitoring for " << clientId_;
}

void IpcHealthMonitor::stop() {
  if (running_.exchange(false)) {
    XLOG(INFO) << "Stopping IPC health monitoring for " << clientId_;
  }
}

void IpcHealthMonitor::trackEventSent() {
  eventsSent_.add(1);

  {
    auto metrics = metrics_.wlock();
    metrics->eventsSent++;
  }
}

void IpcHealthMonitor::trackEventDropped() {
  eventsDropped_.add(1);

  {
    auto metrics = metrics_.wlock();
    metrics->eventsDropped++;
  }
}

void IpcHealthMonitor::trackEventReceived() {
  eventsReceived_.add(1);

  {
    auto metrics = metrics_.wlock();
    metrics->eventsReceived++;
  }
}

void IpcHealthMonitor::trackQueueDepth(int64_t depth) {
  {
    auto metrics = metrics_.wlock();
    metrics->queueDepth = depth;
  }
}

void IpcHealthMonitor::trackStateTransition(
    IpcConnectionState oldState,
    IpcConnectionState newState) {
  auto metrics = metrics_.wlock();
  metrics->connectionState = newState;
  metrics->lastStateTransitionTimestamp = getCurrentTimestampMs();

  if (oldState == IpcConnectionState::CONNECTED &&
      newState != IpcConnectionState::CONNECTED) {
    // Track disconnection
    metrics->disconnectCount++;
    fb303::fbData->addStatValue(
        counterPrefix_ + ".ipc.disconnect_count", 1, fb303::SUM);
  } else if (
      oldState != IpcConnectionState::CONNECTED &&
      newState == IpcConnectionState::CONNECTED) {
    // Track successful connection
    metrics->connectCount++;
    fb303::fbData->addStatValue(
        counterPrefix_ + ".ipc.connect_count", 1, fb303::SUM);
  }

  if (newState == IpcConnectionState::CONNECTING) {
    // Track reconnection attempt
    metrics->reconnectCount++;
    fb303::fbData->addStatValue(
        counterPrefix_ + ".ipc.reconnect_count", 1, fb303::SUM);
  }

  // Log the state transition
  XLOG(INFO) << "IPC state transition for " << clientId_ << ": "
             << stateToString(oldState) << " -> " << stateToString(newState);

  // Report the state to fb303
  fb303::fbData->setCounter(
      counterPrefix_ + ".ipc.connection_state", static_cast<int>(newState));
}

IpcHealthMetrics IpcHealthMonitor::getMetrics() const {
  return *metrics_.rlock();
}

int64_t IpcHealthMonitor::getCurrentTimestampMs() const {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
             std::chrono::steady_clock::now().time_since_epoch())
      .count();
}

std::string IpcHealthMonitor::stateToString(IpcConnectionState state) const {
  switch (state) {
    case IpcConnectionState::CONNECTED:
      return "CONNECTED";
    case IpcConnectionState::DISCONNECTED:
      return "DISCONNECTED";
    case IpcConnectionState::CONNECTING:
      return "CONNECTING";
    case IpcConnectionState::CANCELLED:
      return "CANCELLED";
    case IpcConnectionState::UNKNOWN:
      return "UNKNOWN";
    default:
      return "INVALID";
  }
}

} // namespace facebook::fboss
