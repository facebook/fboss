// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/fsdb/client/FsdbStreamClient.h"

#ifdef IS_OSS_FBOSS_CENTOS9
#include <cstdint>
#include <optional>
#include "fboss/fsdb/client/Client.h"
#endif

namespace facebook::fboss::fsdb {
#ifdef IS_OSS_FBOSS_CENTOS9
/**
 * Default value sourced from Cfgr "neteng/qosdb/cos_utility_maps"
 * dscpToClassOfServiceMap.ClassOfService.NC : 48
 */
const uint8_t kDscpForClassOfServiceNC = 48;

void FsdbStreamClient::resetClient() {
  CHECK(streamEvb_->getEventBase()->isInEventBaseThread());
  client_.reset();
}

std::optional<uint8_t> getTosForClientPriority(
    const std::optional<FsdbStreamClient::Priority> priority) {
  if (priority.has_value()) {
    switch (*priority) {
      case FsdbStreamClient::Priority::CRITICAL:
        return kDscpForClassOfServiceNC;
      case FsdbStreamClient::Priority::NORMAL:
        // no TC marking by default
        return std::nullopt;
    }
  }
  return std::nullopt;
}

bool shouldUseEncryptedClient(const FsdbStreamClient::ServerOptions& options) {
  // use encrypted connection for all clients except CRITICAL ones.
  return (options.priority != FsdbStreamClient::Priority::CRITICAL);
}

void FsdbStreamClient::createClient(const ServerOptions& options) {
  CHECK(streamEvb_->getEventBase()->isInEventBaseThread());
  resetClient();

  auto tos = getTosForClientPriority(options.priority);
  bool encryptedClient = shouldUseEncryptedClient(options);

  client_ = Client::getClient(
      options.dstAddr /* dstAddr */,
      options.srcAddr /* srcAddr */,
      tos,
      !encryptedClient,
      streamEvb_->getEventBase());
}

#else

void FsdbStreamClient::resetClient() {}

void FsdbStreamClient::createClient(const ServerOptions& /* options */) {}

#endif
} // namespace facebook::fboss::fsdb
