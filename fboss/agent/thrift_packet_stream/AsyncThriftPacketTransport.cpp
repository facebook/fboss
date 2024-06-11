// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/thrift_packet_stream/AsyncThriftPacketTransport.h"
#include "fboss/agent/thrift_packet_stream/BidirectionalPacketStream.h"

#include <folly/logging/xlog.h>

namespace facebook {
namespace fboss {
ssize_t AsyncThriftPacketTransport::send(
    const std::unique_ptr<folly::IOBuf>& buf) {
  if (!buf) {
    return 0;
  }
  TPacket packet;
  *packet.l2Port() = iface();
  *packet.buf() = buf->to<std::string>();
  if (auto serverSharedPtr = server_.lock()) {
    return serverSharedPtr->send(std::move(packet));
  }
  XLOG(ERR) << "AsyncThriftPacketTransport server not available";
  return 0;
}

void AsyncThriftPacketTransport::close() {
  if (auto serverSharedPtr = server_.lock()) {
    serverSharedPtr->close(iface());
  }
}

} // namespace fboss
} // namespace facebook
