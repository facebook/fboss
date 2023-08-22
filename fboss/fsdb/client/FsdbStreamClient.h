// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <fb303/ThreadCachedServiceData.h>
#include <folly/SocketAddress.h>
#include <folly/experimental/coro/AsyncScope.h>
#include <folly/io/async/AsyncSocketTransport.h>
#include <folly/io/async/ScopedEventBaseThread.h>
#include <gtest/gtest_prod.h>
#include <thrift/lib/cpp2/async/ClientBufferedStream.h>
#include <thrift/lib/cpp2/async/RpcOptions.h>
#include <thrift/lib/cpp2/async/Sink.h>
#include <optional>
#include <string>
#ifndef IS_OSS
#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h"
#endif
#include "fboss/lib/CommonThriftUtils.h"

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

DECLARE_int32(fsdb_state_chunk_timeout);
DECLARE_int32(fsdb_stat_chunk_timeout);

using State = facebook::fboss::ReconnectingThriftClient::State;

namespace facebook::fboss::fsdb {
class FsdbService;

class FsdbStreamClient : public ReconnectingThriftClient {
 public:
  using FsdbStreamStateChangeCb = std::function<void(State, State)>;

  FsdbStreamClient(
      const std::string& clientId,
      folly::EventBase* streamEvb,
      folly::EventBase* connRetryEvb,
      const std::string& counterPrefix,
      bool isStats = false,
      StreamStateChangeCb stateChangeCb = [](State /*old*/,
                                             State /*newState*/) {});
  virtual ~FsdbStreamClient();

  bool serviceLoopRunning() const {
    return serviceLoopRunning_.load();
  }
  bool isStats() const {
    return isStats_;
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
  void resetClient() override;
  void connectToServer(const ServerOptions& options) override;
  void timeoutExpired() noexcept;

#if FOLLY_HAS_COROUTINES && !defined(IS_OSS)
  folly::coro::Task<void> serviceLoopWrapper() override;
  virtual folly::coro::Task<StreamT> setupStream() = 0;
  virtual folly::coro::Task<void> serveStream(StreamT&& stream) = 0;
#endif

 protected:
#ifndef IS_OSS
  std::unique_ptr<apache::thrift::Client<FsdbService>> client_;
#endif

  apache::thrift::RpcOptions& getRpcOptions() {
    return rpcOptions_;
  }

 private:
  folly::EventBase* streamEvb_;
  std::atomic<bool> serviceLoopRunning_{false};
  const bool isStats_;
  apache::thrift::RpcOptions rpcOptions_;
};

} // namespace facebook::fboss::fsdb
