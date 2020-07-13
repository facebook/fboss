/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "QsfpClient.h"

#include <folly/io/async/AsyncSocket.h>

DEFINE_string(qsfp_service_host, "::1", "Host running qsfp service");
DEFINE_int32(qsfp_service_port, 5910, "Port running qsfp service");
DEFINE_int32(qsfp_service_recv_timeout,
             5000,
             "Receive timeout(ms) from qsfp service");

namespace facebook { namespace fboss {

static constexpr int kQsfpConnTimeoutMs = 2000;
static constexpr int kQsfpSendTimeoutMs = 5000;

// static
folly::Future<std::unique_ptr<QsfpServiceAsyncClient>>
QsfpClient::createClient(folly::EventBase* eb) {
  // SR relies on both configerator and smcc being up
  // use raw thrift instead
  auto createClient = [eb]() {
    folly::SocketAddress addr(FLAGS_qsfp_service_host, FLAGS_qsfp_service_port);
    auto socket = folly::AsyncSocket::newSocket(
        eb, addr, kQsfpConnTimeoutMs);
    socket->setSendTimeout(kQsfpSendTimeoutMs);
    auto channel = apache::thrift::HeaderClientChannel::newChannel(std::move(socket));
    return std::make_unique<QsfpServiceAsyncClient>(std::move(channel));
  };
  return folly::via(eb, createClient);
}

// static
apache::thrift::RpcOptions QsfpClient::getRpcOptions(){
  apache::thrift::RpcOptions opts;
  opts.setTimeout(
      std::chrono::milliseconds(FLAGS_qsfp_service_recv_timeout));
  return opts;
}

}} // namespace facebook::fboss
