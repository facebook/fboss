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

#include <stddef.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>
#include <chrono>
#include <list>
#include "fboss/lib/link_snapshots/RingBuffer-defs.h"
#include "fboss/lib/phy/gen-cpp2/phy_types.h"
#include "folly/logging/xlog.h"

// By default we'll use 1 minute as our buffer time for snapshots
constexpr auto kDefaultTimespanSeconds = 60;

namespace facebook::fboss {

using namespace fboss::phy;

class SnapshotWrapper {
 public:
  explicit SnapshotWrapper(LinkSnapshot snapshot) : snapshot_(snapshot) {}
  void publish(std::set<std::string> portNames);

  LinkSnapshot snapshot_;
  bool published_{false};
};

/*
 * intervalSeconds is the time between snapshot collections
 * timespanSeconds is the total cached time stored in the snapshotManager
 * We will store timespan//interval + 1 snapshots in memory
 *
 */
// TODO(ccpowers): We may want to move from std::array to std:vector so we
// can use FLAGS_refresh_interval/FLAGS_gearbox_stats_interval instead of
// needing to define separate constants for the intervals.
template <
    size_t intervalSeconds,
    size_t timespanSeconds = kDefaultTimespanSeconds>
class SnapshotManager {
 public:
  static constexpr size_t length = timespanSeconds / intervalSeconds + 1;

  explicit SnapshotManager(std::set<std::string> portNames);
  void addSnapshot(LinkSnapshot val);
  void publishAllSnapshots();
  const RingBuffer<SnapshotWrapper, length>& getSnapshots() const;
  void publishFutureSnapshots(int numToPublish);
  void publishFutureSnapshots() {
    publishFutureSnapshots(length);
  }

 private:
  void publishIfNecessary();

  RingBuffer<SnapshotWrapper, length> buf_;
  int numSnapshotsToPublish_{0};
  std::set<std::string> portNames_;
};

} // namespace facebook::fboss
