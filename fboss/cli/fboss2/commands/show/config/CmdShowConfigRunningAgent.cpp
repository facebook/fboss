/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/show/config/CmdShowConfigRunningAgent.h"

#include <folly/json/json.h>
#include "fboss/agent/if/gen-cpp2/FbossCtrl.h"
#include "fboss/cli/fboss2/utils/CmdClientUtilsCommon.h"

namespace facebook::fboss {

CmdShowConfigRunningAgent::RetType CmdShowConfigRunningAgent::queryClient(
    const HostInfo& hostInfo) {
  RetType retVal;
  std::string configStr;

  auto client =
      utils::createClient<apache::thrift::Client<FbossCtrl>>(hostInfo);

  client->sync_getRunningConfig(configStr);

  retVal = folly::parseJson(configStr);
  return retVal;
}

void CmdShowConfigRunningAgent::printOutput(
    RetType& agentConfig,
    std::ostream& out) {
  out << folly::toPrettyJson(agentConfig) << std::endl;
}

} // namespace facebook::fboss
