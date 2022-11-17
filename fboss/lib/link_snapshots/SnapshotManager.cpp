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

void SnapshotWrapper::publish(const std::set<std::string>& portNames) {
  // S309875: Log length is too long now due to the tcvrStats and tcvrState
  // fields containing lots of duplicate data from other fields. For now lets
  // just clear these fields so that they aren't included in the log.
  // TODO(ccpowers): At some point we should de-duplicate the data at its source
  // rather than just wiping the duplicate data prior to logging it.
  auto patchedSnapshot = snapshot_;
  if (patchedSnapshot.transceiverInfo_ref()) {
    patchedSnapshot.transceiverInfo_ref()->tcvrStats_ref().reset();
    patchedSnapshot.transceiverInfo_ref()->tcvrState_ref().reset();
  }
  auto serializedSnapshot =
      apache::thrift::SimpleJSONSerializer::serialize<std::string>(
          patchedSnapshot);
  if (!published_) {
    std::stringstream log;
    log << LinkSnapshotAlert() << "Collected snapshot for ports ";
    for (const auto& port : portNames) {
      log << PortParam(port);
    }
    XLOG(DBG2) << log.str() << " " << LinkSnapshotParam(serializedSnapshot);
    published_ = true;
  }
}

} // namespace facebook::fboss
