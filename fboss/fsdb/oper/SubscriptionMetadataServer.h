// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

#include <string>
#include "fboss/fsdb/server/FsdbOperTreeMetadataTracker.h"

namespace facebook::fboss::fsdb {

class SubscriptionMetadataServer {
  using PublisherRoot2Metadata =
      FsdbOperTreeMetadataTracker::PublisherRoot2Metadata;

 public:
  explicit SubscriptionMetadataServer(
      std::optional<PublisherRoot2Metadata> metadata)
      : metadata_(std::move(metadata)) {}

  bool trackingMetadata() const {
    return metadata_ != std::nullopt;
  }
  bool ready(const std::string& publisherRoot) const;
  std::optional<FsdbOperTreeMetadata> getMetadata(
      const std::string& publisherRoot) const;

  const std::optional<PublisherRoot2Metadata> getAllPublishersMetadata() const {
    return metadata_;
  }

 private:
  const std::optional<PublisherRoot2Metadata> metadata_;
};
} // namespace facebook::fboss::fsdb
