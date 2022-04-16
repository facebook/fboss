// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h"
#include "fboss/fsdb/server/OperPathToPublisherRoot.h"

#include <unordered_map>

namespace facebook::fboss::fsdb {
struct FsdbOperTreeMetadata {
  FsdbOperTreeMetadata() {
    operMetadata.lastConfirmedAt() = 0;
  }
  OperMetadata operMetadata;
  uint64_t numOpenConnections{0};
};
class FsdbOperTreeMetadataTracker {
 public:
  FsdbOperTreeMetadataTracker() = default;
  ~FsdbOperTreeMetadataTracker() = default;
  using Path = std::vector<std::string>;
  using PathItr = Path::const_iterator;

  void registerPublisherRoot(const Path& path) {
    registerPublisherRoot(path.begin(), path.end());
  }
  void registerPublisherRoot(PathItr begin, PathItr end) {
    registerPublisherRoot(OperPathToPublisherRoot().publisherRoot(begin, end));
  }
  void registerPublisherRoot(const std::string& publisher);
  void unregisterPublisherRoot(const Path& path) {
    unregisterPublisherRoot(path.begin(), path.end());
  }
  void unregisterPublisherRoot(PathItr begin, PathItr end) {
    unregisterPublisherRoot(
        OperPathToPublisherRoot().publisherRoot(begin, end));
  }
  void unregisterPublisherRoot(const std::string& publisher);

  void updateMetadata(
      const std::string& publisher,
      const OperMetadata& metadata,
      // Since multiple streams/threads could be updating
      // timestamps at different times
      // Its possible to have a update that came in earlier
      // endup updating metadata later. So as a aid to
      // that, help enforcing forward progress while holding
      // Metadata tracker update lock.
      bool enforceForwardProgress = true);
  void updateMetadata(
      const Path& path,
      const OperMetadata& metadata,
      bool enforceForwardProgress = true) {
    return updateMetadata(
        path.begin(), path.end(), metadata, enforceForwardProgress);
  }
  void updateMetadata(
      PathItr begin,
      PathItr end,
      const OperMetadata& metadata,
      bool enforceForwardProgress = true) {
    updateMetadata(
        OperPathToPublisherRoot().publisherRoot(begin, end),
        metadata,
        enforceForwardProgress);
  }

  using PublisherRoot2Metadata =
      std::unordered_map<std::string, FsdbOperTreeMetadata>;

  PublisherRoot2Metadata getAllMetadata() const;

  std::optional<FsdbOperTreeMetadata> getPublisherRootMetadata(
      const Path& path) const {
    return getPublisherRootMetadata(path.begin(), path.end());
  }
  std::optional<FsdbOperTreeMetadata> getPublisherRootMetadata(
      PathItr begin,
      PathItr end) const {
    return getPublisherRootMetadata(
        OperPathToPublisherRoot().publisherRoot(begin, end));
  }
  std::optional<FsdbOperTreeMetadata> getPublisherRootMetadata(
      const std::string& root) const;

 private:
  PublisherRoot2Metadata publisherRoot2Metadata_;
};
} // namespace facebook::fboss::fsdb
