// Copyright 2004-present Facebook.  All rights reserved.
#pragma once

#include <fboss/agent/if/gen-cpp2/PacketStream.tcc>
#include "fboss/agent/AsyncPacketTransport.h"
#include "fboss/agent/thrift_packet_stream/BidirectionalPacketStream.h"

#include <memory>

namespace facebook {
namespace fboss {

class AsyncThriftPacketTransport : public AsyncPacketTransport {
 public:
  /**
   * Create a thrift stream
   */
  AsyncThriftPacketTransport(
      std::string l2Port,
      std::shared_ptr<facebook::fboss::BidirectionalPacketStream> server)
      : AsyncPacketTransport(std::move(l2Port)), server_(server) {}

  virtual ~AsyncThriftPacketTransport() override {}

  /**
   * Send the data in buffer to destination. Returns the return code from send.
   */
  virtual ssize_t send(const std::unique_ptr<folly::IOBuf>& buf) override;

  void recvPacket(TPacket&& packet) {
    if (!isReading()) {
      return;
    }
    readCallback_->onDataAvailable(folly::IOBuf::copyBuffer(*packet.buf_ref()));
  }
  /**
   * Stop listening on the socket.
   */
  virtual void close() override;

 private:
  AsyncThriftPacketTransport(const AsyncThriftPacketTransport&) = delete;
  AsyncThriftPacketTransport& operator=(const AsyncThriftPacketTransport&) =
      delete;
  std::weak_ptr<facebook::fboss::BidirectionalPacketStream> server_;
  std::string clientId_;
};

} // namespace fboss
} // namespace facebook
