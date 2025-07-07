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
#include "fboss/agent/AgentConfig.h"
#include "fboss/lib/ThriftMethodRateLimit.h"

#include <fb303/ExportType.h>
#include <fb303/ServiceData.h>
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

DEFINE_bool(
    thrift_rate_limit_shadow_mode,
    true,
    "Run thrift rate limit in shadow mode");

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

  std::map<std::string, double> method2QpsLimit = {};
  try {
    auto config = AgentConfig::fromDefaultFile();
    for (const auto& item : *config->thrift.thriftApiToRateLimitInQps()) {
      XLOG(DBG2) << "set rate limit " << item.second << " qps to "
                 << "thrift method " << item.first;
      method2QpsLimit[item.first] = item.second;
    }
  } catch (const std::exception&) {
    XLOG(ERR) << "cannot load thrift rate limit settings from agent config";
  }
  if (!method2QpsLimit.empty()) {
    auto odsCounterUpdateFunc = [](const std::string& method,
                                   uint64_t count,
                                   uint64_t aggCount) {
      XLOG(DBG2) << "Thrift method " << method << " rate limited " << count
                 << " times" << ", total number of thrift rate limit deny "
                 << aggCount;
      // Update ODS counter for rate-limited thrift methods
      facebook::fb303::fbData->addStatValue(
          "thrift.method." + method + ".rate_limited", 1, facebook::fb303::SUM);
      facebook::fb303::fbData->addStatValue(
          "thrift.method.aggregate.rate_limited", 1, facebook::fb303::SUM);
    };
    auto rateLimiter = std::make_shared<ThriftMethodRateLimit>(
        method2QpsLimit,
        FLAGS_thrift_rate_limit_shadow_mode,
        odsCounterUpdateFunc);
    auto preprocessFunc =
        ThriftMethodRateLimit::getThriftMethodRateLimitPreprocessFunc(
            rateLimiter);
    server->addPreprocessFunc(
        "ThriftMethodRateLimit", std::move(preprocessFunc));
  }
  return server;
}
} // namespace facebook::fboss
