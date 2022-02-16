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

  void signalReceived(int signum) noexcept override;

 private:
  // Forbidden copy constructor and assignment operator
  QsfpServiceSignalHandler(QsfpServiceSignalHandler const&) = delete;
  QsfpServiceSignalHandler& operator=(QsfpServiceSignalHandler const&) = delete;

  folly::FunctionScheduler* functionScheduler_;
  std::shared_ptr<apache::thrift::ThriftServer> qsfpServer_;
  std::shared_ptr<QsfpServiceHandler> qsfpServiceHandler_;
};
} // namespace facebook::fboss
