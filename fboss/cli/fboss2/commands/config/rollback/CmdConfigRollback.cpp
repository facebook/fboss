/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/rollback/CmdConfigRollback.h"
#include "fboss/cli/fboss2/session/ConfigSession.h"

namespace facebook::fboss {

CmdConfigRollbackTraits::RetType CmdConfigRollback::queryClient(
    const HostInfo& hostInfo,
    const utils::RevisionList& revisions) {
  auto& session = ConfigSession::getInstance();

  // Validate arguments
  if (revisions.size() > 1) {
    throw std::invalid_argument(
        "Too many arguments. Expected 0 or 1 revision specifier.");
  }

  if (!revisions.empty() && revisions[0] == "current") {
    throw std::invalid_argument(
        "Cannot rollback to 'current'. Please specify a revision number like 'r42'.");
  }

  try {
    int newRevision;
    if (revisions.empty()) {
      // No revision specified - rollback to previous revision
      newRevision = session.rollback(hostInfo);
    } else {
      // Specific revision specified
      newRevision = session.rollback(hostInfo, revisions[0]);
    }
    if (newRevision) {
      return "Successfully rolled back to r" + std::to_string(newRevision) +
          " and config reloaded.";
    } else {
      return "Failed to create a new revision after rollback.";
    }
  } catch (const std::exception& ex) {
    throw std::runtime_error(
        "Failed to rollback config: " + std::string(ex.what()));
  }
}

void CmdConfigRollback::printOutput(const RetType& logMsg) {
  std::cout << logMsg << std::endl;
}

} // namespace facebook::fboss
