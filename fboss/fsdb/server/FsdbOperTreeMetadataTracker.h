// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h"

#include <unordered_map>

namespace facebook::fboss::fsdb {
struct FsdbOperTreeMetadata {
  FsdbOperTreeMetadata() {
    operMetadata.lastConfirmedAt() = 0;
    operMetadata.lastPublishedAt() = 0;
  }
  OperMetadata operMetadata;
  uint64_t numOpenConnections{0};
  uint64_t lastPublishedUpdateProcessedAt{0};
};
class FsdbOperTreeMetadataTracker {
 public:
  FsdbOperTreeMetadataTracker() = default;
  ~FsdbOperTreeMetadataTracker() = default;

  void registerPublisherRoot(const std::string& publisherRoot);
  void unregisterPublisherRoot(const std::string& publisherRoot);

  void updateMetadata(
      const std::string& publisherRoot,
      const OperMetadata& metadata,
      // Since multiple streams/threads could be updating
      // timestamps at different times
      // Its possible to have a update that came in earlier
      // endup updating metadata later. So as a aid to
      // that, help enforcing forward progress while holding
      // Metadata tracker update lock.
      bool enforceForwardProgress = true);
  using PublisherRoot2Metadata =
      std::unordered_map<std::string, FsdbOperTreeMetadata>;

  PublisherRoot2Metadata getAllMetadata() const;

  std::optional<FsdbOperTreeMetadata> getPublisherRootMetadata(
      const std::string& root) const;

 private:
  PublisherRoot2Metadata publisherRoot2Metadata_;
};
} // namespace facebook::fboss::fsdb
