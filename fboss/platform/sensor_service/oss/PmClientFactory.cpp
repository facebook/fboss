// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/sensor_service/PmClientFactory.h"

#include <folly/io/async/AsyncSSLSocket.h>
#include <folly/io/async/SSLOptions.h>
#include <thrift/lib/cpp2/async/RocketClientChannel.h>

namespace facebook::fboss::platform::sensor_service {
std::unique_ptr<
    apache::thrift::Client<platform_manager::PlatformManagerService>>
PmClientFactory::create() const {
  return createPlainTextClient();
}

std::unique_ptr<
    apache::thrift::Client<platform_manager::PlatformManagerService>>
PmClientFactory::createPlainTextClient() const {
  auto socket = folly::AsyncSocket::UniquePtr(
      new folly::AsyncSocket(evb_.get(), pmSocketAddr_));
  auto channel =
      apache::thrift::RocketClientChannel::newChannel(std::move(socket));
  return std::make_unique<
      apache::thrift::Client<platform_manager::PlatformManagerService>>(
      std::move(channel));
}

std::unique_ptr<
    apache::thrift::Client<platform_manager::PlatformManagerService>>
PmClientFactory::createSecureClient(
    const std::pair<std::string, std::string>& /*certAndkey*/) const {
  // Unused in OSS.
  return nullptr;
}

} // namespace facebook::fboss::platform::sensor_service
