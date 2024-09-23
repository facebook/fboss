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
#include <folly/SocketAddress.h>
#include <folly/coro/AsyncScope.h>

#include <optional>
#include <string>

namespace facebook::fboss {

// helper macro to be used in ReconnectingThriftClient to output connection info
#define STREAM_XLOG(LEVEL) \
  XLOG(LEVEL) << "[STREAM " << connectionLogStr_ << "] "

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

  enum class Priority : uint8_t { NORMAL, CRITICAL };

  struct ServerOptions {
    ServerOptions(
        const std::string& dstIp,
        uint16_t dstPort,
        const std::optional<std::string>& deviceName = std::nullopt)
        : dstAddr(folly::SocketAddress(dstIp, dstPort)),
          deviceName(deviceName.value_or(dstAddr.getAddressStr())) {}

    ServerOptions(
        const std::string& dstIp,
        uint16_t dstPort,
        const std::string& srcIp,
        const std::optional<std::string>& deviceName = std::nullopt)
        : dstAddr(folly::SocketAddress(dstIp, dstPort)),
          srcAddr(folly::SocketAddress(srcIp, 0)),
          deviceName(deviceName.value_or(dstAddr.getAddressStr())) {}

    ServerOptions(
        const std::string& dstIp,
        uint16_t dstPort,
        const std::string& srcIp,
        Priority priority,
        const std::optional<std::string>& deviceName = std::nullopt)
        : dstAddr(folly::SocketAddress(dstIp, dstPort)),
          srcAddr(folly::SocketAddress(srcIp, 0)),
          priority{priority},
          deviceName(deviceName.value_or(dstAddr.getAddressStr())) {}

    folly::SocketAddress dstAddr;
    std::string fsdbPort;
    std::optional<folly::SocketAddress> srcAddr;
    std::optional<Priority> priority;
    std::string deviceName;
  };

  ReconnectingThriftClient(
      const std::string& clientId,
      folly::EventBase* streamEvb,
      folly::EventBase* connRetryEvb,
      const std::string& counterPrefix,
      const std::string& aggCounterPrefix,
      StreamStateChangeCb stateChangeCb,
      uint32_t reconnectTimeout);

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

  void setServerOptions(
      ServerOptions&& options,
      bool allowReset = false /* allow reset for use in tests*/);

  std::string getServer() const {
    if (auto serverOptions = serverOptions_.rlock();
        serverOptions->has_value()) {
      return (*serverOptions)->dstAddr.getAddressStr();
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
  virtual void connectToServer(const ServerOptions& options) = 0;

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
  folly::Synchronized<std::optional<ServerOptions>> serverOptions_;
  std::unique_ptr<folly::AsyncTimeout> timer_;
  uint32_t reconnectTimeout_;
  std::string connectionLogStr_;
  folly::Synchronized<std::optional<std::function<void()>>>
      gracefulServiceLoopCompletionCb_;
};
}; // namespace facebook::fboss
