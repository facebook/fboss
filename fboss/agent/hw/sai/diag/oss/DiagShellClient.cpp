/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/switch/gen-cpp2/SaiCtrlAsyncClient.h"
#include "folly/Memory.h"

#include <folly/SocketAddress.h>
#include <folly/init/Init.h>
#include <folly/io/async/AsyncSocket.h>
#include <folly/io/async/EventBase.h>
#include <folly/io/async/EventBaseManager.h>
#include <thrift/lib/cpp2/async/RocketClientChannel.h>
#include "fboss/agent/hw/sai/diag/DiagShellClient.h"

#include <sys/socket.h>
#include <sys/types.h>

namespace utility {
using folly::IPAddress;

std::unique_ptr<facebook::fboss::SaiCtrlAsyncClient>
getStreamingClient(folly::EventBase* evb, const IPAddress& ip, uint32_t port) {
  folly::SocketAddress addr(ip, port);
  return std::make_unique<facebook::fboss::SaiCtrlAsyncClient>(
      apache::thrift::RocketClientChannel::newChannel(
          folly::AsyncSocket::UniquePtr(new folly::AsyncSocket(evb, addr))));
}
} // namespace utility
