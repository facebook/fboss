// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/fsdb/if/gen-cpp2/fsdb_common_types.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h"

#include <folly/Synchronized.h>

namespace facebook::fboss::fsdb {
struct FsdbPublisherMetadata {
  OperMetadata operMetadata;
  uint64_t numOpenConnections{0};
};
class FsdbPublisherMetadataTracker {
 public:
  FsdbPublisherMetadataTracker() = default;
  ~FsdbPublisherMetadataTracker() = default;
  void registerPublisher(PublisherId publisher);
  void unregisterPublisher(PublisherId publisher);

 private:
  FsdbPublisherMetadataTracker(const FsdbPublisherMetadataTracker&) = delete;
  FsdbPublisherMetadataTracker& operator=(const FsdbPublisherMetadataTracker&) =
      delete;

  folly::Synchronized<std::map<PublisherId, FsdbPublisherMetadata>>
      publisherId2Metadata_;
};
} // namespace facebook::fboss::fsdb
