// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <folly/io/async/EventBase.h>

#include "fboss/platform/platform_manager/gen-cpp2/PlatformManagerService.h"

namespace facebook::fboss::platform::sensor_service {
class PmClientFactory {
 public:
  std::unique_ptr<
      apache::thrift::Client<platform_manager::PlatformManagerService>>
  create() const;

 private:
  std::unique_ptr<
      apache::thrift::Client<platform_manager::PlatformManagerService>>
  createPlainTextClient() const;
  std::unique_ptr<
      apache::thrift::Client<platform_manager::PlatformManagerService>>
  createSecureClient(
      const std::pair<std::string, std::string>& certAndKey) const;
  std::unique_ptr<folly::EventBase> const evb_{
      std::make_unique<folly::EventBase>()};
  const folly::SocketAddress pmSocketAddr_{"::1", 5975};
};
} // namespace facebook::fboss::platform::sensor_service
