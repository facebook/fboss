// Copyright 2004-present Facebook. All Rights Reserved.
#include "fboss/agent/thrift_packet_stream/PacketStreamClient.h"
#include <folly/io/async/AsyncSocket.h>
#include <folly/logging/xlog.h>
#include <thrift/lib/cpp2/async/RocketClientChannel.h>
#include "folly/CancellationToken.h"

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
  XLOG(INFO) << "Destroying PacketStreamClient";
  cancel();
}

bool PacketStreamClient::isConnectedToServer() {
  return (state_.load() == State::CONNECTED && client_);
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
  auto state = state_.load();
  if (State::DISCONNECTED == state) {
    throw std::runtime_error("Client resumed after cancel called");
  }
  if (State::INIT != state) {
    XLOG(INFO) << "Client is already in process of connecting to server";
    return;
  }

  cancelSource_ = std::make_unique<folly::CancellationSource>();
  state_.store(State::CONNECTING);

  evb_->runInEventBaseThread([this, ip, port] {
    try {
      createClient(ip, port);
      if (isConnectCancelled()) {
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
  if (isConnectCancelled()) {
    XLOG(ERR) << "Cancellation Requested;";
    co_return;
  }
  state_.store(State::CONNECTED);
  XLOG(INFO) << clientId_ << " connected successfully";
  auto getToken = [this]() {
    return cancelSource_.withWLock(
        [](auto& cancelSource) { return cancelSource->getToken(); });
  };

  auto gen = std::move(result).toAsyncGenerator();
  try {
    while (auto packet = co_await folly::coro::co_withCancellation(
               getToken(), gen.next())) {
      recvPacket(std::move(*packet));
    }
  } catch (const folly::OperationCancelled&) {
    XLOG(WARNING) << "Packet Stream Operation cancelled";
  } catch (const std::exception& ex) {
    XLOG(ERR) << clientId_ << " Server error: " << folly::exceptionStr(ex);
    state_.store(State::INIT);
  }
  XLOG(INFO) << "Client Cancellation Completed";
  co_return;
}
#endif

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

void PacketStreamClient::cancel() {
#if FOLLY_HAS_COROUTINES
  XLOG(INFO) << "Cancel PacketStreamClient";
  cancelSource_.withWLock([](auto& cancelSource) {
    if (cancelSource) {
      XLOG(INFO) << "Request PacketStreamClient Cancellation";
      cancelSource->requestCancellation();
    }
  });

  // already disconnected;
  if (state_.load() == State::DISCONNECTED) {
    XLOG(WARNING) << clientId_ << " already disconnected";
    return;
  }

  evb_->runImmediatelyOrRunInEventBaseThreadAndWait([this]() {
    try {
      if (isConnectedToServer()) {
        folly::coro::blockingWait(client_->co_disconnect(clientId_));
      }
    } catch (const std::exception& ex) {
      XLOG(WARNING) << clientId_ << " disconnect failed:" << ex.what();
    }
  });

  clientEvbThread_->getEventBase()->runImmediatelyOrRunInEventBaseThreadAndWait(
      [this]() { client_.reset(); });
  // terminate event base getting ready for clean-up
  clientEvbThread_->getEventBase()->terminateLoopSoon();
  evb_->terminateLoopSoon();
#endif
  state_.store(State::DISCONNECTED);
}

#if FOLLY_HAS_COROUTINES
bool PacketStreamClient::isConnectCancelled() {
  return cancelSource_.withRLock([this](auto& cancelSource) -> bool {
    if (!cancelSource) {
      return true;
    }
    if (cancelSource->isCancellationRequested()) {
      state_.store(State::INIT);
      return true;
    }
    return false;
  });
}
#endif

} // namespace fboss
} // namespace facebook
