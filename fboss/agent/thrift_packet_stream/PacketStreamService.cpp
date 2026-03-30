// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/thrift_packet_stream/PacketStreamService.h"

#include <folly/logging/xlog.h>

#if FOLLY_HAS_COROUTINES
#include <folly/coro/AsyncGenerator.h>
#include <folly/coro/WithCancellation.h>
#endif

DEFINE_int32(
    packet_stream_rx_buffer_size,
    1000,
    "Buffer size for packet stream sink (Rx path)");

namespace facebook {
namespace fboss {

PacketStreamService::~PacketStreamService() {
  try {
#if FOLLY_HAS_COROUTINES
    rxCancellationSources_.withWLock([](auto& sources) {
      for (auto& [clientId, source] : sources) {
        source.requestCancellation();
        XLOG(DBG2) << "Cancelled Rx sink for client: " << clientId;
      }
      sources.clear();
    });
#endif

    clientMap_.withWLock([](auto& lockedMap) {
      for (auto& iter : lockedMap) {
        auto& clientInfo = iter.second;
        auto publisher = std::move(clientInfo.publisher_);
        std::move(*publisher.get()).complete();
        XLOG(DBG2) << "Completed Tx stream for client: " << iter.first;
      }
      lockedMap.clear();
    });
  } catch (const std::exception&) {
    XLOG(ERR) << "Failed to close the thrift stream;";
  }
}

static TPacketException createTPacketException(
    TPacketErrorCode code,
    const std::string& msg) {
  TPacketException ex;
  ex.code() = code;
  ex.msg() = msg;
  return ex;
}

apache::thrift::ServerStream<TPacket> PacketStreamService::connect(
    std::unique_ptr<std::string> clientIdPtr) {
  try {
    if (!clientIdPtr || clientIdPtr->empty()) {
      XLOG(ERR) << "Invalid Client";
      throw createTPacketException(
          TPacketErrorCode::INVALID_CLIENT, "Invalid client");
    }
    const auto& clientId = *clientIdPtr;
    auto streamAndPublisher =
        apache::thrift::ServerStream<TPacket>::createPublisher(
            [client = clientId, this] {
              // when the client is disconnected run this section.
              XLOG(DBG2) << "Client disconnected: " << client;
              clientMap_.withWLock([client = client](auto& lockedMap) {
                lockedMap.erase(client);
              });
              clientDisconnected(client);
            });

    clientMap_.withWLock(
        [client = clientId,
         &publisher = streamAndPublisher.second](auto& lockedMap) {
          lockedMap.emplace(
              std::make_pair(client, ClientInfo(std::move(publisher))));
        });
    clientConnected(clientId);
    XLOG(DBG2) << clientId << " connected successfully to PacketStreamService";
    return std::move(streamAndPublisher.first);
  } catch (const std::exception& except) {
    XLOG(ERR) << "connect failed with exp:" << except.what();
    throw createTPacketException(
        TPacketErrorCode::INTERNAL_ERROR, except.what());
  }
}

void PacketStreamService::send(const std::string& clientId, TPacket&& packet) {
  clientMap_.withRLock([&](auto& lockedMap) {
    auto iter = lockedMap.find(clientId);
    if (iter == lockedMap.end()) {
      XLOG(ERR) << "Client '" << clientId << "' Not Connected";
      throw createTPacketException(
          TPacketErrorCode::CLIENT_NOT_CONNECTED, "client not connected");
    }
    const auto& clientInfo = iter->second;
    // ignore port registration if not required
    if (portRegistration_) {
      auto portIter = clientInfo.portList_.find(*packet.l2Port());
      if (portIter == clientInfo.portList_.end()) {
        XLOG(ERR) << "Port '" << *packet.l2Port() << "' not Registered";
        throw createTPacketException(
            TPacketErrorCode::PORT_NOT_REGISTERED, "PORT not registered");
      }
    }
    clientInfo.publisher_->next(packet);
  });
}

bool PacketStreamService::isClientConnected(const std::string& clientId) {
  return clientMap_.withRLock([&](auto& lockedMap) {
    auto iter = lockedMap.find(clientId);
    if (iter == lockedMap.end()) {
      return false;
    }
    return true;
  });
}

bool PacketStreamService::isPortRegistered(
    const std::string& clientId,
    const std::string& port) {
  return clientMap_.withRLock([&](auto& lockedMap) {
    auto iter = lockedMap.find(clientId);
    if (iter == lockedMap.end()) {
      return false;
    }
    const auto& clientInfo = iter->second;
    auto portIter = clientInfo.portList_.find(port);
    if (portIter == clientInfo.portList_.end()) {
      return false;
    }
    return true;
  });
}
void PacketStreamService::registerPort(
    std::unique_ptr<std::string> clientIdPtr,
    std::unique_ptr<std::string> l2PortPtr) {
  if (!clientIdPtr || clientIdPtr->empty()) {
    XLOG(ERR) << "Invalid Client";
    throw createTPacketException(
        TPacketErrorCode::INVALID_CLIENT, "Invalid client");
  }

  if (!l2PortPtr || l2PortPtr->empty()) {
    XLOG(ERR) << "Invalid port";
    throw createTPacketException(
        TPacketErrorCode::INVALID_L2PORT, "Invalid Port");
  }
  const auto& clientId = *clientIdPtr;
  const auto& l2Port = *l2PortPtr;

  clientMap_.withWLock([&](auto& lockedMap) {
    auto iter = lockedMap.find(clientId);
    if (iter == lockedMap.end()) {
      XLOG(ERR) << "Client " << clientId << " not found";
      throw createTPacketException(
          TPacketErrorCode::CLIENT_NOT_CONNECTED, "client not connected");
    }
    auto& clientInfo = iter->second;
    clientInfo.portList_.insert(l2Port);
    addPort(clientId, l2Port);
  });
}
void PacketStreamService::clearPort(
    std::unique_ptr<std::string> clientIdPtr,
    std::unique_ptr<std::string> l2PortPtr) {
  if (!clientIdPtr || clientIdPtr->empty()) {
    throw createTPacketException(
        TPacketErrorCode::INVALID_CLIENT, "Invalid client");
  }

  if (!l2PortPtr || l2PortPtr->empty()) {
    throw createTPacketException(
        TPacketErrorCode::INVALID_L2PORT, "Invalid Port");
  }
  const auto& clientId = *clientIdPtr;
  const auto& l2Port = *l2PortPtr;

  clientMap_.withWLock([&](auto& lockedMap) {
    auto iter = lockedMap.find(clientId);
    if (iter == lockedMap.end()) {
      XLOG(ERR) << "Client not connected";
      throw createTPacketException(
          TPacketErrorCode::CLIENT_NOT_CONNECTED, "client not connected");
    }
    auto& clientInfo = iter->second;
    auto portIter = clientInfo.portList_.find(l2Port);
    if (portIter == clientInfo.portList_.end()) {
      throw createTPacketException(
          TPacketErrorCode::PORT_NOT_REGISTERED, "PORT not registered");
    }
    clientInfo.portList_.erase(l2Port);
    removePort(clientId, l2Port);
  });
}
void PacketStreamService::disconnect(std::unique_ptr<std::string> clientIdPtr) {
  if (!clientIdPtr || clientIdPtr->empty()) {
    throw createTPacketException(
        TPacketErrorCode::INVALID_CLIENT, "Invalid client");
  }

  const auto& clientId = *clientIdPtr;

  clientMap_.withWLock([&](auto& lockedMap) {
    auto iter = lockedMap.find(clientId);
    if (iter == lockedMap.end()) {
      throw createTPacketException(
          TPacketErrorCode::CLIENT_NOT_CONNECTED, "client not connected");
    }
    auto& clientInfo = iter->second;
    auto publisher = std::move(clientInfo.publisher_);
    std::move(*publisher.get()).complete();
    lockedMap.erase(iter);
    clientDisconnected(clientId);
  });
}

void PacketStreamService::processReceivedPacket(
    const std::string& clientId,
    TPacket&& packet) {
  XLOG(DBG3) << "PacketStreamService: Dropping packet from client " << clientId
             << " on port " << *packet.l2Port() << " (no handler override)";
}

#if FOLLY_HAS_COROUTINES
folly::coro::Task<apache::thrift::SinkConsumer<TPacket, bool>>
PacketStreamService::co_packetSink(std::unique_ptr<std::string> clientIdPtr) {
  if (!clientIdPtr || clientIdPtr->empty()) {
    XLOG(ERR) << "Invalid client ID for packetSink";
    throw createTPacketException(
        TPacketErrorCode::INVALID_CLIENT, "Invalid client");
  }

  const auto& clientIdStr = *clientIdPtr;

  auto cancellationSource = folly::CancellationSource();
  rxCancellationSources_.wlock()->emplace(clientIdStr, cancellationSource);

  XLOG(DBG2) << "Client " << clientIdStr << " starting packet Rx sink";

  co_return apache::thrift::SinkConsumer<TPacket, bool>{
      [this, clientIdStr, cancellationSource = std::move(cancellationSource)](
          folly::coro::AsyncGenerator<TPacket&&> gen)
          -> folly::coro::Task<bool> {
        XLOG(DBG2) << "Sink consumption started for client: " << clientIdStr;

        try {
          while (auto item = co_await folly::coro::co_withCancellation(
                     cancellationSource.getToken(), gen.next())) {
            processReceivedPacket(clientIdStr, std::move(*item));
          }

          XLOG(DBG2) << "Sink completed normally for client: " << clientIdStr;
        } catch (const folly::OperationCancelled&) {
          XLOG(DBG2) << "Sink cancelled for client: " << clientIdStr;
        } catch (const std::exception& e) {
          XLOG(ERR) << "Sink error for client " << clientIdStr << ": "
                    << e.what();
          rxCancellationSources_.wlock()->erase(clientIdStr);
          co_return false;
        }

        rxCancellationSources_.wlock()->erase(clientIdStr);
        co_return true;
      },
      FLAGS_packet_stream_rx_buffer_size > 0
          ? static_cast<uint64_t>(FLAGS_packet_stream_rx_buffer_size)
          : 1000}
      .setChunkTimeout(std::chrono::milliseconds(0));
}
#endif

} // namespace fboss
} // namespace facebook
