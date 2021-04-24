// Copyright 2004-present Facebook. All Rights Reserved.
#include "fboss/agent/thrift_packet_stream/PacketStreamClient.h"
#include <folly/io/async/AsyncSocket.h>
#include <folly/logging/xlog.h>
#include <thrift/lib/cpp2/async/RocketClientChannel.h>

namespace facebook {
namespace fboss {

PacketStreamClient::PacketStreamClient(
    const std::string& clientId,
    folly::EventBase* evb)
    : clientId_(clientId),
      evb_(evb),
      clientEvbThread_(
          std::make_unique<folly::ScopedEventBaseThread>(clientId)) {
  if (!evb_) {
    throw std::runtime_error("Must pass valid evb to ctor, but passed null");
  }
}

PacketStreamClient::~PacketStreamClient() {
  try {
    XLOG(INFO) << "Destroying PacketStreamClient";
#if FOLLY_HAS_COROUTINES
    if (cancelSource_) {
      cancelSource_->requestCancellation();
    }
    if (isConnectedToServer()) {
      folly::coro::blockingWait(client_->co_disconnect(clientId_));
    }
#endif
  } catch (const std::exception& ex) {
    XLOG(WARNING) << clientId_ << " disconnect failed:" << ex.what();
  }
  if (evb_) {
    evb_->terminateLoopSoon();
  }
  clientEvbThread_->getEventBase()->runImmediatelyOrRunInEventBaseThreadAndWait(
      [this]() { client_.reset(); });
}

void PacketStreamClient::createClient(const std::string& ip, uint16_t port) {
  clientEvbThread_->getEventBase()->runImmediatelyOrRunInEventBaseThreadAndWait(
      [this, ip, port]() {
        auto addr = folly::SocketAddress(ip, port);
        client_ = std::make_unique<facebook::fboss::PacketStreamAsyncClient>(
            apache::thrift::RocketClientChannel::newChannel(
                folly::AsyncSocket::UniquePtr(new folly::AsyncSocket(
                    clientEvbThread_->getEventBase(), addr))));
      });
}

void PacketStreamClient::connectToServer(const std::string& ip, uint16_t port) {
#if FOLLY_HAS_COROUTINES
  if (State::INIT != state_.load()) {
    XLOG(INFO) << "Client is already in process of connecting to server";
    return;
  }
  if (!evb_) {
    throw std::runtime_error("Client resumed after cancel called");
  }
  state_.store(State::CONNECTING);
  cancelSource_ = std::make_unique<folly::CancellationSource>();
  evb_->runInEventBaseThread([this, ip, port] {
    try {
      createClient(ip, port);
      if (cancelSource_->isCancellationRequested()) {
        state_.store(State::INIT);
        return;
      }
      folly::coro::blockingWait(connect());
    } catch (const std::exception& ex) {
      XLOG(ERR) << "Connect to server failed with ex:" << ex.what();
      state_.store(State::INIT);
    }
  });
#else
  throw std::runtime_error("Coroutine support is needed for PacketStream");
#endif
}

#if FOLLY_HAS_COROUTINES
folly::coro::Task<void> PacketStreamClient::connect() {
  auto result = co_await client_->co_connect(clientId_);
  if (cancelSource_->isCancellationRequested()) {
    state_.store(State::INIT);
    XLOG(ERR) << "Cancellation Requested;";
    co_return;
  }
  state_.store(State::CONNECTED);
  XLOG(INFO) << clientId_ << " connected successfully";
  co_await folly::coro::co_withCancellation(
      cancelSource_->getToken(),
      folly::coro::co_invoke(
          [gen = std::move(result).toAsyncGenerator(),
           this]() mutable -> folly::coro::Task<void> {
            try {
              while (auto packet = co_await gen.next()) {
                recvPacket(std::move(*packet));
              }
            } catch (const std::exception& ex) {
              XLOG(ERR) << clientId_
                        << " Server error: " << folly::exceptionStr(ex);
              state_.store(State::INIT);
            }
            co_return;
          }));
  XLOG(INFO) << "Client Cancellation Completed";
}
#endif

void PacketStreamClient::cancel() {
  XLOG(INFO) << "Cancel PacketStreamClient";

#if FOLLY_HAS_COROUTINES
  if (cancelSource_) {
    cancelSource_->requestCancellation();
  }
#endif
  evb_ = nullptr;
  state_.store(State::INIT);
}

bool PacketStreamClient::isConnectedToServer() {
  return (state_.load() == State::CONNECTED);
}

void PacketStreamClient::registerPortToServer(const std::string& port) {
#if FOLLY_HAS_COROUTINES
  if (!isConnectedToServer()) {
    XLOG(ERR) << "Client not connected;";
    throw std::runtime_error("Client not connected;");
  }
  folly::coro::blockingWait(client_->co_registerPort(clientId_, port));
#else
  throw std::runtime_error("Coroutine support needed");
#endif
}

void PacketStreamClient::clearPortFromServer(const std::string& l2port) {
#if FOLLY_HAS_COROUTINES
  if (!isConnectedToServer()) {
    XLOG(ERR) << "Client not connected;";
    throw std::runtime_error("Client not connected;");
  }
  folly::coro::blockingWait(client_->co_clearPort(clientId_, l2port));
#else
  throw std::runtime_error("Coroutine support needed");
#endif
}

} // namespace fboss
} // namespace facebook
