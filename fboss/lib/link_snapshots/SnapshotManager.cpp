/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/lib/link_snapshots/SnapshotManager.h"
#include <sstream>
#include "fboss/lib/AlertLogger.h"

using namespace std::chrono;

namespace facebook::fboss {

void SnapshotWrapper::publish(std::set<std::string> portNames) {
  auto serializedSnapshot =
      apache::thrift::SimpleJSONSerializer::serialize<std::string>(snapshot_);
  if (!published_) {
    std::stringstream log;
    log << LinkSnapshotAlert() << "Collected snapshot for ports ";
    for (const auto& port : portNames) {
      log << PortParam(port);
    }
    XLOG(INFO) << log.str() << " " << LinkSnapshotParam(serializedSnapshot);
    published_ = true;
  }
}

} // namespace facebook::fboss
