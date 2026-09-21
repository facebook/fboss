/*
 *  Copyright (c) Meta Platforms, Inc. and affiliates.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 *  Open-source implementation of getPaiDiagStreamingClient — uses a raw
 *  AsyncSocket + RocketClientChannel. No ServiceRouter / SSL.
 *
 *  Mirrors fboss/agent/hw/sai/diag/oss/DiagShellClient.cpp.
 */
#include "fboss/qsfp_service/diag/PaiDiagShellClient.h"

#include <folly/SocketAddress.h>
#include <folly/io/async/AsyncSocket.h>
#include <folly/io/async/EventBase.h>
#include <thrift/lib/cpp2/async/RocketClientChannel.h>

namespace facebook::fboss {

std::unique_ptr<apache::thrift::Client<QsfpService>> getPaiDiagStreamingClient(
    folly::EventBase* evb,
    const folly::IPAddress& ip,
    uint32_t port) {
  folly::SocketAddress addr(ip, port);
  return std::make_unique<apache::thrift::Client<QsfpService>>(
      apache::thrift::RocketClientChannel::newChannel(
          folly::AsyncSocket::UniquePtr(new folly::AsyncSocket(evb, addr))));
}

} // namespace facebook::fboss
