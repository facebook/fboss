// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <fb303/ThreadCachedServiceData.h>
#include <folly/SocketAddress.h>
#include <folly/coro/AsyncScope.h>
#include <folly/io/async/AsyncSocketTransport.h>
#include <folly/io/async/ScopedEventBaseThread.h>
#include <folly/synchronization/Baton.h>
#include <gtest/gtest_prod.h>
#include <thrift/lib/cpp2/async/ClientBufferedStream.h>
#include <thrift/lib/cpp2/async/RpcOptions.h>
#include <thrift/lib/cpp2/async/Sink.h>
#include <optional>
#include <string>
#include "fboss/fsdb/common/Utils.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h"
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
DECLARE_int32(fsdb_reconnect_ms);

using State = facebook::fboss::ReconnectingThriftClient::State;

namespace facebook::fboss::fsdb {
class FsdbService;

class FsdbStreamClient : public ReconnectingThriftClient {
 public:
  using FsdbStreamStateChangeCb = std::function<void(State, State)>;

  class FsdbClientGRDisconnectException : public FsdbException {
   public:
    explicit FsdbClientGRDisconnectException(std::string msg) {
      this->message_ref() = std::move(msg);
      this->errorCode_ref() = FsdbErrorCode::PUBLISHER_GR_DISCONNECT;
    }
  };

  FsdbStreamClient(
      const std::string& clientId,
      folly::EventBase* streamEvb,
      folly::EventBase* connRetryEvb,
      const std::string& counterPrefix,
      bool isStats = false,
      StreamStateChangeCb stateChangeCb = [](State /*old*/,
                                             State /*newState*/) {},
      int fsdbReconnectMs = FLAGS_fsdb_reconnect_ms);
  virtual ~FsdbStreamClient();

  bool serviceLoopRunning() const {
    return serviceLoopRunning_.load();
  }
  bool isStats() const {
    return isStats_;
  }
  fsdb::FsdbErrorCode getDisconnectReason() const {
    return *disconnectReason_.rlock();
  }
  void onCancellation() override {}

  template <typename PubUnit>
  using PubStreamT = apache::thrift::ClientSink<PubUnit, OperPubFinalResponse>;
  template <typename SubUnit>
  using SubStreamT = apache::thrift::ClientBufferedStream<SubUnit>;
  using StatePubStreamT = PubStreamT<OperState>;
  using DeltaPubStreamT = PubStreamT<OperDelta>;
  using PatchPubStreamT = PubStreamT<PublisherMessage>;
  using StateSubStreamT = SubStreamT<OperState>;
  using DeltaSubStreamT = SubStreamT<OperDelta>;
  using PatchSubStreamT = SubStreamT<SubscriberMessage>;
  using StateExtSubStreamT = SubStreamT<OperSubPathUnit>;
  using DeltaExtSubStreamT = SubStreamT<OperSubDeltaUnit>;

  using StreamT = std::variant<
      StatePubStreamT,
      DeltaPubStreamT,
      PatchPubStreamT,
      StateSubStreamT,
      DeltaSubStreamT,
      PatchSubStreamT,
      StateExtSubStreamT,
      DeltaExtSubStreamT>;

 private:
  void createClient(const ServerOptions& options);
  void resetClient() override;
  void connectToServer(const ServerOptions& options) override;
  void timeoutExpired() noexcept;

#if FOLLY_HAS_COROUTINES
  folly::coro::Task<void> serviceLoopWrapper() override;
  virtual folly::coro::Task<StreamT> setupStream() = 0;
  virtual folly::coro::Task<void> serveStream(StreamT&& stream) = 0;
#endif

 protected:
  folly::EventBase* getStreamEventBase() const {
    return streamEvb_;
  }
  void setDisconnectReason(FsdbErrorCode reason) {
    auto disconnectReason = disconnectReason_.wlock();
    *disconnectReason = reason;
  }
  virtual void setState(State state) override {
    if (state == State::CONNECTED) {
      setDisconnectReason(FsdbErrorCode::NONE);
    }
    ReconnectingThriftClient::setState(state);
  }
  void setStateDisconnectedWithReason(fsdb::FsdbErrorCode reason) {
    setDisconnectReason(reason);
    setState(State::DISCONNECTED);
  }
  std::unique_ptr<apache::thrift::Client<FsdbService>> client_;

  apache::thrift::RpcOptions& getRpcOptions() {
    return rpcOptions_;
  }
  folly::Synchronized<FsdbErrorCode> disconnectReason_{FsdbErrorCode::NONE};

 private:
  folly::EventBase* streamEvb_;
  std::atomic<bool> serviceLoopRunning_{false};
  const bool isStats_;
  apache::thrift::RpcOptions rpcOptions_;
};

} // namespace facebook::fboss::fsdb
