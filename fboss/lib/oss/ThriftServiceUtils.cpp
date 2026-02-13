/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/lib/ThriftServiceUtils.h"

#include <folly/io/async/Epoll.h>
#include <folly/logging/xlog.h>

#if FOLLY_HAS_EPOLL
// Conditionally include EpollBackend only if folly has epoll support
#include <folly/executors/IOThreadPoolExecutor.h>
#include <folly/io/async/EpollBackend.h>
#include <folly/system/HardwareConcurrency.h>
#endif

namespace facebook::fboss {

namespace {
#if FOLLY_HAS_EPOLL
// Get EventBase::Options configured to use EpollBackend
folly::EventBase::Options getEpollEventBaseOptions() {
  return folly::EventBase::Options{}.setBackendFactory(
      []() -> std::unique_ptr<folly::EventBaseBackendBase> {
        return std::make_unique<folly::EpollBackend>(
            folly::EpollBackend::Options{});
      });
}

// EventBaseManager that creates EventBases with EpollBackend
folly::EventBaseManager& getEpollEventBaseManager() {
  static folly::EventBaseManager manager(getEpollEventBaseOptions());
  return manager;
}
#endif
} // namespace

void ThriftServiceUtils::setPreferredEventBaseBackend(
    apache::thrift::ThriftServer& server) {
  // T243914889: Set the preferred event base backend to epoll for OSS to avoid
  // the default libevent2 performance issue
  setPreferredEventBaseBackendToEpoll(server);
}

void ThriftServiceUtils::setPreferredEventBaseBackendToEpoll(
    apache::thrift::ThriftServer& server) {
#if FOLLY_HAS_EPOLL
  auto& epollManager = getEpollEventBaseManager();
  auto executor = std::make_shared<folly::IOThreadPoolExecutor>(
      folly::available_concurrency(),
      std::make_shared<folly::NamedThreadFactory>("ThriftIO"),
      &epollManager,
      folly::IOThreadPoolExecutor::Options().setEnableThreadIdCollection(
          THRIFT_FLAG(enable_io_queue_lag_detection)));
  server.setIOThreadPool(executor);
  XLOG(INFO) << "Set preferred event base backend to Epoll";
#else
  // Epoll should be well-established and widely supported since Linux 2.6
  // Just in case folly doesn't have epoll support, we should throw an error
  // to avoid the user unknowingly using the wrong backend
  throw std::runtime_error("Folly does not have epoll support");
#endif
}

folly::Function<void(apache::thrift::ThriftServer&)>
ThriftServiceUtils::createThriftServerConfig() {
  return [](apache::thrift::ThriftServer& server) {
    ThriftServiceUtils::setPreferredEventBaseBackend(server);
  };
}

} // namespace facebook::fboss
