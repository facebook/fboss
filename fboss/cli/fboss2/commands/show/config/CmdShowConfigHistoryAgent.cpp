/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/show/config/CmdShowConfigHistoryAgent.h"
#include "fboss/cli/fboss2/CmdHandler.cpp"

#include "fboss/cli/fboss2/commands/show/config/CmdShowConfigUtils.h"

namespace facebook::fboss {

CmdShowConfigHistoryAgent::RetType CmdShowConfigHistoryAgent::queryClient(
    const HostInfo& hostInfo) {
  RetType retVal;

  retVal = getConfigHistory(hostInfo, "agent");
  return retVal;
}

void CmdShowConfigHistoryAgent::printOutput(
    RetType& agentConfigVersionHistory,
    std::ostream& out) {
  out << agentConfigVersionHistory << std::endl;
}

// Explicit template instantiation
template void CmdHandler<CmdShowConfigHistoryAgent, CmdShowConfigTraits>::run();

} // namespace facebook::fboss
