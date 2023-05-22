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
#include <thrift/lib/cpp2/server/ThriftServer.h>

DECLARE_int32(thrift_idle_timeout);
DECLARE_int32(thrift_task_expire_timeout);

namespace facebook::fboss {

void serverSSLSetup(apache::thrift::ThriftServer& server);

void setupThriftModules();

template <typename THRIFT_HANDLER>
std::unique_ptr<apache::thrift::ThriftServer> setupThriftServer(
    folly::EventBase& eventBase,
    std::shared_ptr<THRIFT_HANDLER>& handler,
    std::vector<int> ports,
    bool setupSSL) {
  // Start the thrift server
  auto server = std::make_unique<apache::thrift::ThriftServer>();
  server->setTaskExpireTime(
      std::chrono::milliseconds(FLAGS_thrift_task_expire_timeout * 1000));
  server->getEventBaseManager()->setEventBase(&eventBase, false);
  server->setInterface(handler);
  // Since thrift calls may involve programming HW, don't
  // set queue timeouts
  server->setQueueTimeout(std::chrono::milliseconds(0));
  server->setSocketQueueTimeout(std::chrono::milliseconds(0));

  if (setupSSL) {
    serverSSLSetup(*server);
  }

  setupThriftModules();

  std::vector<folly::SocketAddress> addresses;
  for (auto port : ports) {
    folly::SocketAddress address;
    address.setFromLocalPort(port);
    addresses.push_back(address);
  }
  server->setAddresses(addresses);
  server->setIdleTimeout(std::chrono::seconds(FLAGS_thrift_idle_timeout));
  return server;
}

} // namespace facebook::fboss
