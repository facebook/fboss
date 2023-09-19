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

namespace {
// Default max rsyslog line length is 8k (currently qsfp_service allows up to
// 16k, but we shouldn't actually have lines that long anymore)
// Set it a bit lower to account for extra data from logging library
// (e.g. date+timestamp)
constexpr auto kMaxLogLineLength = 8000;
} // namespace

namespace facebook::fboss {

void SnapshotWrapper::publish(const std::set<std::string>& portNames) {
  auto serializedSnapshot =
      apache::thrift::SimpleJSONSerializer::serialize<std::string>(snapshot_);
  if (!published_) {
    std::stringstream log;
    log << LinkSnapshotAlert() << "Collected snapshot for ports ";
    for (const auto& port : portNames) {
      log << PortParam(port);
    }
    log << " " << LinkSnapshotParam(serializedSnapshot);
    // Check that length isn't too long. Should only trigger in debug mode
    // (i.e. in link tests)
    DCHECK(log.str().size() < kMaxLogLineLength)
        << "CHECK failed, snapshot length was too long.";
    XLOG(DBG2) << log.str();
    published_ = true;
  }
}

} // namespace facebook::fboss
