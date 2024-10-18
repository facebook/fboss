// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/fsdb/client/FsdbStreamClient.h"
#include "common/time/Time.h"
#include "fboss/lib/thrift_service_client/ThriftServiceClient.h"

#include <folly/logging/xlog.h>
#include <chrono>
#include <cstdint>
#include <optional>
#include <random>

DEFINE_int32(
    fsdb_reconnect_ms,
    1000,
    "reconnect to fsdb timer in milliseconds");

DEFINE_int32(
    fsdb_state_chunk_timeout,
    0, // disabled by default for now
    "Chunk timeout in seconds for FSDB State streams");
DEFINE_int32(
    fsdb_stat_chunk_timeout,
    0, // disabled by default for now
    "Chunk timeout in seconds for FSDB Stat streams");

namespace {
/*
 * When Agent Coldboots/warmboots, it will attempt to establish subscriptions
 * to all remote interface nodes. If several remote interface nodes are
 * unreacuable, we will periodically attempt to reconnect to all those remote
 * nodes. This will cause periodic spikes and may cause drops. Avoid it by
 * spacing out reconnect requests to remote nodes.
 *
 * This is done by adding jitter to reconnect interval.
 */

int getReconnectIntervalInMs(int value) {
  static std::mt19937 engine{std::random_device{}()};
  // Create a uniform distribution between 0 and value / 2
  std::uniform_int_distribution<int> distribution(0, value * 0.5);
  int jitter = distribution(engine);
  bool isEven = (jitter % 2 == 0);
  // evenly distribute jitter: add if even, subtract if odd
  return isEven ? value + jitter : value - jitter;
}

} // namespace

namespace facebook::fboss::fsdb {
FsdbStreamClient::FsdbStreamClient(
    const std::string& clientId,
    folly::EventBase* streamEvb,
    folly::EventBase* connRetryEvb,
    const std::string& counterPrefix,
    bool isStats,
    StreamStateChangeCb stateChangeCb,
    int fsdbReconnectMs)
    : ReconnectingThriftClient(
          clientId,
          streamEvb,
          connRetryEvb,
          counterPrefix,
          "fsdb_streams",
          stateChangeCb,
          getReconnectIntervalInMs(fsdbReconnectMs)),
      streamEvb_(streamEvb),
      isStats_(isStats) {
  if (isStats && FLAGS_fsdb_stat_chunk_timeout) {
    rpcOptions_.setChunkTimeout(
        std::chrono::seconds(FLAGS_fsdb_stat_chunk_timeout));
  } else if (!isStats && FLAGS_fsdb_state_chunk_timeout) {
    rpcOptions_.setChunkTimeout(
        std::chrono::seconds(FLAGS_fsdb_state_chunk_timeout));
  }
  scheduleTimeout();
}

FsdbStreamClient::~FsdbStreamClient() {
  STREAM_XLOG(DBG2) << "Destroying FsdbStreamClient";
  CHECK(isCancelled());
}

void FsdbStreamClient::connectToServer(const ServerOptions& options) {
  CHECK(getState() == State::DISCONNECTED);
  streamEvb_->runImmediatelyOrRunInEventBaseThreadAndWait([this, &options]() {
    try {
      createClient(options);
      setState(State::CONNECTING);
    } catch (const std::exception& ex) {
      STREAM_XLOG(ERR) << "Connect to server failed with ex: " << ex.what();
      setState(State::DISCONNECTED);
    }
  });
}

#if FOLLY_HAS_COROUTINES
folly::coro::Task<void> FsdbStreamClient::serviceLoopWrapper() {
  STREAM_XLOG(INFO) << "Service loop started";
  serviceLoopRunning_.store(true);
  SCOPE_EXIT {
    STREAM_XLOG(INFO) << "Service loop done";
    serviceLoopRunning_.store(false);
  };
  try {
    auto stream = co_await setupStream();
    setState(State::CONNECTED);
    co_await serveStream(std::move(stream));
  } catch (const folly::OperationCancelled&) {
    STREAM_XLOG(DBG2) << "Service loop cancelled";
  } catch (const apache::thrift::SinkThrew& ex) {
    if (ex.getMessage().find("FsdbClientGRDisconnectException") !=
        std::string::npos) {
      STREAM_XLOG(INFO)
          << "Service loop exited with FsdbClientGRDisconnectException";
    } else {
      STREAM_XLOG(ERR)
          << "Service loop exited with unknown exception, mark DISCONNECTED: "
          << ex.getMessage();
      setStateDisconnectedWithReason(FsdbErrorCode::DISCONNECTED);
    }
  } catch (const FsdbException& ef) {
    STREAM_XLOG(ERR) << "FsdbException: "
                     << apache::thrift::util::enumNameSafe(ef.get_errorCode())
                     << ": " << ef.get_message();
    setStateDisconnectedWithReason(ef.get_errorCode());
  } catch (const apache::thrift::transport::TTransportException& et) {
    FsdbErrorCode disconnectReason = FsdbErrorCode::CLIENT_TRANSPORT_EXCEPTION;
    if (et.getType() ==
        apache::thrift::transport::TTransportException::
            TTransportExceptionType::TIMED_OUT) {
      disconnectReason = FsdbErrorCode::CLIENT_CHUNK_TIMEOUT;
    }
    setStateDisconnectedWithReason(disconnectReason);
  } catch (const std::exception& ex) {
    STREAM_XLOG(ERR) << "Unknown error: " << folly::exceptionStr(ex);
    setStateDisconnectedWithReason(FsdbErrorCode::DISCONNECTED);
  }
  notifyGracefulServiceLoopCompletion();
  co_return;
}
#endif

void FsdbStreamClient::resetClient() {
  CHECK(streamEvb_->getEventBase()->isInEventBaseThread());
  client_.reset();
}

utils::ConnectionOptions::TrafficClass getTosForClientPriority(
    const std::optional<FsdbStreamClient::Priority> priority) {
  if (priority == FsdbStreamClient::Priority::CRITICAL) {
    return utils::ConnectionOptions::TrafficClass::NC;
  }
  return utils::ConnectionOptions::TrafficClass::DEFAULT;
}

bool shouldUseEncryptedClient(const FsdbStreamClient::ServerOptions& options) {
  // use encrypted connection for all clients except CRITICAL ones.
  return (options.priority != FsdbStreamClient::Priority::CRITICAL);
}

void FsdbStreamClient::createClient(const ServerOptions& options) {
  CHECK(streamEvb_->getEventBase()->isInEventBaseThread());
  resetClient();

  auto tos = getTosForClientPriority(options.priority);
  bool encryptedClient = shouldUseEncryptedClient(options);
  client_ = createFsdbClient(
      utils::ConnectionOptions(options.dstAddr)
          .setSrcAddr(options.srcAddr)
          .setTrafficClass(tos)
          .setPreferEncrypted(encryptedClient),
      streamEvb_);
}

} // namespace facebook::fboss::fsdb
