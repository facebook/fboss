// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/fsdb/client/FsdbStreamClient.h"

#include <folly/CancellationToken.h>
#include <folly/experimental/coro/BlockingWait.h>
#include <folly/logging/xlog.h>

#ifndef IS_OSS
#include "fboss/fsdb/client/facebook/Client.h"
#endif

DEFINE_int32(
    fsdb_reconnect_ms,
    1000,
    "re-connect to fsdbm timer in milliseconds");

namespace facebook::fboss::fsdb {
FsdbStreamClient::FsdbStreamClient(
    const std::string& clientId,
    folly::EventBase* streamEvb,
    folly::EventBase* connRetryEvb,
    FsdbStreamStateChangeCb stateChangeCb)
    : folly::AsyncTimeout(connRetryEvb),
      clientId_(clientId),
      streamEvb_(streamEvb),
      connRetryEvb_(connRetryEvb),
      clientEvbThread_(
          std::make_unique<folly::ScopedEventBaseThread>(clientId)),
      stateChangeCb_(stateChangeCb) {
  if (!streamEvb_ || !connRetryEvb) {
    throw std::runtime_error(
        "Must pass valid stream, connRetry evbs to ctor, but passed null");
  }

  cancelSource_ = std::make_unique<folly::CancellationSource>();
  connRetryEvb->runInEventBaseThread(
      [this] { scheduleTimeout(FLAGS_fsdb_reconnect_ms); });
}

FsdbStreamClient::~FsdbStreamClient() {
  XLOG(DBG2) << "Destroying FsdbStreamClient";
  cancel();
}

void FsdbStreamClient::setState(State state) {
  auto oldState = state_.load();
  if (oldState == state) {
    return;
  }
  state_.store(state);
  stateChangeCb_(oldState, state);
}

void FsdbStreamClient::setServerToConnect(
    const std::string& ip,
    uint16_t port,
    bool allowReset) {
  if (!allowReset && serverAddress_) {
    throw std::runtime_error("Cannot reset server address");
  }
  serverAddress_ = folly::SocketAddress(ip, port);
}

void FsdbStreamClient::timeoutExpired() noexcept {
  if (getState() == State::DISCONNECTED && serverAddress_) {
    connectToServer(
        serverAddress_->getIPAddress().str(), serverAddress_->getPort());
  }
  scheduleTimeout(FLAGS_fsdb_reconnect_ms);
}

bool FsdbStreamClient::isConnectedToServer() const {
  return (getState() == State::CONNECTED);
}

void FsdbStreamClient::connectToServer(const std::string& ip, uint16_t port) {
  CHECK(getState() == State::DISCONNECTED);

  streamEvb_->runInEventBaseThreadAndWait([this, ip, port]() {
    try {
      createClient(ip, port);
      setState(State::CONNECTED);
    } catch (const std::exception& ex) {
      XLOG(ERR) << "Connect to server failed with ex:" << ex.what();
      setState(State::DISCONNECTED);
    }
  });

  if (getState() == State::CONNECTED) {
#if FOLLY_HAS_COROUTINES
    auto startServiceLoop = ([this]() -> folly::coro::Task<void> {
      try {
        co_await serviceLoop();
      } catch (const std::exception& ex) {
        XLOG(ERR) << "Service loop broken:" << ex.what();
        setState(State::DISCONNECTED);
      }
      co_return;
    });
    startServiceLoop().scheduleOn(streamEvb_).start();
#endif
  }
}

// TODO derived classes to override this

void FsdbStreamClient::cancel() {
  XLOG(DBG2) << "Cancel FsdbStreamClient: " << clientId();
  cancelSource_.withWLock([this](auto& cancelSource) {
    if (cancelSource) {
      XLOG(DBG2) << "Request FsdbStreamClient Cancellation for: " << clientId();
      cancelSource->requestCancellation();
    }
  });

  // already disconnected;
  if (getState() == State::CANCELLED) {
    XLOG(WARNING) << clientId_ << " already cancelled";
    return;
  }
  serverAddress_.reset();
  connRetryEvb_->runInEventBaseThreadAndWait([this] { cancelTimeout(); });
  setState(State::CANCELLED);
  resetClient();
  // terminate event base getting ready for clean-up
  clientEvbThread_->getEventBase()->terminateLoopSoon();
  XLOG(DBG2) << " Cancelled: " << clientId_;
}

bool FsdbStreamClient::isCancelled() const {
  return (getState() == State::CANCELLED);
}

} // namespace facebook::fboss::fsdb
