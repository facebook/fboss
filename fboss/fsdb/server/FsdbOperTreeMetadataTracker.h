// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/fsdb/if/gen-cpp2/fsdb_common_types.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h"

#include <folly/Synchronized.h>
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

  /* We use the conventions of taking the first element of
   * path and considering it as publisher root. This matches
   * the structure of our FSDB tree. However in the
   * future, if we ever change this, we could create a
   * mapping from (sub) path to a publisher root. So
   * for e.g. {"unit0","agent"} may map to publish root
   * of unit0_agent
   */
  void registerPublisherRoot(const Path& path) {
    registerPublisherRoot(path.begin(), path.end());
  }
  void registerPublisherRoot(PathItr begin, PathItr end) {
    checkPath(begin, end);
    registerPublisherImpl(*begin);
  }
  void unregisterPublisherRoot(const Path& path) {
    unregisterPublisherRoot(path.begin(), path.end());
  }
  void unregisterPublisherRoot(PathItr begin, PathItr end) {
    checkPath(begin, end);
    unregisterPublisherImpl(*begin);
  }
  void updateMetadata(
      const Path& path,
      const OperMetadata& metadata,
      // Since multiple streams/threads could be updating
      // timestamps, at different times
      // Its possible to have a update that came in earlier
      // endup updating metadata later. So as a aid to
      // that, help enforcing forward progress while holding
      // Metadata tracker update lock.
      bool enforceForwardProgress = true) {
    return updateMetadata(
        path.begin(), path.end(), metadata, enforceForwardProgress);
  }
  void updateMetadata(
      PathItr begin,
      PathItr end,
      const OperMetadata& metadata,
      // Since multiple streams/threads could be updating
      // timestamps at different times
      // Its possible to have a update that came in earlier
      // endup updating metadata later. So as a aid to
      // that, help enforcing forward progress while holding
      // Metadata tracker update lock.
      bool enforceForwardProgress = true) {
    checkPath(begin, end);
    updateMetadataImpl(*begin, metadata, enforceForwardProgress);
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
    checkPath(begin, end);
    return getPublisherRootMetadataImpl(*begin);
  }
  std::optional<FsdbOperTreeMetadata> getPublisherRootMetadata(
      const std::string& root) const {
    return getPublisherRootMetadataImpl(root);
  }

 private:
  std::optional<FsdbOperTreeMetadata> getPublisherRootMetadataImpl(
      const std::string& root) const;

  void checkPath(PathItr begin, PathItr end) const {
    if (begin == end) {
      FsdbException e;
      e.message_ref() = "Empty path";
      e.errorCode_ref() = FsdbErrorCode::INVALID_PATH;
      throw e;
    }
  }
  void registerPublisherImpl(const std::string& publisher);
  void unregisterPublisherImpl(const std::string& publisher);
  void updateMetadataImpl(
      const std::string& publisher,
      const OperMetadata& metadata,
      bool enforceForwardProgress);

  PublisherRoot2Metadata publisherRoot2Metadata_;
};
} // namespace facebook::fboss::fsdb
