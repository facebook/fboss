// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

#include <folly/testing/TestUtil.h>
#include <memory>

#include "common/init/Init.h"
#include "common/services/cpp/ServiceFrameworkLight.h"
#include "fboss/fsdb/server/ServiceHandler.h"
#include "thrift/lib/cpp2/server/ThriftServer.h"
#include "thrift/lib/cpp2/util/ScopedServerInterfaceThread.h"

namespace facebook::fboss::fsdb::test {
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
      uint32_t statsSubscriptionServe_ms = 1000);
  ~FsdbTestServer();

  uint16_t getFsdbPort() const {
    return fsdbPort_;
  }

  std::unique_ptr<apache::thrift::Client<FsdbService>> getClient() {
    return apache::thrift::makeTestClient(handler_);
  }

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
  std::string getPublisherId(int publisherIdx) const;
  const int kMaxPublishers_{3};
  std::unique_ptr<services::ServiceFrameworkLight> serviceFramework_;
  std::unique_ptr<std::thread> thriftThread_;
  std::shared_ptr<ServiceHandler> handler_;
  uint16_t fsdbPort_{};
};

} // namespace facebook::fboss::fsdb::test
