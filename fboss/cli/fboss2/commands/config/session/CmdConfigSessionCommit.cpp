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

#include "fboss/cli/fboss2/CmdHandler.cpp"

#include <fmt/format.h>
#include "fboss/cli/fboss2/session/ConfigSession.h"

namespace facebook::fboss {

CmdConfigSessionCommitTraits::RetType CmdConfigSessionCommit::queryClient(
    const HostInfo& hostInfo) {
  auto& session = ConfigSession::getInstance();

  if (!session.sessionExists()) {
    return "No config session exists. Make a config change first.";
  }

  auto result = session.commit(hostInfo);

  std::string message;
  std::string shortSha = result.commitSha.substr(0, 7);
  if (result.actionLevel == cli::ConfigActionLevel::AGENT_RESTART) {
    message = fmt::format(
        "Config session committed successfully as {} and wedge_agent restarted.",
        shortSha);
  } else {
    message = fmt::format(
        "Config session committed successfully as {} and config reloaded.",
        shortSha);
  }

  return message;
}

void CmdConfigSessionCommit::printOutput(const RetType& logMsg) {
  std::cout << logMsg << std::endl;
}

// Explicit template instantiation
template void
CmdHandler<CmdConfigSessionCommit, CmdConfigSessionCommitTraits>::run();

} // namespace facebook::fboss
