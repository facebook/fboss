/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/show/fabric/reachability/CmdShowFabricReachability.h"

#include "fboss/cli/fboss2/utils/CmdUtils.h"
#include "fboss/cli/fboss2/utils/Table.h"

namespace facebook::fboss {

CmdShowFabricReachability::RetType CmdShowFabricReachability::queryClient(
    const HostInfo& hostInfo,
    const ObjectArgType& queriedSwitchNames) {
  std::vector<std::string> switchNames;
  if (queriedSwitchNames.size()) {
    switchNames.reserve(queriedSwitchNames.size());
    for (const auto& queriedSwitchName : queriedSwitchNames) {
      switchNames.push_back(utils::removeFbDomains(queriedSwitchName));
    }
  }

  auto reachabilityMatrix =
      utils::getCachedSwSwitchReachabilityInfo(hostInfo, switchNames);
  return createModel(reachabilityMatrix);
}

CmdShowFabricReachability::RetType CmdShowFabricReachability::createModel(
    std::unordered_map<std::string, std::vector<std::string>>&
        reachabilityMatrix) {
  RetType model;
  for (auto& [switchName, reachability] : reachabilityMatrix) {
    cli::ReachabilityEntry entry;
    std::sort(reachability.begin(), reachability.end(), utils::comparePortName);
    entry.switchName() = switchName;
    entry.reachablePorts() = reachability;
    model.reachabilityEntries()->push_back(entry);
  }
  return model;
}

void CmdShowFabricReachability::printOutput(
    const RetType& model,
    std::ostream& out) {
  utils::Table table;
  table.setHeader({
      "Switch Name",
      "Reachable via # Ports",
      "Reachable Via Ports",
  });
  for (const auto& entry : *model.reachabilityEntries()) {
    std::stringstream ss;
    std::copy(
        (*entry.reachablePorts()).begin(),
        (*entry.reachablePorts()).end(),
        std::ostream_iterator<std::string>(ss, " "));
    table.addRow(
        {*entry.switchName(),
         folly::to<std::string>(entry.reachablePorts()->size()),
         ss.str()});
  }
  out << table << std::endl;
}
} // namespace facebook::fboss
