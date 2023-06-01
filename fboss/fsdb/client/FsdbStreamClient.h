// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <fb303/ThreadCachedServiceData.h>
#include <folly/SocketAddress.h>
#include <folly/experimental/coro/AsyncScope.h>
#include <folly/io/async/AsyncSocketTransport.h>
#include <folly/io/async/ScopedEventBaseThread.h>
#include <gtest/gtest_prod.h>
#include <thrift/lib/cpp2/async/ClientBufferedStream.h>
#include <thrift/lib/cpp2/async/Sink.h>
#include <optional>
#include <string>
#ifndef IS_OSS
#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h"
#endif

#include <atomic>
#include <functional>

namespace folly {
class CancellationToken;
class AsyncTimeout;
} // namespace folly

namespace apache::thrift {
template <class>
class Client;
} // namespace apache::thrift

namespace facebook::fboss::fsdb {
class FsdbService;

class FsdbStreamClient {
 public:
  enum class State : uint16_t {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    CANCELLED
  };
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

  using FsdbStreamStateChangeCb = std::function<void(State, State)>;
  FsdbStreamClient(
      const std::string& clientId,
      folly::EventBase* streamEvb,
      folly::EventBase* connRetryEvb,
      const std::string& counterPrefix,
      FsdbStreamStateChangeCb stateChangeCb = [](State /*old*/,
                                                 State /*newState*/) {});
  virtual ~FsdbStreamClient();

  void setServerOptions(
      ServerOptions&& options,
      /* allow reset for use in tests*/
      bool allowReset = false);

  void cancel();

  bool isConnectedToServer() const;
  bool isCancelled() const;
  const std::string& clientId() const {
    return clientId_;
  }
  State getState() const {
    return *state_.rlock();
  }
  bool serviceLoopRunning() const {
    return serviceLoopRunning_.load();
  }
  const std::string& getCounterPrefix() const {
    return counterPrefix_;
  }

#ifndef IS_OSS
  template <typename PubUnit>
  using PubStreamT = apache::thrift::ClientSink<PubUnit, OperPubFinalResponse>;
  template <typename SubUnit>
  using SubStreamT = apache::thrift::ClientBufferedStream<SubUnit>;
  using StatePubStreamT = PubStreamT<OperState>;
  using DeltaPubStreamT = PubStreamT<OperDelta>;
  using StateSubStreamT = SubStreamT<OperState>;
  using DeltaSubStreamT = SubStreamT<OperDelta>;
  using StateExtSubStreamT = SubStreamT<OperSubPathUnit>;
  using DeltaExtSubStreamT = SubStreamT<OperSubDeltaUnit>;

  using StreamT = std::variant<
      StatePubStreamT,
      DeltaPubStreamT,
      StateSubStreamT,
      DeltaSubStreamT,
      StateExtSubStreamT,
      DeltaExtSubStreamT>;
#endif

 private:
  void createClient(const ServerOptions& options);
  void resetClient();
  void connectToServer(const ServerOptions& options);
  void timeoutExpired() noexcept;

#if FOLLY_HAS_COROUTINES && !defined(IS_OSS)
  folly::coro::Task<void> serviceLoopWrapper();
  virtual folly::coro::Task<StreamT> setupStream() = 0;
  virtual folly::coro::Task<void> serveStream(StreamT&& stream) = 0;
#endif

 protected:
  void setState(State state);
#ifndef IS_OSS
  std::unique_ptr<apache::thrift::Client<FsdbService>> client_;
#endif

 private:
  std::string getConnectedCounterName() {
    return counterPrefix_ + ".connected";
  }
  std::string clientId_;
  folly::EventBase* streamEvb_;
  folly::EventBase* connRetryEvb_;
  folly::Synchronized<State> state_{State::DISCONNECTED};
  std::string counterPrefix_;
  folly::Synchronized<std::optional<ServerOptions>> serverOptions_;
  FsdbStreamStateChangeCb stateChangeCb_;
  std::atomic<bool> serviceLoopRunning_{false};
  std::unique_ptr<folly::AsyncTimeout> timer_;
#if FOLLY_HAS_COROUTINES
  folly::coro::CancellableAsyncScope serviceLoopScope_;
#endif
  fb303::TimeseriesWrapper disconnectEvents_;
};

} // namespace facebook::fboss::fsdb
