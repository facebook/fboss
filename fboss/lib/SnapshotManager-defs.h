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

#include "fboss/lib/SnapshotManager.h"

namespace facebook::fboss {

using namespace std::chrono;

template <size_t length>
SnapshotManager<length>::SnapshotManager() {
  lastPublished_ =
      steady_clock::now() - seconds(FLAGS_link_snapshot_publish_interval);
}

template <size_t length>
void SnapshotManager<length>::addSnapshot(LinkSnapshot val) {
  auto snapshot = SnapshotWrapper(val);
  buf_.write(snapshot);
  auto now = steady_clock::now();
  if (lastPublished_ + seconds(FLAGS_link_snapshot_publish_interval) <= now) {
    snapshot.publish();
    lastPublished_ = now;
  }
}

template <size_t length>
RingBuffer<SnapshotWrapper, length>& SnapshotManager<length>::getSnapshots() {
  return buf_;
}
} // namespace facebook::fboss
