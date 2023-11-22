// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/fsdb/oper/SubscriptionMetadataServer.h"

namespace facebook::fboss::fsdb {

bool SubscriptionMetadataServer::ready(const std::string& publisherRoot) const {
  if (!trackingMetadata()) {
    // Not tracking metadata, always ready
    return true;
  }
  auto mitr = metadata_->find(publisherRoot);
  return mitr != metadata_->end() &&
      mitr->second.operMetadata.lastConfirmedAt().value_or(0) > 0;
}
std::optional<FsdbOperTreeMetadata> SubscriptionMetadataServer::getMetadata(
    const std::string& publisherRoot) const {
  if (!trackingMetadata()) {
    // Not tracking metadata
    return std::nullopt;
  }
  auto mitr = metadata_->find(publisherRoot);
  return mitr != metadata_->end()
      ? std::optional<FsdbOperTreeMetadata>(mitr->second)
      : std::nullopt;
}
} // namespace facebook::fboss::fsdb
