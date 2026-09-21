// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <memory>
#include <optional>

#include <folly/io/async/ScopedEventBaseThread.h>
#include "fboss/fsdb/server/ServiceHandler.h"

namespace facebook::fboss::fsdb::test {

class FsdbTestServerImpl {
 public:
  explicit FsdbTestServerImpl(
      std::shared_ptr<ServiceHandler> handler,
      uint16_t port,
      std::optional<size_t> numIOWorkerThreads = std::nullopt,
      std::optional<size_t> numCPUWorkerThreads = std::nullopt)
      : handler_(std::move(handler)),
        port_(port),
        numIOWorkerThreads_(numIOWorkerThreads),
        numCPUWorkerThreads_(numCPUWorkerThreads) {}
  virtual ~FsdbTestServerImpl() = default;

  virtual void startServer(uint16_t& fsdbPort) = 0;
  virtual void stopServer() = 0;

  // Returns a client whose socket lives on a dedicated IO thread.
  // The shared_ptr custom deleter ensures the client is destroyed on
  // the IO thread so that socket teardown runs on the correct EventBase.
  std::shared_ptr<apache::thrift::Client<FsdbService>> getClient();

 protected:
  std::shared_ptr<apache::thrift::ThriftServer> createServer(
      std::shared_ptr<ServiceHandler> handler,
      uint16_t port);

  void checkServerStart(uint16_t& fsdbPort);

  std::shared_ptr<ServiceHandler> handler_;
  std::shared_ptr<apache::thrift::ThriftServer> server_;
  uint16_t port_;
  // When unset, ThriftServer's defaults (which scale with CPU cores) apply.
  // Set explicitly by callers that spin up many parallel server instances
  // (e.g. HRT test rig: 4 devices x 8 planes = 32 servers).
  std::optional<size_t> numIOWorkerThreads_;
  std::optional<size_t> numCPUWorkerThreads_;
  std::shared_ptr<folly::ScopedEventBaseThread> clientEvbThread_{
      std::make_shared<folly::ScopedEventBaseThread>("FsdbTestClientIO")};
};

class FsdbTestServer {
 public:
  FsdbTestServer(
      uint16_t port = 0,
      uint32_t stateSubscriptionServe_ms = 50,
      uint32_t statsSubscriptionServe_ms = 1000)
      : FsdbTestServer(
            std::make_unique<FsdbConfig>(),
            port,
            stateSubscriptionServe_ms,
            statsSubscriptionServe_ms) {}
  explicit FsdbTestServer(
      std::shared_ptr<FsdbConfig> config,
      uint16_t port = 0,
      uint32_t stateSubscriptionServe_ms = 50,
      uint32_t statsSubscriptionServe_ms = 1000,
      uint32_t subscriptionServeQueueSize = 1000,
      uint32_t statsSubscriptionServeQueueSize = 8,
      std::optional<size_t> numIOWorkerThreads = std::nullopt,
      std::optional<size_t> numCPUWorkerThreads = std::nullopt);
  ~FsdbTestServer();

  uint16_t getFsdbPort() const {
    return fsdbPort_;
  }

  std::shared_ptr<apache::thrift::Client<FsdbService>> getClient();

  const ServiceHandler& serviceHandler() const {
    CHECK(handler_);
    return *handler_;
  }

  ServiceHandler& serviceHandler() {
    CHECK(handler_);
    return *handler_;
  }
  std::optional<FsdbOperTreeMetadata> getPublisherRootMetadata(
      const std::string& root,
      bool isStats) const;
  ServiceHandler::ActiveSubscriptions getActiveSubscriptions() const;

 private:
  void startTestServer(
      uint16_t port,
      std::optional<size_t> numIOWorkerThreads,
      std::optional<size_t> numCPUWorkerThreads);
  void stopTestServer();
  std::string getPublisherId(int publisherIdx) const;
  const int kMaxPublishers_{3};
  std::shared_ptr<FsdbConfig> config_;
  std::shared_ptr<ServiceHandler> handler_;
  uint16_t fsdbPort_{};
  std::unique_ptr<FsdbTestServerImpl> impl_;
};

} // namespace facebook::fboss::fsdb::test
