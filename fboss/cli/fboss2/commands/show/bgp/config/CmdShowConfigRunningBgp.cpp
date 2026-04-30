/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/show/bgp/config/CmdShowConfigRunningBgp.h"

#include <folly/json/json.h>
#include "fboss/bgp/if/gen-cpp2/TBgpService.h"
#include "fboss/cli/fboss2/utils/CmdClientUtilsCommon.h"

namespace facebook::fboss {

CmdShowConfigRunningBgp::RetType CmdShowConfigRunningBgp::queryClient(
    const HostInfo& hostInfo) {
  RetType retVal;
  std::string configStr;

  auto client = utils::createClient<apache::thrift::Client<
      facebook::neteng::fboss::bgp::thrift::TBgpService>>(hostInfo);

  client->sync_getRunningConfig(configStr);

  retVal = folly::parseJson(configStr);
  return retVal;
}

void CmdShowConfigRunningBgp::printOutput(
    RetType& bgpConfig,
    std::ostream& out) {
  out << folly::toPrettyJson(bgpConfig) << std::endl;
}

} // namespace facebook::fboss
