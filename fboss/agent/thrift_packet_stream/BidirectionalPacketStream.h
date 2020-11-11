// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/AsyncPacketTransport.h"
#include "fboss/agent/thrift_packet_stream/PacketStreamClient.h"
#include "fboss/agent/thrift_packet_stream/PacketStreamService.h"
#include "folly/Synchronized.h"

#include <memory>

namespace facebook {
namespace fboss {
class BidirectionalPacketAcceptor {
 public:
  BidirectionalPacketAcceptor() = default;
  virtual ~BidirectionalPacketAcceptor() = default;
  virtual void recvPacket(Packet&& packet) = 0;
};

class BidirectionalPacketStream
    : public std::enable_shared_from_this<BidirectionalPacketStream>,
      public PacketStreamService,
      public PacketStreamClient,
      public folly::AsyncTimeout {
 public:
  explicit BidirectionalPacketStream(
      const std::string& serviceName,
      folly::EventBase* ioEventBase,
      folly::EventBase* timerEventBase,
      double timeout,
      BidirectionalPacketAcceptor* acceptor = nullptr)
      : PacketStreamService(serviceName),
        PacketStreamClient(serviceName, ioEventBase),
        folly::AsyncTimeout(timerEventBase),
        evb_(timerEventBase),
        timeout_(timeout),
        acceptor_(acceptor),
        serviceName_(serviceName) {
    if (!evb_ || timeout <= 0) {
      throw std::runtime_error("Invalid timer settings");
    }
    // timer will be started when connectClient call is made.
  }

  virtual ~BidirectionalPacketStream() override;

  void connectClient(uint16_t peerServerPort);
  // Should be called only when destruction. After stop client is called
  // connectClient should never be called again on this object.
  void stopClient();

  std::shared_ptr<AsyncPacketTransport> listen(const std::string& port);
  void close(const std::string& port);
  ssize_t send(Packet&& packet);

  void setPacketAcceptor(BidirectionalPacketAcceptor* acceptor) {
    acceptor_.store(acceptor);
  }
  void timeoutExpired() noexcept override;

 protected:
  // client calls
  virtual void recvPacket(Packet&& packet) override;
  // server calls
  virtual void clientConnected(const std::string& clientId) override;
  virtual void clientDisconnected(const std::string& clientId) override;
  virtual void addPort(const std::string& clientId, const std::string& l2Port)
      override;
  virtual void removePort(
      const std::string& clientId,
      const std::string& l2Port) override;

 private:
  void registerPortsToServer();
  std::unique_ptr<facebook::fboss::PacketStreamAsyncClient> createClient();
  folly::EventBase* evb_;
  double timeout_;
  using TransportMap =
      std::unordered_map<std::string, std::shared_ptr<AsyncPacketTransport>>;
  folly::Synchronized<TransportMap> transportMap_;
  folly::Synchronized<std::unordered_set<std::string>> portMap_;
  std::string connectedClientId_;
  std::atomic<bool> clientConnected_{false};
  std::atomic<BidirectionalPacketAcceptor*> acceptor_{nullptr};
  std::atomic<uint16_t> peerServerPort_{0};
  std::string serviceName_;
  std::atomic<bool> newConnection_{false};
}; // namespace fboss

} // namespace fboss
} // namespace facebook
