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

#include <fb303/ThreadCachedServiceData.h>
#include <folly/CancellationToken.h>
#include <folly/Synchronized.h>
#include <folly/io/async/EventBase.h>
#include <atomic>
#include <string>
#include <unordered_map>
namespace facebook::fboss {

enum class IpcConnectionState {
  UNKNOWN = 0,
  DISCONNECTED = 1,
  CONNECTING = 2,
  CONNECTED = 3,
  CANCELLED = 4,
};

struct IpcHealthMetrics {
  int64_t lastSuccessfulIpcTimestamp{0};
  int64_t lastRoundTripTime{0};
  int64_t lastStateTransitionTimestamp{0};
  IpcConnectionState connectionState{IpcConnectionState::UNKNOWN};
  int64_t connectCount{0};
  int64_t disconnectCount{0};
  int64_t reconnectCount{0};
  int64_t eventsSent{0};
  int64_t eventsDropped{0};
  int64_t eventsReceived{0};
  int64_t queueDepth{0};
};

/*
 * IpcHealthMonitor monitors the health of IPC communication between
 * hardware agent and software agent in the split architecture.
 *
 * This class implements comprehensive monitoring for IPC health metrics
 */
class IpcHealthMonitor {
 public:
  IpcHealthMonitor(
      const std::string& clientId,
      const std::string& counterPrefix,
      folly::EventBase* evb);
  ~IpcHealthMonitor();

  // Start monitoring IPC health
  void start();

  // Stop monitoring IPC health
  void stop();

  // Track failed IPC operation
  void trackFailedOperation();

  // Track event operations
  void trackEventSent();
  void trackEventDropped();
  void trackEventReceived();

  // Track queue depth
  void trackQueueDepth(int64_t depth);

  // Track connection state transitions
  void trackStateTransition(
      IpcConnectionState oldState,
      IpcConnectionState newState);

  // Get current metrics
  IpcHealthMetrics getMetrics() const;

 private:
  // Helper methods
  int64_t getCurrentTimestampMs() const;
  double getDropRate() const;
  double getSuccessRate() const;
  std::string stateToString(IpcConnectionState state) const;

  std::string clientId_;
  std::string counterPrefix_;
  folly::EventBase* evb_;

  std::atomic<bool> running_{false};

  // Metrics storage with atomic counters for high-frequency operations
  folly::Synchronized<IpcHealthMetrics> metrics_;

  // fb303 TimeseriesWrapper for efficient metric tracking
  fb303::TimeseriesWrapper eventsSent_;
  fb303::TimeseriesWrapper eventsDropped_;
  fb303::TimeseriesWrapper eventsReceived_;
  fb303::TimeseriesWrapper queueDepth_;
};

} // namespace facebook::fboss
