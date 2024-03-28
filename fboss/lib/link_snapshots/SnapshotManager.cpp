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

DEFINE_bool(
    enable_snapshot_debugs,
    true,
    "Print link snapshot debugs on console");

namespace {
// Default max rsyslog line length is 12k (currently qsfp_service allows up to
// 16k, but we shouldn't actually have lines that long anymore)
// Set it a bit lower to account for extra data from logging library
// (e.g. date+timestamp)
constexpr auto kMaxLogLineLength = 12000;
} // namespace

namespace facebook::fboss {

void SnapshotWrapper::publish(const std::set<std::string>& portNames) {
  if (!FLAGS_enable_snapshot_debugs) {
    return;
  }
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
