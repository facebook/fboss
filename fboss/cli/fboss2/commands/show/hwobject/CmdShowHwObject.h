/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once

#include "fboss/cli/fboss2/CmdGlobalOptions.h"
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"

namespace facebook::fboss {

struct CmdShowHwObjectTraits : public ReadCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_HW_OBJECT_LIST;
  using ObjectArgType = utils::HwObjectList;
  using RetType = std::string;
};

class CmdShowHwObject
    : public CmdHandler<CmdShowHwObject, CmdShowHwObjectTraits> {
 public:
  using ObjectArgType = CmdShowHwObjectTraits::ObjectArgType;
  using RetType = CmdShowHwObjectTraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const ObjectArgType& queriedHwObjectTypes) {
    std::string hwObjectInfo;

    if (utils::isFbossFeatureEnabled(hostInfo.getName(), "multi_switch")) {
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
          utils::createClient<facebook::fboss::FbossCtrlAsyncClient>(hostInfo);
      client->sync_listHwObjects(
          hwObjectInfo, queriedHwObjectTypes.data(), true);
    }

    return hwObjectInfo;
  }

  void printOutput(const RetType& hwObjectInfo, std::ostream& out = std::cout) {
    out << hwObjectInfo << std::endl;
  }
};

} // namespace facebook::fboss
