/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include "fboss/lib/link_snapshots/SnapshotManager.h"

namespace facebook::fboss {

using namespace std::chrono;

template <size_t length>
SnapshotManager<length>::SnapshotManager(std::set<std::string> portNames)
    : portNames_(portNames) {}

template <size_t length>
void SnapshotManager<length>::addSnapshot(LinkSnapshot val) {
  auto snapshot = SnapshotWrapper(val);
  buf_.write(snapshot);

  if (numSnapshotsToPublish_ > 0) {
    snapshot.publish(portNames_);
  }
  if (numSnapshotsToPublish_ > 0) {
    numSnapshotsToPublish_--;
  }
}

template <size_t length>
const RingBuffer<SnapshotWrapper, length>&
SnapshotManager<length>::getSnapshots() const {
  return buf_;
}

template <size_t length>
void SnapshotManager<length>::publishAllSnapshots() {
  for (auto& snapshot : buf_) {
    snapshot.publish(portNames_);
  }
}

template <size_t length>
void SnapshotManager<length>::publishFutureSnapshots(int numToPublish) {
  numSnapshotsToPublish_ = numToPublish;
}
} // namespace facebook::fboss
