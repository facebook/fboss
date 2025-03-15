// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/fsdb/common/Utils.h"
#include "fboss/fsdb/if/gen-cpp2/FsdbService.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h"
#include "fboss/lib/CommonThriftUtils.h"
#include "fboss/lib/thrift_service_client/ConnectionOptions.h"

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

#include <atomic>
#include <functional>
#include <optional>
#include <string>

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
DECLARE_int32(
    fsdb_reconnect_ms); // TODO: clean up after migrating to exponential backoff
DECLARE_int32(fsdb_initial_backoff_reconnect_ms);
DECLARE_int32(fsdb_max_backoff_reconnect_ms);

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
      int fsdbInitialBackoffReconnectMs =
          FLAGS_fsdb_initial_backoff_reconnect_ms,
      int fsdbMaxBackoffReconnectMs = FLAGS_fsdb_max_backoff_reconnect_ms);
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
  void createClient(const utils::ConnectionOptions& options);
  void resetClient() override;
  void connectToServer(const utils::ConnectionOptions& options) override;
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
    updateDisconnectReasonCounter(reason);
  }
  std::unique_ptr<apache::thrift::Client<FsdbService>> client_;

  apache::thrift::RpcOptions& getRpcOptions() {
    return rpcOptions_;
  }
  folly::Synchronized<FsdbErrorCode> disconnectReason_{FsdbErrorCode::NONE};

 private:
  void updateDisconnectReasonCounter(fsdb::FsdbErrorCode reason) {
    switch (reason) {
      case fsdb::FsdbErrorCode::CLIENT_CHUNK_TIMEOUT:
        disconnectReasonChunkTimeout_.add(1);
        break;
      case fsdb::FsdbErrorCode::SUBSCRIPTION_DATA_CALLBACK_ERROR:
        disconnectReasonDataCbError_.add(1);
        break;
      case fsdb::FsdbErrorCode::CLIENT_TRANSPORT_EXCEPTION:
        disconnectReasonTransportError_.add(1);
        break;
      case fsdb::FsdbErrorCode::ID_ALREADY_EXISTS:
        disconnectReasonIdExists_.add(1);
        break;
      case fsdb::FsdbErrorCode::EMPTY_PUBLISHER_ID:
      case fsdb::FsdbErrorCode::UNKNOWN_PUBLISHER:
      case fsdb::FsdbErrorCode::EMPTY_SUBSCRIBER_ID:
      case fsdb::FsdbErrorCode::INVALID_PATH:
        disconnectReasonBadArgs_.add(1);
        break;
      default:
        break;
    };
  }

  folly::EventBase* streamEvb_;
  std::atomic<bool> serviceLoopRunning_{false};
  const bool isStats_;
  apache::thrift::RpcOptions rpcOptions_;
  // counters for various disconnect reasons
  fb303::TimeseriesWrapper disconnectReasonChunkTimeout_{
      getCounterPrefix() + ".disconnectReason.chunkTimeout",
      fb303::SUM,
      fb303::RATE};
  fb303::TimeseriesWrapper disconnectReasonDataCbError_{
      getCounterPrefix() + ".disconnectReason.dataCbError",
      fb303::SUM,
      fb303::RATE};
  fb303::TimeseriesWrapper disconnectReasonTransportError_{
      getCounterPrefix() + ".disconnectReason.transportError",
      fb303::SUM,
      fb303::RATE};
  fb303::TimeseriesWrapper disconnectReasonIdExists_{
      getCounterPrefix() + ".disconnectReason.dupId",
      fb303::SUM,
      fb303::RATE};
  fb303::TimeseriesWrapper disconnectReasonBadArgs_{
      getCounterPrefix() + ".disconnectReason.badArgs",
      fb303::SUM,
      fb303::RATE};
};

} // namespace facebook::fboss::fsdb
