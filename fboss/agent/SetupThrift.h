/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once
#include <chrono>
#include <memory>

#include <folly/SocketAddress.h>
#include <folly/io/async/EventBase.h>
#include <gflags/gflags.h>
#include <thrift/lib/cpp2/async/PooledRequestChannel.h>
#include <thrift/lib/cpp2/async/RocketClientChannel.h>
#include <thrift/lib/cpp2/server/ThriftServer.h>

DECLARE_int32(thrift_idle_timeout);
DECLARE_int32(thrift_task_expire_timeout);

DECLARE_bool(thrift_test_utils_thrift_handler);

namespace facebook::fboss {

void serverSSLSetup(apache::thrift::ThriftServer& server);

void setupThriftModules();
void clearThriftModules();

std::unique_ptr<apache::thrift::ThriftServer> setupThriftServer(
    folly::EventBase& eventBase,
    const std::vector<std::shared_ptr<apache::thrift::AsyncProcessorFactory>>&
        handlers,
    const std::vector<int>& ports,
    bool setupSSL);

template <typename THRIFT_HANDLER>
std::unique_ptr<apache::thrift::ThriftServer> setupThriftServer(
    folly::EventBase& eventBase,
    std::shared_ptr<THRIFT_HANDLER>& handler,
    std::vector<int> ports,
    bool setupSSL) {
  std::vector<std::shared_ptr<apache::thrift::AsyncProcessorFactory>>
      handlers{};
  handlers.push_back(handler);
  return setupThriftServer(eventBase, handlers, ports, setupSSL);
}

template <typename clientT>
std::unique_ptr<clientT> setupClient(
    std::string clientName,
    std::shared_ptr<folly::ScopedEventBaseThread>& evbThread,
    const std::string& ip = "::1",
    uint32_t port = 5909) {
  evbThread = std::make_shared<folly::ScopedEventBaseThread>(clientName);
  auto channel = apache::thrift::PooledRequestChannel::newChannel(
      evbThread->getEventBase(),
      evbThread,
      [ip, port](folly::EventBase& evb) mutable {
        return apache::thrift::RocketClientChannel::newChannel(
            folly::AsyncSocket::UniquePtr(
                new folly::AsyncSocket(&evb, ip, port)));
      });
  return std::make_unique<clientT>(std::move(channel));
}

} // namespace facebook::fboss
