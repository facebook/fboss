// Copyright 2004-present Facebook. All Rights Reserved.
//
#pragma once

#include <csignal>
#include <memory>

#include <folly/io/async/AsyncSignalHandler.h>

#include "thrift/lib/cpp2/async/AsyncProcessor.h"
#include "thrift/lib/cpp2/server/ThriftServer.h"

namespace facebook::fboss::platform::helpers {

void init(int* argc, char*** argv);

void runThriftService(
    std::shared_ptr<apache::thrift::ThriftServer> server,
    std::shared_ptr<apache::thrift::ServerInterface> handler,
    const std::string& serviceName,
    uint32_t port);

// SignalHandler provides graceful shutdown handling for SIGINT and SIGTERM
// signals. When either signal is received, it will call stop() on the
// associated ThriftServer to initiate graceful shutdown.
class SignalHandler : public folly::AsyncSignalHandler {
 public:
  SignalHandler(
      folly::EventBase* eventBase,
      std::shared_ptr<apache::thrift::ThriftServer> server)
      : AsyncSignalHandler(eventBase), server_(server) {
    registerSignalHandler(SIGINT);
    registerSignalHandler(SIGTERM);
  }

  void signalReceived(int signum) noexcept override {
    XLOG(INFO) << "Received signal " << signum
               << ", initiating graceful shutdown";
    server_->stop();
  }

 private:
  std::shared_ptr<apache::thrift::ThriftServer> server_;
};

} // namespace facebook::fboss::platform::helpers
