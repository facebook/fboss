/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/lib/SnapshotManager.h"
#include "fboss/lib/AlertLogger.h"

using namespace std::chrono;

DEFINE_int32(
    link_snapshot_publish_interval,
    duration_cast<seconds>(std::chrono::hours(1)).count(),
    "Time interval between link snapshots (in seconds)");

namespace facebook::fboss {

void SnapshotWrapper::publish() {
  auto serializedSnapshot =
      apache::thrift::SimpleJSONSerializer::serialize<std::string>(snapshot_);
  if (!published_) {
    XLOG(INFO) << LinkSnapshotAlert() << "Collected snapshot "
               << LinkSnapshotParam(serializedSnapshot);
    published_ = true;
  }
}

} // namespace facebook::fboss
