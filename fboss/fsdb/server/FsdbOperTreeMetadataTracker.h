// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/fsdb/if/gen-cpp2/fsdb_common_types.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h"

#include <folly/Synchronized.h>
#include <unordered_map>

namespace facebook::fboss::fsdb {
struct FsdbOperTreeMetadata {
  FsdbOperTreeMetadata() {
    operMetadata.generation() = 0;
    operMetadata.lastConfirmedAtSecsSinceEpoch() = 0;
  }
  OperMetadata operMetadata;
  uint64_t numOpenConnections{0};
};
class FsdbOperTreeMetadataTracker {
 public:
  FsdbOperTreeMetadataTracker() = default;
  ~FsdbOperTreeMetadataTracker() = default;
  void registerPublisher(PublisherId publisher);
  void unregisterPublisher(PublisherId publisher);
  void updateMetadata(
      const PublisherId& publisher,
      const OperMetadata& metadata,
      // Since multiple streams/threads could be updating
      // timestamps, generation numbers at different times
      // Its possible to have a update that came in earlier
      // endup updating metadata later. So as a aid to
      // that, help enforcing forward progress while holding
      // Metadata tracker update lock.
      bool enforceForwardProgress = true);

  using PublisherId2Metadata =
      std::unordered_map<PublisherId, FsdbOperTreeMetadata>;

  PublisherId2Metadata getAllMetadata() const;

 private:
  FsdbOperTreeMetadataTracker(const FsdbOperTreeMetadataTracker&) = delete;
  FsdbOperTreeMetadataTracker& operator=(const FsdbOperTreeMetadataTracker&) =
      delete;

  folly::Synchronized<PublisherId2Metadata> publisherId2Metadata_;
};
} // namespace facebook::fboss::fsdb
