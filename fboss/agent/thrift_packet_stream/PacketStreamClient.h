// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <fboss/agent/if/gen-cpp2/PacketStreamAsyncClient.h>
#include <folly/experimental/coro/BlockingWait.h>

#include <unordered_map>

namespace facebook {
namespace fboss {
class PacketStreamClient {
 public:
  explicit PacketStreamClient(
      const std::string& clientId,
      folly::EventBase* evb);

  virtual ~PacketStreamClient();
  void connectToServer(std::unique_ptr<PacketStreamAsyncClient> client);
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
  };
  folly::coro::Task<void> connect();
  std::string clientId_;
  std::unique_ptr<folly::CancellationSource> cancelSource_;
  std::unique_ptr<PacketStreamAsyncClient> client_;
  folly::EventBase* evb_;
  std::atomic<State> state_{State::INIT};
};

} // namespace fboss
} // namespace facebook
