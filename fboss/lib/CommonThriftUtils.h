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

#include "fboss/lib/thrift_service_client/ConnectionOptions.h"

#include <fb303/ThreadCachedServiceData.h>
#include <folly/SocketAddress.h>
#include <folly/coro/AsyncScope.h>

#include <optional>
#include <random>
#include <string>

namespace facebook::fboss {

// helper macro to be used in ReconnectingThriftClient to output connection info
#define STREAM_XLOG(LEVEL) \
  XLOG(LEVEL) << "[STREAM " << connectionLogStr_ << "] "

class ExpBackoff {
 public:
  ExpBackoff(
      int32_t _initialBackoffReconnectTimeout,
      int32_t _maxBackoffReconnectTimeout,
      bool useJitter = true)
      : initialBackoffReconnectTimeout(
            std::chrono::milliseconds(_initialBackoffReconnectTimeout)),
        maxBackoffReconnectTimeout(
            std::chrono::milliseconds(_maxBackoffReconnectTimeout)),
        useJitter_(useJitter),
        currentBackoff_(addJitter(initialBackoffReconnectTimeout)) {}

  void reportSuccess() {
    currentBackoff_ = addJitter(initialBackoffReconnectTimeout);
  }

  void reportError() {
    currentBackoff_ = std::min(currentBackoff_ * 2, maxBackoffReconnectTimeout);
    currentBackoff_ = addJitter(currentBackoff_);
  }

  std::chrono::milliseconds getCurTimeout() {
    return currentBackoff_;
  }

 private:
  /*
   * When Agent Coldboots/warmboots, it will attempt to establish subscriptions
   * to all remote interface nodes. If several remote interface nodes are
   * unreachable, we will periodically attempt to reconnect to all those remote
   * nodes. This will cause periodic spikes and may cause drops. Avoid it by
   * spacing out reconnect requests to remote nodes.
   *
   * This is done by adding jitter to reconnect interval.
   */
  std::chrono::milliseconds addJitter(std::chrono::milliseconds t) {
    if (!useJitter_) {
      return t;
    }
    // Create a uniform distribution between 0 and value / 2
    std::uniform_int_distribution<int> distribution(0, t.count() * 0.5);
    int jitter = distribution(engine_);
    bool isEven = (jitter % 2 == 0);
    // evenly distribute jitter: add if even, subtract if odd
    return isEven ? std::chrono::milliseconds(t.count() + jitter)
                  : std::chrono::milliseconds(t.count() - jitter);
  }

 private:
  std::chrono::milliseconds initialBackoffReconnectTimeout; // in ms
  std::chrono::milliseconds maxBackoffReconnectTimeout; // in ms
  bool useJitter_{true};
  std::mt19937 engine_{
      std::random_device{}()}; // for adding jitter to exp backoff
  std::chrono::milliseconds currentBackoff_;
};

class ReconnectingThriftClient {
 public:
  enum class State : uint16_t {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    CANCELLED
  };

  static std::string connectionStateToString(State state) {
    switch (state) {
      case State::CONNECTING:
        return "CONNECTING";
      case State::CONNECTED:
        return "CONNECTED";
      case State::DISCONNECTED:
        return "DISCONNECTED";
      case State::CANCELLED:
        return "CANCELLED";
    }
    throw std::runtime_error(
        "Unhandled ReconnectingThriftClient::State::" +
        std::to_string(static_cast<int>(state)));
  }

  using StreamStateChangeCb = std::function<void(State, State)>;

  ReconnectingThriftClient(
      const std::string& clientId,
      folly::EventBase* streamEvb,
      folly::EventBase* connRetryEvb,
      const std::string& counterPrefix,
      const std::string& aggCounterPrefix,
      StreamStateChangeCb stateChangeCb,
      int initialBackoffReconnectMs,
      int maxBackoffReconnectMs = 1000);

  virtual ~ReconnectingThriftClient() {}

  State getState() const {
    return *state_.rlock();
  }

  const std::string& getCounterPrefix() const {
    return counterPrefix_;
  }

  const std::string& clientId() const {
    return clientId_;
  }

  void setConnectionOptions(
      utils::ConnectionOptions options,
      bool allowReset = false /* allow reset for use in tests*/);

  std::string getServer() const {
    if (auto connectionOptions = connectionOptions_.rlock();
        connectionOptions->has_value()) {
      return (*connectionOptions)->getDstAddr().getAddressStr();
    }
    return "";
  }
  void cancel();
  void timeoutExpired() noexcept;
  virtual void resetClient() = 0;
  bool isCancelled() const;
  bool isConnectedToServer() const;
  virtual void onCancellation() = 0;

 protected:
  virtual void setState(State state);
  void scheduleTimeout();
  std::string getConnectedCounterName() {
    return counterPrefix_ + ".connected";
  }
  virtual void connectToServer(const utils::ConnectionOptions& options) = 0;

  void setGracefulServiceLoopCompletion(const std::function<void()>& cb) {
    auto requested = gracefulServiceLoopCompletionCb_.wlock();
    *requested = cb;
  }
  bool isGracefulServiceLoopCompletionRequested() {
    auto requested = gracefulServiceLoopCompletionCb_.rlock();
    return requested->has_value();
  }
  void notifyGracefulServiceLoopCompletion() {
    auto requested = gracefulServiceLoopCompletionCb_.rlock();
    if (requested->has_value()) {
      (*requested).value()();
    }
  }

#if FOLLY_HAS_COROUTINES
  virtual folly::coro::Task<void> serviceLoopWrapper() = 0;
#endif

#if FOLLY_HAS_COROUTINES
  folly::coro::CancellableAsyncScope serviceLoopScope_;
#endif
  std::string clientId_;
  folly::EventBase* streamEvb_;
  folly::EventBase* connRetryEvb_;
  std::string counterPrefix_;
  folly::Synchronized<State> state_{State::DISCONNECTED};
  fb303::TimeseriesWrapper disconnectEvents_;
  fb303::TimeseriesWrapper aggDisconnectEvents_;
  StreamStateChangeCb stateChangeCb_;
  folly::Synchronized<std::optional<utils::ConnectionOptions>>
      connectionOptions_;
  ExpBackoff backoff_;
  std::unique_ptr<folly::AsyncTimeout> timer_;
  std::string connectionLogStr_;
  folly::Synchronized<std::optional<std::function<void()>>>
      gracefulServiceLoopCompletionCb_;
};
}; // namespace facebook::fboss
