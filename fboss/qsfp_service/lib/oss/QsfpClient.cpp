// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/qsfp_service/lib/QsfpClient.h"

#include <folly/SocketAddress.h>
#include <folly/io/async/AsyncSocket.h>
#include <thrift/lib/cpp2/async/HeaderClientChannel.h>

namespace facebook::fboss {

static constexpr int kQsfpSendTimeoutMs = 5000;
static constexpr int kQsfpConnTimeoutMs = 2000;

// static
folly::Future<std::unique_ptr<apache::thrift::Client<QsfpService>>>
QsfpClient::createClient(folly::EventBase* eb) {
  auto createClient = [eb]() {
    folly::SocketAddress addr(FLAGS_qsfp_service_host, FLAGS_qsfp_service_port);
    auto socket = folly::AsyncSocket::newSocket(eb, addr, kQsfpConnTimeoutMs);
    socket->setSendTimeout(kQsfpSendTimeoutMs);
    auto channel =
        apache::thrift::HeaderClientChannel::newChannel(std::move(socket));
    return std::make_unique<apache::thrift::Client<QsfpService>>(
        std::move(channel));
  };
  return folly::via(eb, createClient);
}

} // namespace facebook::fboss
