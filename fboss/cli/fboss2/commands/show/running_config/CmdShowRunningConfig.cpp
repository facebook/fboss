/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <folly/json/json.h>
#include <ostream>
#include <string>

#include "fboss/agent/if/gen-cpp2/FbossCtrlAsyncClient.h"
#include "fboss/cli/fboss2/utils/CmdClientUtilsCommon.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"

#include "CmdShowRunningConfig.h"

namespace facebook::fboss {

CmdShowRunningConfigTraits::RetType CmdShowRunningConfig::queryClient(
    const HostInfo& hostInfo) {
  auto client =
      utils::createClient<apache::thrift::Client<FbossCtrl>>(hostInfo);
  std::string configStr;
  client->sync_getRunningConfig(configStr);

  // Parse and pretty-print the JSON
  // The config is already a valid JSON string, we just need to format it
  return folly::toPrettyJson(folly::parseJson(configStr));
}

void CmdShowRunningConfig::printOutput(
    const RetType& config,
    std::ostream& out) {
  out << config << std::endl;
}

} // namespace facebook::fboss
