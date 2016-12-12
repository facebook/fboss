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

namespace facebook { namespace fboss {

static constexpr int kQsfpServicePort = 5910;
static constexpr int kQsfpConnTimeoutMs = 2000;
static constexpr int kQsfpSendTimeoutMs = 5000;
static constexpr int kQsfpRecvTimeoutMs = 5000;

// static
std::unique_ptr<QsfpServiceAsyncClient> QsfpClient::createClient(
    folly::EventBase* eb) {
  // SR relies on both configerator and smcc being up
  // use raw thrift instead
  folly::SocketAddress addr("::1", kQsfpServicePort);
  auto socket = apache::thrift::async::TAsyncSocket::newSocket(
      eb, addr, kQsfpConnTimeoutMs);
  socket->setSendTimeout(kQsfpSendTimeoutMs);
  auto channel = apache::thrift::HeaderClientChannel::newChannel(socket);
  return std::make_unique<QsfpServiceAsyncClient>(std::move(channel));
}

// static
apache::thrift::RpcOptions QsfpClient::getRpcOptions(){
  apache::thrift::RpcOptions opts;
  opts.setTimeout(
      std::chrono::milliseconds(kQsfpRecvTimeoutMs));
  return opts;
}

}} // namespace facebook::fboss
