/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/SetupThrift.h"

#include <folly/io/async/EventBase.h>
#include <gflags/gflags.h>
#include <thrift/lib/cpp2/async/MultiplexAsyncProcessor.h>
#include <thrift/lib/cpp2/server/ThriftServer.h>

DEFINE_int32(thrift_idle_timeout, 60, "Thrift idle timeout in seconds.");
// Programming 16K routes can take 20+ seconds
DEFINE_int32(
    thrift_task_expire_timeout,
    30,
    "Thrift task expire timeout in seconds.");

// TODO: remove after netcastle changes are in place
DEFINE_bool(
    thrift_test_utils_thrift_handler,
    false,
    "Enable thrift handler for HW tests");

DEFINE_bool(
    hw_agent_for_testing,
    false,
    "Whether to prepare hw agent for testing. This includes "
    "1) Enable thrift handler for HW tests, "
    "2) Consume config file created by sw agent with overrides.");

namespace {
// The worst performance of programming acceptable route scale is 28s across
// our platforms in production. Hence setting the queue timeout to 30s to give
// small headroom.
constexpr auto kThriftServerQueueTimeout = std::chrono::seconds(30);
} // namespace

namespace facebook::fboss {
std::unique_ptr<apache::thrift::ThriftServer> setupThriftServer(
    folly::EventBase& eventBase,
    const std::vector<std::shared_ptr<apache::thrift::AsyncProcessorFactory>>&
        handlers,
    const std::vector<int>& ports,
    bool setupSSL) {
  // Start the thrift server
  auto server = std::make_unique<apache::thrift::ThriftServer>();
  server->setTaskExpireTime(
      std::chrono::milliseconds(FLAGS_thrift_task_expire_timeout * 1000));
  server->getEventBaseManager()->setEventBase(&eventBase, false);

  auto handler =
      std::make_shared<apache::thrift::MultiplexAsyncProcessorFactory>(
          handlers);

  server->setInterface(handler);

  server->setQueueTimeout(kThriftServerQueueTimeout);
  server->setSocketQueueTimeout(kThriftServerQueueTimeout);

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
