// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/thrift_packet_stream/BidirectionalPacketStream.h"
#include "fboss/agent/thrift_packet_stream/AsyncThriftPacketTransport.h"
#include "servicerouter/client/cpp2/ServiceRouter.h"

namespace facebook {
namespace fboss {

BidirectionalPacketStream::~BidirectionalPacketStream() {
  LOG(INFO) << "Closing Bidirectional stream:";
  if (evb_) {
    evb_->runImmediatelyOrRunInEventBaseThreadAndWait(
        [this]() { cancelTimeout(); });
  }
}

std::unique_ptr<facebook::fboss::PacketStreamAsyncClient>
BidirectionalPacketStream::createClient() {
  // create an SR thrift client
  facebook::servicerouter::ServiceOptions opts;
  opts["single_host"] = {"::1", folly::to<std::string>(peerServerPort_.load())};
  opts["svc_select_count"] = {"1"};
  facebook::servicerouter::ConnConfigs cfg;
  cfg["thrift_transport"] = "rocket";
  cfg["thrift_security"] = "disabled";
  // TODO (shankaran): Add meaningful information for debugging.
  cfg["client_id"] = serviceName_;
  return std::make_unique<facebook::fboss::PacketStreamAsyncClient>(
      facebook::servicerouter::cpp2::getClientFactory().getChannel(
          "", nullptr, opts, cfg));
}

void BidirectionalPacketStream::registerPortsToServer() {
  if (PacketStreamClient::isConnectedToServer() && newConnection_.load()) {
    try {
      portMap_.withWLock([&](auto& lockedMap) {
        for (const auto& port : lockedMap) {
          // register the port for local server
          PacketStreamService::registerPort(
              std::make_unique<std::string>(connectedClientId_),
              std::make_unique<std::string>(port));
          // now lets ask the server to register port.
          PacketStreamClient::registerPortToServer(port);
          LOG(INFO) << "Registered Port:" << port << " successfully with "
                    << connectedClientId_;
        }
      });
      newConnection_.store(false);
    } catch (const std::exception& ex) {
      LOG(ERROR) << "Failed to register ports err:" << ex.what();
    }
  }
}

void BidirectionalPacketStream::connectClient(uint16_t port) {
  if (!port) {
    throw std::runtime_error("Invalid port");
  }
  LOG(INFO) << "Starting Connection to Server: " << port;
  peerServerPort_.store(port);
  newConnection_.store(true);
  PacketStreamClient::connectToServer(createClient());
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
    LOG(INFO) << "Reconnecting to server on port: " << peerServerPort_.load();

    newConnection_.store(true);
    PacketStreamClient::connectToServer(createClient());
  }
  scheduleTimeout(timeout_);
}

std::shared_ptr<AsyncPacketTransport> BidirectionalPacketStream::listen(
    const std::string& port) {
  if (port.empty()) {
    return {};
  }
  LOG(INFO) << "Start listening on Port: " << port;
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
      LOG(INFO) << "Registered Port:" << port << " successfully with "
                << connectedClientId_;

    } catch (const std::exception& ex) {
      LOG(ERROR) << "Failed to register port: " << port << " err:" << ex.what();
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
      LOG(ERROR) << "Error deleting the port: " << ex.what();
    }
  }
  LOG(INFO) << "Unregister Port:" << port;
}

void BidirectionalPacketStream::recvPacket(TPacket&& packet) {
  const auto& port = *packet.l2Port_ref();
  if (port.empty()) {
    LOG(ERROR) << "Packet received with port empty";
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
    LOG(ERROR) << "Packet received for port:" << port
               << " that's doesn't have transport to forward";
    throw std::runtime_error("acceptor not registered");
  }
}

ssize_t BidirectionalPacketStream::send(TPacket&& packet) {
  if (!clientConnected_.load()) {
    LOG(ERROR) << "client not yet connected";
    return -1;
  }
  ssize_t sz = packet.buf_ref()->size();
  try {
    // call the packetstreamservice send method to send the packet.
    PacketStreamService::send(connectedClientId_, std::move(packet));
  } catch (const std::exception& ex) {
    LOG(ERROR) << "send packet failed:" << ex.what();
    // TODO:(shankaran) - Try to reconnect when there is a failure.
    return -1;
  }
  return sz;
}

// server calls. Right now supports one birectional connection so we don't
// care about the client id.
void BidirectionalPacketStream::clientConnected(const std::string& clientId) {
  clientConnected_.store(true);
  connectedClientId_ = clientId;
}
void BidirectionalPacketStream::clientDisconnected(
    const std::string& /* clientId */) {
  clientConnected_.store(false);
  connectedClientId_.clear();
}

// server stream callbacks. We don't want to do any extra steps when
// client registers/removes the port.
void BidirectionalPacketStream::addPort(
    const std::string& /* clientId */,
    const std::string& /* l2Port */) {}
void BidirectionalPacketStream::removePort(
    const std::string& /* clientId */,
    const std::string& /* l2Port */) {}

} // namespace fboss
} // namespace facebook
