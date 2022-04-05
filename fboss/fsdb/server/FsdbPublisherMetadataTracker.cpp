// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/fsdb/server/FsdbPublisherMetadataTracker.h"

namespace facebook::fboss::fsdb {

void FsdbPublisherMetadataTracker::registerPublisher(PublisherId publisher) {
  publisherId2Metadata_.withWLock([&publisher](auto& pub2Metadata) {
    ++pub2Metadata[publisher].numOpenConnections;
  });
}
void FsdbPublisherMetadataTracker::unregisterPublisher(PublisherId publisher) {
  publisherId2Metadata_.withWLock([&publisher](auto& pub2Metadata) {
    auto itr = pub2Metadata.find(publisher);
    if (itr == pub2Metadata.end()) {
      return;
    }
    CHECK_GT(itr->second.numOpenConnections, 0);
    --itr->second.numOpenConnections;
    if (itr->second.numOpenConnections == 0) {
      pub2Metadata.erase(itr);
    }
  });
}
} // namespace facebook::fboss::fsdb
