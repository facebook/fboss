// Copyright 2004-present Facebook. All Rights Reserved.
#include "fboss/agent/thrift_packet_stream/PacketStreamClient.h"
#include <folly/io/async/AsyncSocket.h>
#include <folly/logging/xlog.h>
#include <thrift/lib/cpp2/async/RocketClientChannel.h>
#include "folly/CancellationToken.h"

#if FOLLY_HAS_COROUTINES
#include <folly/coro/AsyncGenerator.h>
#include <folly/coro/WithCancellation.h>
#endif

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
  XLOG(DBG2) << "Destroying PacketStreamClient";
  cancel();
}

bool PacketStreamClient::isConnectedToServer() {
  return (state_.load() == State::CONNECTED && client_);
}

bool PacketStreamClient::send(TPacket&& packet) {
#if FOLLY_HAS_COROUTINES
  if (!sinkRunning_.load()) {
    XLOG(ERR) << clientId_ << " sink not ready, dropping packet";
    return false;
  }
  sinkQueue_.enqueue(std::move(packet));
  return true;
#else
  throw std::runtime_error("Coroutine support is needed for PacketStream");
#endif
}

bool PacketStreamClient::isSinkReady() {
#if FOLLY_HAS_COROUTINES
  return sinkRunning_.load();
#else
  return false;
#endif
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
    XLOG(DBG2) << "Client is already in process of connecting to server";
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
  XLOG(DBG2) << clientId_ << " connected successfully";
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
  XLOG(DBG2) << "Client Cancellation Completed";
  co_return;
}

folly::coro::Task<void> PacketStreamClient::sinkLoop(
    apache::thrift::ClientSink<TPacket, bool> sink) {
  try {
    sinkRunning_.store(true);
    XLOG(DBG2) << clientId_ << " sink loop started";

    co_await sink.sink([this]() -> folly::coro::AsyncGenerator<TPacket&&> {
      while (true) {
        auto packet = co_await sinkQueue_.dequeue();
        if (isConnectCancelled()) {
          XLOG(DBG2) << clientId_ << " sink loop cancelled via queue";
          co_return;
        }
        co_yield std::move(packet);
      }
    }());

    XLOG(DBG2) << clientId_ << " sink loop completed normally";
  } catch (const folly::OperationCancelled&) {
    XLOG(DBG2) << clientId_ << " sink loop cancelled";
  } catch (const std::exception& ex) {
    XLOG(ERR) << clientId_ << " sink loop error: " << ex.what();
  }
  sinkRunning_.store(false);
}
#endif

void PacketStreamClient::createSink() {
#if FOLLY_HAS_COROUTINES
  if (!isConnectedToServer()) {
    throw std::runtime_error("Client not connected");
  }

  auto getToken = [this]() {
    return cancelSource_.withWLock(
        [](auto& cancelSource) { return cancelSource->getToken(); });
  };

  // Create the sink on clientEvbThread_ (where the Thrift client lives).
  // This is a blocking call — if co_packetSink fails, the exception
  // propagates to the caller.
  auto sink = folly::coro::blockingWait(
      folly::coro::co_withExecutor(
          clientEvbThread_->getEventBase(),
          folly::coro::co_invoke(
              [this]() -> folly::coro::Task<
                           apache::thrift::ClientSink<TPacket, bool>> {
                co_return co_await client_->co_packetSink(clientId_);
              })));

  // Launch the drain loop as a managed coroutine on clientEvbThread_.
  // CancellableAsyncScope ensures cancel() waits for it to complete.
  sinkLoopScope_.add(
      folly::coro::co_withExecutor(
          clientEvbThread_->getEventBase(), sinkLoop(std::move(sink))),
      getToken());
#else
  throw std::runtime_error("Coroutine support is needed for PacketStream");
#endif
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

void PacketStreamClient::cancel() {
#if FOLLY_HAS_COROUTINES
  // already disconnected;
  if (state_.load() == State::DISCONNECTED) {
    XLOG(WARNING) << clientId_ << " already disconnected";
    return;
  }

  XLOG(DBG2) << "Cancel PacketStreamClient";
  cancelSource_.withWLock([](auto& cancelSource) {
    if (cancelSource) {
      XLOG(DBG2) << "Request PacketStreamClient Cancellation";
      cancelSource->requestCancellation();
    }
  });

  // Unblock sink queue with dummy packet so sinkLoop() can exit
  sinkQueue_.enqueue(TPacket{});

  // Wait for sinkLoop() coroutine to fully complete before destroying
  // any members it references (client_, sinkQueue_, etc.).
  folly::coro::blockingWait(sinkLoopScope_.cancelAndJoinAsync());

  evb_->runImmediatelyOrRunInEventBaseThreadAndWait([this]() {
    try {
      if (isConnectedToServer()) {
        folly::coro::blockingWait(client_->co_disconnect(clientId_));
      }
    } catch (const std::exception& ex) {
      XLOG(WARNING) << clientId_ << " disconnect failed:" << ex.what();
    }
  });
#endif

  // Flip to disconnected early so other threads won't try to touch client_
  // while we're destroying it.
  state_.store(State::DISCONNECTED);

#if FOLLY_HAS_COROUTINES
  clientEvbThread_->getEventBase()->runImmediatelyOrRunInEventBaseThreadAndWait(
      [this]() { client_.reset(); });
  // terminate event base getting ready for clean-up
  clientEvbThread_->getEventBase()->terminateLoopSoon();
  evb_->terminateLoopSoon();
#endif
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
