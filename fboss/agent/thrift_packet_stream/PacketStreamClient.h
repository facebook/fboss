// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <fboss/agent/if/gen-cpp2/PacketStreamAsyncClient.h>
#if FOLLY_HAS_COROUTINES
#include <folly/coro/BlockingWait.h>
#endif
#include <folly/Synchronized.h>
#include <folly/io/async/ScopedEventBaseThread.h>
#include <unordered_map>

namespace facebook {
namespace fboss {
class PacketStreamClient {
 public:
  PacketStreamClient(const std::string& clientId, folly::EventBase* evb);

  virtual ~PacketStreamClient();
  void connectToServer(const std::string& ip, uint16_t port);
  void registerPortToServer(const std::string& port);
  void clearPortFromServer(const std::string& l2port);
  bool isConnectedToServer();
  void cancel();

 protected:
  // The derived client should implement this function which
  // will have the logic to do operation after receiving this
  // packet.
  virtual void recvPacket(TPacket&& packet) = 0;

 private:
  enum class State : uint16_t {
    INIT = 0,
    CONNECTING = 1,
    CONNECTED = 2,
    DISCONNECTED = 3,
  };
  void createClient(const std::string& ip, uint16_t port);

#if FOLLY_HAS_COROUTINES
  bool isConnectCancelled();
  folly::coro::Task<void> connect();
  folly::Synchronized<std::unique_ptr<folly::CancellationSource>> cancelSource_;
#endif
  std::string clientId_;
  std::unique_ptr<PacketStreamAsyncClient> client_;
  folly::EventBase* evb_;
  std::atomic<State> state_{State::INIT};
  std::unique_ptr<folly::ScopedEventBaseThread> clientEvbThread_;
};

} // namespace fboss
} // namespace facebook
