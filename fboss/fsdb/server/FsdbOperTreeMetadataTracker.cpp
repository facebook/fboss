// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/fsdb/server/FsdbOperTreeMetadataTracker.h"

#include <folly/logging/xlog.h>

namespace facebook::fboss::fsdb {

void FsdbOperTreeMetadataTracker::registerPublisherRoot(
    const std::string& publisherRoot) {
  auto& pubMetadata = publisherRoot2Metadata_[publisherRoot];
  ++pubMetadata.numOpenConnections;
  XLOG(DBG2) << " Publisher: " << publisherRoot
             << " open connections  : " << pubMetadata.numOpenConnections;
}
void FsdbOperTreeMetadataTracker::unregisterPublisherRoot(
    const std::string& publisherRoot) {
  auto itr = publisherRoot2Metadata_.find(publisherRoot);
  if (itr == publisherRoot2Metadata_.end()) {
    return;
  }
  CHECK_GT(itr->second.numOpenConnections, 0);
  --itr->second.numOpenConnections;
  if (itr->second.numOpenConnections == 0) {
    XLOG(DBG2) << " All open connections gone, removing : " << publisherRoot;
    publisherRoot2Metadata_.erase(itr);
  }
}

void FsdbOperTreeMetadataTracker::updateMetadata(
    const std::string& publisherRoot,
    const OperMetadata& metadata,
    bool enforceForwardProgress) {
  auto itr = publisherRoot2Metadata_.find(publisherRoot);
  if (itr == publisherRoot2Metadata_.end()) {
    throw std::runtime_error(
        "Publisher root: " + publisherRoot +
        " must be registered before adding metadata");
  }
  auto& operMetadata = itr->second.operMetadata;
  if (enforceForwardProgress) {
    if (metadata.lastConfirmedAt()) {
      operMetadata.lastConfirmedAt() = std::max(
          *operMetadata.lastConfirmedAt(), *metadata.lastConfirmedAt());
    }
  } else {
    operMetadata.lastConfirmedAt() =
        metadata.lastConfirmedAt().value_or(*operMetadata.lastConfirmedAt());
  }
}

FsdbOperTreeMetadataTracker::PublisherRoot2Metadata
FsdbOperTreeMetadataTracker::getAllMetadata() const {
  return publisherRoot2Metadata_;
}

std::optional<FsdbOperTreeMetadata>
FsdbOperTreeMetadataTracker::getPublisherRootMetadata(
    const std::string& root) const {
  auto itr = publisherRoot2Metadata_.find(root);
  return itr == publisherRoot2Metadata_.end()
      ? std::nullopt
      : std::optional<FsdbOperTreeMetadata>(itr->second);
}
} // namespace facebook::fboss::fsdb
