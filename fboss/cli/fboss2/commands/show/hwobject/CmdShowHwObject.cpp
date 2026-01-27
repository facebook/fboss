/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "CmdShowHwObject.h"

#include "fboss/cli/fboss2/utils/CmdClientUtils.h"

namespace facebook::fboss {

CmdShowHwObject::RetType CmdShowHwObject::queryClient(
    const HostInfo& hostInfo,
    const ObjectArgType& queriedHwObjectTypes) {
  std::string hwObjectInfo;

  if (utils::isMultiSwitchEnabled(hostInfo)) {
    auto hwAgentQueryFn =
        [&hwObjectInfo, queriedHwObjectTypes](
            apache::thrift::Client<facebook::fboss::FbossHwCtrl>& client) {
          std::string hwAgentObjectInfo;
          // TODO - we look at non cached objects. Add a cli option to
          // look at cached objects if so desired.
          client.sync_listHwObjects(
              hwAgentObjectInfo, queriedHwObjectTypes.data(), false);
          hwObjectInfo += hwAgentObjectInfo;
        };
    utils::runOnAllHwAgents(hostInfo, hwAgentQueryFn);
  } else {
    auto client =
        utils::createClient<apache::thrift::Client<FbossCtrl>>(hostInfo);
    client->sync_listHwObjects(hwObjectInfo, queriedHwObjectTypes.data(), true);
  }

  return hwObjectInfo;
}

void CmdShowHwObject::printOutput(
    const RetType& hwObjectInfo,
    std::ostream& out) {
  out << hwObjectInfo << std::endl;
}

} // namespace facebook::fboss
