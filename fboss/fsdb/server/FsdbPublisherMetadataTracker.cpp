// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/fsdb/server/FsdbPublisherMetadataTracker.h"

#include <folly/logging/xlog.h>

namespace facebook::fboss::fsdb {

void FsdbPublisherMetadataTracker::registerPublisher(PublisherId publisher) {
  publisherId2Metadata_.withWLock([&publisher](auto& pub2Metadata) {
    auto& pubMetadata = pub2Metadata[publisher];
    ++pubMetadata.numOpenConnections;
    XLOG(DBG2) << " Publisher: " << publisher
               << " open connections  : " << pubMetadata.numOpenConnections;
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
      XLOG(DBG2) << " All open connections gone, removing : " << publisher;
      pub2Metadata.erase(itr);
    }
  });
}

FsdbPublisherMetadataTracker::PublisherId2Metadata
FsdbPublisherMetadataTracker::getAllMetadata() const {
  auto pub2Metadata = publisherId2Metadata_.rlock();
  return *pub2Metadata;
}
} // namespace facebook::fboss::fsdb
