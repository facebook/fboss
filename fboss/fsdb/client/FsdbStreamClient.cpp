// Copyright 2004-present Facebook. All Rights Reserved.

#include <chrono>

#include "fboss/fsdb/client/FsdbStreamClient.h"

#include <folly/logging/xlog.h>
#include "common/time/Time.h"
#ifndef IS_OSS
#include "fboss/fsdb/client/Client.h"
#endif

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

namespace facebook::fboss::fsdb {
FsdbStreamClient::FsdbStreamClient(
    const std::string& clientId,
    folly::EventBase* streamEvb,
    folly::EventBase* connRetryEvb,
    const std::string& counterPrefix,
    bool isStats,
    StreamStateChangeCb stateChangeCb)
    : ReconnectingThriftClient(
          clientId,
          streamEvb,
          connRetryEvb,
          counterPrefix,
          "fsdb_streams",
          stateChangeCb,
          FLAGS_fsdb_reconnect_ms),
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
  } catch (const std::exception& ex) {
    STREAM_XLOG(ERR) << "Unknown error: " << folly::exceptionStr(ex);
    setStateDisconnectedWithReason(FsdbErrorCode::DISCONNECTED);
  }
  notifyGracefulServiceLoopCompletion();
  co_return;
}
#endif

} // namespace facebook::fboss::fsdb
