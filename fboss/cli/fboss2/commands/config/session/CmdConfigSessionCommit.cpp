/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/session/CmdConfigSessionCommit.h"
#include "fboss/cli/fboss2/session/ConfigSession.h"

namespace facebook::fboss {

CmdConfigSessionCommitTraits::RetType CmdConfigSessionCommit::queryClient(
    const HostInfo& hostInfo) {
  auto& session = ConfigSession::getInstance();

  if (!session.sessionExists()) {
    return "No config session exists. Make a config change first.";
  }

  int revision = session.commit(hostInfo);
  return "Config session committed successfully as r" +
      std::to_string(revision) + " and config reloaded.";
}

void CmdConfigSessionCommit::printOutput(const RetType& logMsg) {
  std::cout << logMsg << std::endl;
}

} // namespace facebook::fboss
