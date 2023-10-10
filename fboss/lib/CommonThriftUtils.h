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
#include <folly/experimental/coro/AsyncScope.h>

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
  using StreamStateChangeCb = std::function<void(State, State)>;

  enum class Priority : uint8_t { NORMAL, CRITICAL };

  struct ServerOptions {
    ServerOptions(const std::string& dstIp, uint16_t dstPort)
        : dstAddr(folly::SocketAddress(dstIp, dstPort)) {}

    ServerOptions(const std::string& dstIp, uint16_t dstPort, std::string srcIp)
        : dstAddr(folly::SocketAddress(dstIp, dstPort)),
          srcAddr(folly::SocketAddress(srcIp, 0)) {}

    ServerOptions(
        const std::string& dstIp,
        uint16_t dstPort,
        std::string srcIp,
        Priority priority)
        : dstAddr(folly::SocketAddress(dstIp, dstPort)),
          srcAddr(folly::SocketAddress(srcIp, 0)),
          priority{priority} {}

    folly::SocketAddress dstAddr;
    std::string fsdbPort;
    std::optional<folly::SocketAddress> srcAddr;
    std::optional<Priority> priority;
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
  void cancel();
  void timeoutExpired() noexcept;
  virtual void resetClient() = 0;
  bool isCancelled() const;
  bool isConnectedToServer() const;

 protected:
  void setState(State state);
  void scheduleTimeout();
  std::string getConnectedCounterName() {
    return counterPrefix_ + ".connected";
  }
  virtual void connectToServer(const ServerOptions& options) = 0;

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
};
}; // namespace facebook::fboss
