// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <fb303/ThreadCachedServiceData.h>
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
  BidirectionalPacketAcceptor(const BidirectionalPacketAcceptor&) = delete;
  BidirectionalPacketAcceptor& operator=(const BidirectionalPacketAcceptor&) =
      delete;
  BidirectionalPacketAcceptor(BidirectionalPacketAcceptor&&) = delete;
  BidirectionalPacketAcceptor& operator=(BidirectionalPacketAcceptor&&) =
      delete;
  virtual void recvPacket(TPacket&& packet) = 0;
};

class BidirectionalPacketStream
    : public std::enable_shared_from_this<BidirectionalPacketStream>,
      public PacketStreamService,
      public PacketStreamClient,
      public folly::AsyncTimeout {
 public:
  BidirectionalPacketStream(
      const std::string& serviceName,
      folly::EventBase* ioEventBase,
      folly::EventBase* timerEventBase,
      double timeout,
      BidirectionalPacketAcceptor* acceptor = nullptr);

  virtual ~BidirectionalPacketStream() override;
  BidirectionalPacketStream(const BidirectionalPacketStream&) = delete;
  BidirectionalPacketStream& operator=(const BidirectionalPacketStream&) =
      delete;
  BidirectionalPacketStream(BidirectionalPacketStream&&) = delete;
  BidirectionalPacketStream& operator=(BidirectionalPacketStream&&) = delete;

  void connectClient(uint16_t peerServerPort);
  // Should be called only when destruction. After stop client is called
  // connectClient should never be called again on this object.
  void stopClient();

  std::shared_ptr<AsyncPacketTransport> listen(const std::string& port);
  void close(const std::string& port);
  ssize_t send(TPacket&& packet);

  void setPacketAcceptor(BidirectionalPacketAcceptor* acceptor) {
    acceptor_.store(acceptor);
  }
  void timeoutExpired() noexcept override;

  bool isPortRegistered(const std::string& port) {
    return PacketStreamService::isPortRegistered(connectedClientId_, port);
  }

 protected:
  // client calls
  virtual void recvPacket(TPacket&& packet) override;
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
  fb303::TimeseriesWrapper STATS_err_port_register;
  fb303::TimeseriesWrapper STATS_err_invalid_connect_client_port;
  fb303::TimeseriesWrapper STATS_err_delete_port;
  fb303::TimeseriesWrapper STATS_err_pkt_recv_empty_port;
  fb303::TimeseriesWrapper STATS_err_acceptor_not_registered;
  fb303::TimeseriesWrapper STATS_err_send_pkt_failed;
  fb303::TimeseriesWrapper STATS_err_send_client_not_connected;
  fb303::TimeseriesWrapper STATS_pkt_recvd;
  fb303::TimeseriesWrapper STATS_pkt_send_success;
  fb303::TimeseriesWrapper STATS_start_reconnect_to_server;
  fb303::TimeseriesWrapper STATS_client_connected;
  fb303::TimeseriesWrapper STATS_client_disconnected;
  fb303::TimeseriesWrapper STATS_port_registered;
  fb303::TimeseriesWrapper STATS_port_removed;
};
} // namespace fboss
} // namespace facebook
