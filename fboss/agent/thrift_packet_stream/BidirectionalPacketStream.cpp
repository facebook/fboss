// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/thrift_packet_stream/BidirectionalPacketStream.h"
#include "fboss/agent/thrift_packet_stream/AsyncThriftPacketTransport.h"

#include <folly/logging/xlog.h>

namespace facebook {
namespace fboss {

#define BIDIRECTIONAL_INIT_STATS(serviceName)                                  \
  STATS_err_port_register(                                                     \
      folly::to<std::string>(serviceName, ".err.port_register"),               \
      fb303::SUM,                                                              \
      fb303::RATE),                                                            \
      STATS_err_invalid_connect_client_port(                                   \
          folly::to<std::string>(                                              \
              serviceName, ".err.invalid_connect_client_port"),                \
          fb303::SUM,                                                          \
          fb303::RATE),                                                        \
      STATS_err_delete_port(                                                   \
          folly::to<std::string>(serviceName, ".err.delete_port"),             \
          fb303::SUM,                                                          \
          fb303::RATE),                                                        \
      STATS_err_pkt_recv_empty_port(                                           \
          folly::to<std::string>(serviceName, ".err.pkt_recv_empty_port"),     \
          fb303::SUM,                                                          \
          fb303::RATE),                                                        \
      STATS_err_acceptor_not_registered(                                       \
          folly::to<std::string>(serviceName, ".err.acceptor_not_registered"), \
          fb303::SUM,                                                          \
          fb303::RATE),                                                        \
      STATS_err_send_pkt_failed(                                               \
          folly::to<std::string>(serviceName, ".err.send_pkt_failed"),         \
          fb303::SUM,                                                          \
          fb303::RATE),                                                        \
      STATS_err_send_client_not_connected(                                     \
          folly::to<std::string>(                                              \
              serviceName, ".err.send_client_not_connected"),                  \
          fb303::SUM,                                                          \
          fb303::RATE),                                                        \
      STATS_pkt_recvd(                                                         \
          folly::to<std::string>(serviceName, ".pkt_recvd"),                   \
          fb303::SUM,                                                          \
          fb303::RATE),                                                        \
      STATS_pkt_send_success(                                                  \
          folly::to<std::string>(serviceName, ".pkt_send_success"),            \
          fb303::SUM,                                                          \
          fb303::RATE),                                                        \
      STATS_start_reconnect_to_server(                                         \
          folly::to<std::string>(serviceName, ".start_reconnect_to_server"),   \
          fb303::SUM,                                                          \
          fb303::RATE),                                                        \
      STATS_client_connected(                                                  \
          folly::to<std::string>(serviceName, ".client_connected"),            \
          fb303::SUM,                                                          \
          fb303::RATE),                                                        \
      STATS_client_disconnected(                                               \
          folly::to<std::string>(serviceName, ".client_disconnected"),         \
          fb303::SUM,                                                          \
          fb303::RATE),                                                        \
      STATS_port_registered(                                                   \
          folly::to<std::string>(serviceName, ".port_registered"),             \
          fb303::SUM,                                                          \
          fb303::RATE),                                                        \
      STATS_port_removed(                                                      \
          folly::to<std::string>(serviceName, ".port_removed"),                \
          fb303::SUM,                                                          \
          fb303::RATE)

BidirectionalPacketStream::BidirectionalPacketStream(
    const std::string& serviceName,
    folly::EventBase* ioEventBase,
    folly::EventBase* timerEventBase,
    double timeout,
    BidirectionalPacketAcceptor* acceptor)
    : PacketStreamService(serviceName),
      PacketStreamClient(serviceName, ioEventBase),
      folly::AsyncTimeout(timerEventBase),
      evb_(timerEventBase),
      timeout_(timeout),
      acceptor_(acceptor),
      serviceName_(serviceName),
      BIDIRECTIONAL_INIT_STATS(serviceName) {
  if (!evb_ || timeout <= 0) {
    throw std::runtime_error("Invalid timer settings");
  }
  // timer will be started when connectClient call is made.
}

BidirectionalPacketStream::~BidirectionalPacketStream() {
  XLOG(INFO) << "Closing Bidirectional stream:";
  if (evb_) {
    evb_->runImmediatelyOrRunInEventBaseThreadAndWait(
        [this]() { cancelTimeout(); });
  }
}

void BidirectionalPacketStream::registerPortsToServer() {
  if (newConnection_.load() && PacketStreamClient::isConnectedToServer() &&
      clientConnected_.load()) {
    try {
      portMap_.withWLock([&](auto& lockedMap) {
        for (const auto& port : lockedMap) {
          // register the port for local server
          XLOG(INFO) << serviceName_ << ": Register Port " << port;
          PacketStreamService::registerPort(
              std::make_unique<std::string>(connectedClientId_),
              std::make_unique<std::string>(port));
          // now lets ask the server to register port.
          PacketStreamClient::registerPortToServer(port);
          XLOG(INFO) << "Registered Port:" << port << " successfully with "
                     << connectedClientId_;
        }
      });
      newConnection_.store(false);
    } catch (const std::exception& ex) {
      STATS_err_port_register.add(1);
      XLOG(ERR) << "Failed to register ports err:" << ex.what();
    }
  }
}

void BidirectionalPacketStream::connectClient(uint16_t port) {
  if (!port) {
    STATS_err_invalid_connect_client_port.add(1);
    throw std::runtime_error("Invalid port");
  }
  XLOG(INFO) << serviceName_ << ": Starting Connection to Server: " << port;
  peerServerPort_.store(port);
  newConnection_.store(true);
  PacketStreamClient::connectToServer("::1", port);
  registerPortsToServer();

  evb_->runInEventBaseThread([this]() { scheduleTimeout(timeout_); });
}
void BidirectionalPacketStream::stopClient() {
  evb_->runImmediatelyOrRunInEventBaseThreadAndWait(
      [this]() { cancelTimeout(); });
  PacketStreamClient::cancel();
  // called when destructing so we can clean up evb_
  evb_ = nullptr;
}

void BidirectionalPacketStream::timeoutExpired() noexcept {
  if (PacketStreamClient::isConnectedToServer()) {
    registerPortsToServer();
  } else {
    // try to reconnect.
    STATS_start_reconnect_to_server.add(1);
    auto port = peerServerPort_.load();
    XLOG(INFO) << serviceName_ << ": Reconnecting to server on port: " << port;
    newConnection_.store(true);
    PacketStreamClient::connectToServer("::1", port);
  }
  scheduleTimeout(timeout_);
}

std::shared_ptr<AsyncPacketTransport> BidirectionalPacketStream::listen(
    const std::string& port) {
  if (port.empty()) {
    return {};
  }
  XLOG(INFO) << serviceName_ << ": Start listening on Port: " << port;
  portMap_.withWLock([&](auto& lockedMap) { lockedMap.emplace(port); });
  if (clientConnected_.load() && PacketStreamClient::isConnectedToServer()) {
    // lets try to register the port.
    try {
      // register the port for local server
      PacketStreamService::registerPort(
          std::make_unique<std::string>(connectedClientId_),
          std::make_unique<std::string>(port));
      // now lets ask the server to register port.
      PacketStreamClient::registerPortToServer(port);
      XLOG(INFO) << "Registered Port:" << port << " successfully with "
                 << connectedClientId_;

    } catch (const std::exception& ex) {
      XLOG(ERR) << "Failed to register port: " << port << " err:" << ex.what();
      STATS_err_port_register.add(1);
      return {};
    }
  }

  std::shared_ptr<AsyncPacketTransport> transport =
      transportMap_.withWLock([&](auto& lockedMap) {
        auto iter = lockedMap.find(port);
        if (iter != lockedMap.end()) {
          return iter->second;
        }
        std::shared_ptr<AsyncPacketTransport> ltransport =
            std::make_shared<AsyncThriftPacketTransport>(
                port, shared_from_this());
        lockedMap.emplace(port, ltransport);
        return ltransport;
      });
  return transport;
}

void BidirectionalPacketStream::close(const std::string& port) {
  if (port.empty()) {
    return;
  }

  portMap_.withWLock([&](auto& lockedMap) { lockedMap.erase(port); });
  transportMap_.withWLock([&](auto& lockedMap) { lockedMap.erase(port); });

  if (isConnectedToServer()) {
    try {
      // clear local port registration.
      PacketStreamService::clearPort(
          std::make_unique<std::string>(connectedClientId_),
          std::make_unique<std::string>(port));
      // clear remove server port registration.
      PacketStreamClient::clearPortFromServer(port);
    } catch (const std::exception& ex) {
      XLOG(ERR) << "Error deleting the port: " << ex.what();
      STATS_err_delete_port.add(1);
      return;
    }
  }
  XLOG(INFO) << "Unregister Port:" << port;
}

void BidirectionalPacketStream::recvPacket(TPacket&& packet) {
  const auto& port = *packet.l2Port_ref();
  STATS_pkt_recvd.add(1);
  if (port.empty()) {
    XLOG(ERR) << "Packet received with port empty";
    STATS_err_pkt_recv_empty_port.add(1);
    return;
  }
  bool failed = transportMap_.withRLock([&](auto& lockedMap) {
    auto iter = lockedMap.find(port);
    if (iter != lockedMap.end()) {
      auto* transport =
          reinterpret_cast<AsyncThriftPacketTransport*>(iter->second.get());
      transport->recvPacket(std::move(packet));
      return false;
    }
    // pass the packet to default receiver.
    auto acceptor = acceptor_.load();
    if (acceptor) {
      acceptor->recvPacket(std::move(packet));
      return false;
    }
    return true;
  });
  if (failed) {
    XLOG(ERR) << "Packet received for port:" << port
              << " that's doesn't have transport to forward";
    throw std::runtime_error("acceptor not registered");
  }
}

ssize_t BidirectionalPacketStream::send(TPacket&& packet) {
  if (!clientConnected_.load()) {
    STATS_err_send_client_not_connected.add(1);
    XLOG(ERR) << "client not yet connected";
    return -1;
  }
  ssize_t sz = packet.buf_ref()->size();
  try {
    // call the packetstreamservice send method to send the packet.
    PacketStreamService::send(connectedClientId_, std::move(packet));
  } catch (const std::exception& ex) {
    XLOG(ERR) << "send packet failed:" << ex.what();
    STATS_err_send_pkt_failed.add(1);
    // TODO:(shankaran) - Try to reconnect when there is a failure.
    return -1;
  }
  STATS_pkt_send_success.add(1);
  return sz;
}

// server calls. Right now supports one birectional connection so we don't
// care about the client id.
void BidirectionalPacketStream::clientConnected(const std::string& clientId) {
  clientConnected_.store(true);
  connectedClientId_ = clientId;
  STATS_client_connected.add(1);
}
void BidirectionalPacketStream::clientDisconnected(
    const std::string& /* clientId */) {
  clientConnected_.store(false);
  connectedClientId_.clear();
  STATS_client_disconnected.add(1);
}

// server stream callbacks. We don't want to do any extra steps when
// client registers/removes the port.
void BidirectionalPacketStream::addPort(
    const std::string& /* clientId */,
    const std::string& /* l2Port */) {
  STATS_port_registered.add(1);
}
void BidirectionalPacketStream::removePort(
    const std::string& /* clientId */,
    const std::string& /* l2Port */) {
  STATS_port_removed.add(1);
}

} // namespace fboss
} // namespace facebook
