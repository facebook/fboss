// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/fsdb/server/FsdbOperTreeMetadataTracker.h"

#include <folly/logging/xlog.h>

namespace facebook::fboss::fsdb {

void FsdbOperTreeMetadataTracker::registerPublisherImpl(
    const std::string& publisher) {
  publisherRoot2Metadata_.withWLock([&publisher](auto& pub2Metadata) {
    auto& pubMetadata = pub2Metadata[publisher];
    ++pubMetadata.numOpenConnections;
    XLOG(DBG2) << " Publisher: " << publisher
               << " open connections  : " << pubMetadata.numOpenConnections;
  });
}
void FsdbOperTreeMetadataTracker::unregisterPublisherImpl(
    const std::string& publisher) {
  publisherRoot2Metadata_.withWLock([&publisher](auto& pub2Metadata) {
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

void FsdbOperTreeMetadataTracker::updateMetadataImpl(
    const std::string& publisher,
    const OperMetadata& metadata,
    bool enforceForwardProgress) {
  publisherRoot2Metadata_.withWLock([&](auto& pub2Metadata) {
    auto itr = pub2Metadata.find(publisher);
    if (itr == pub2Metadata.end()) {
      throw std::runtime_error(
          "Publisher: " + publisher +
          " must be registered before adding metadata");
    }
    auto& operMetadata = itr->second.operMetadata;
    if (enforceForwardProgress) {
      if (metadata.generation()) {
        operMetadata.generation() =
            std::max(*operMetadata.generation(), *metadata.generation());
      }
      if (metadata.lastConfirmedAtSecsSinceEpoch()) {
        operMetadata.lastConfirmedAtSecsSinceEpoch() = std::max(
            *operMetadata.lastConfirmedAtSecsSinceEpoch(),
            *metadata.lastConfirmedAtSecsSinceEpoch());
      }
    } else {
      operMetadata.generation() =
          metadata.generation().value_or(*operMetadata.generation());
      operMetadata.lastConfirmedAtSecsSinceEpoch() =
          metadata.lastConfirmedAtSecsSinceEpoch().value_or(
              *operMetadata.lastConfirmedAtSecsSinceEpoch());
    }
  });
}

FsdbOperTreeMetadataTracker::PublisherRoot2Metadata
FsdbOperTreeMetadataTracker::getAllMetadata() const {
  auto pub2Metadata = publisherRoot2Metadata_.rlock();
  return *pub2Metadata;
}

std::optional<FsdbOperTreeMetadata>
FsdbOperTreeMetadataTracker::getPublisherRootMetadataImpl(
    const std::string& root) const {
  auto pub2Metadata = publisherRoot2Metadata_.rlock();
  auto itr = pub2Metadata->find(root);
  return itr == pub2Metadata->end()
      ? std::nullopt
      : std::optional<FsdbOperTreeMetadata>(itr->second);
}
} // namespace facebook::fboss::fsdb
