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

#include <folly/io/async/AsyncSignalHandler.h>

#include <atomic>

namespace apache::thrift {
class ThriftServer;
}

namespace folly {
class FunctionScheduler;
}

namespace facebook::fboss {

class QsfpServiceHandler;

class QsfpServiceSignalHandler : public folly::AsyncSignalHandler {
 public:
  QsfpServiceSignalHandler(
      folly::EventBase* eventBase,
      folly::FunctionScheduler* functionScheduler,
      std::shared_ptr<apache::thrift::ThriftServer> qsfpServer,
      std::shared_ptr<QsfpServiceHandler> qsfpServiceHandler);

  QsfpServiceSignalHandler(QsfpServiceSignalHandler const&) = delete;
  QsfpServiceSignalHandler& operator=(QsfpServiceSignalHandler const&) = delete;

  void signalReceived(int signum) noexcept override;

  // True once the shutdown in signalReceived() completed. Lets main()
  // distinguish a signal-driven loop exit from any other cause.
  bool shutdownComplete() const {
    return shutdownComplete_.load();
  }

 private:
  folly::FunctionScheduler* functionScheduler_;
  std::shared_ptr<apache::thrift::ThriftServer> qsfpServer_;
  std::shared_ptr<QsfpServiceHandler> qsfpServiceHandler_;

  // Guards against a second signal (e.g. SIGINT after SIGTERM) re-entering
  // the shutdown sequence while it's already in progress.
  std::atomic<bool> exitSignalReceived_{false};
  std::atomic<bool> shutdownComplete_{false};
};
} // namespace facebook::fboss
