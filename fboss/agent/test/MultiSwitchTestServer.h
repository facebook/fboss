// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

#include <folly/experimental/TestUtil.h>

#ifndef IS_OSS
#include "common/services/cpp/ServiceFrameworkLight.h"
#endif
#include "fboss/agent/MultiSwitchThriftHandler.h"
#include "fboss/agent/SwSwitch.h"
#include "thrift/lib/cpp2/server/ThriftServer.h"

namespace facebook::fboss {
class MultiSwitchTestServer {
 public:
  explicit MultiSwitchTestServer(
      std::vector<std::shared_ptr<apache::thrift::AsyncProcessorFactory>>&
          handlers,
      uint16_t port = 0);
  ~MultiSwitchTestServer();

  uint16_t getPort() const {
    return swSwitchPort_;
  }

 private:
#ifndef IS_OSS
  std::unique_ptr<services::ServiceFrameworkLight> serviceFramework_;
#endif
  std::unique_ptr<std::thread> thriftThread_;
  uint16_t swSwitchPort_{};
};
} // namespace facebook::fboss
