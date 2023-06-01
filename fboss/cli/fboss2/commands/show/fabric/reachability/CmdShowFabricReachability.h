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

#include <string>
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/fabric/reachability/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"

namespace facebook::fboss {

struct CmdShowFabricReachabilityTraits : public BaseCommandTraits {
  using ParentCmd = CmdShowFabric;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_SWITCH_NAME_LIST;
  using ObjectArgType = std::vector<std::string>;
  using RetType = cli::ShowFabricReachabilityModel;
};
class CmdShowFabricReachability : public CmdHandler<
                                      CmdShowFabricReachability,
                                      CmdShowFabricReachabilityTraits> {
 public:
  using ObjectArgType = CmdShowFabricReachabilityTraits::ObjectArgType;
  using RetType = CmdShowFabricReachabilityTraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const ObjectArgType& queriedSwitchNames) {
    std::map<std::string, std::vector<std::string>> reachabilityMatrix;

    if (queriedSwitchNames.empty()) {
      throw std::runtime_error(
          "Please provide the list of switch name(s) to query reachability.");
    }
    std::vector<std::string> switchNames;
    switchNames.reserve(queriedSwitchNames.size());
    for (const auto& queriedSwitchName : queriedSwitchNames) {
      switchNames.push_back(utils::removeFbDomains(queriedSwitchName));
    }

    auto client =
        utils::createClient<apache::thrift::Client<FbossCtrl>>(hostInfo);
    client->sync_getSwitchReachability(reachabilityMatrix, switchNames);
    return createModel(reachabilityMatrix);
  }

  RetType createModel(
      std::map<std::string, std::vector<std::string>>& reachabilityMatrix) {
    RetType model;
    for (auto& [switchName, reachability] : reachabilityMatrix) {
      cli::ReachabilityEntry entry;
      std::sort(
          reachability.begin(), reachability.end(), utils::comparePortName);
      entry.switchName() = switchName;
      entry.reachablePorts() = reachability;
      model.reachabilityEntries()->push_back(entry);
    }
    return model;
  }

  void printOutput(const RetType& model, std::ostream& out = std::cout) {
    Table table;
    table.setHeader({
        "Switch Name",
        "Reachable Via Ports",
    });
    for (const auto& entry : *model.reachabilityEntries()) {
      std::stringstream ss;
      std::copy(
          (*entry.reachablePorts()).begin(),
          (*entry.reachablePorts()).end(),
          std::ostream_iterator<std::string>(ss, " "));
      table.addRow({*entry.switchName(), ss.str()});
    }
    out << table << std::endl;
  }
};
} // namespace facebook::fboss
