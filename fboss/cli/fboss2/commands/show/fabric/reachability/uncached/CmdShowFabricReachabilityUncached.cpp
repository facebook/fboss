/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/show/fabric/reachability/uncached/CmdShowFabricReachabilityUncached.h"
#include "fboss/cli/fboss2/CmdHandler.cpp"
#include "fboss/cli/fboss2/utils/CmdUtils.h"
namespace facebook::fboss {

CmdShowFabricReachabilityUncached::RetType
CmdShowFabricReachabilityUncached::queryClient(
    const HostInfo& hostInfo,
    const std::vector<std::string>& queriedSwitchNames) {
  if (queriedSwitchNames.empty()) {
    throw std::runtime_error(
        "Switch name(s) required to get reachability information from hardware.");
  }

  std::vector<std::string> switchNames;
  switchNames.reserve(queriedSwitchNames.size());
  for (const auto& queriedSwitchName : queriedSwitchNames) {
    switchNames.push_back(utils::removeFbDomains(queriedSwitchName));
  }

  // FbossHwCtrl thrift endpoint is available whether or not multi_switch
  // feature is enabled. Leverage it.
  // Collecting such information from HwAgent is more efficient, and thus
  // preferred as SwSwitch call would just be a passthrough.
  auto reachabilityMatrix =
      utils::getUncachedSwitchReachabilityInfo(hostInfo, switchNames);

  return createModel(reachabilityMatrix);
}

CmdShowFabricReachabilityUncached::RetType
CmdShowFabricReachabilityUncached::createModel(
    std::unordered_map<std::string, std::vector<std::string>>&
        reachabilityMatrix) {
  return CmdShowFabricReachability::createModel(reachabilityMatrix);
}

void CmdShowFabricReachabilityUncached::printOutput(
    const RetType& model,
    std::ostream& out) {
  CmdShowFabricReachability::printOutput(model, out);
}

std::string_view CmdShowFabricReachabilityUncachedTraits::description() {
  return "Displays, per remote switch, how many and which local fabric ports can reach it, read directly from hardware instead of the agent's cached reachability data. Use it when the cached reachability information is suspected to be stale. DSF-only — applies to DSF switches and returns data only in a DSF topology.";
}

CmdShowFabricReachabilityUncached::RetType
CmdShowFabricReachabilityUncached::sampleModel() {
  RetType model;

  cli::ReachabilityEntry entry;
  entry.switchName() = "rdsw015";
  entry.reachablePorts() = {
      "fab1/33/1",
      "fab1/33/2",
      "fab1/33/3",
      "fab1/33/4",
      "fab1/34/1",
      "fab1/34/2",
      "fab1/34/3",
      "fab1/34/4",
  };
  model.reachabilityEntries()->push_back(entry);

  return model;
}

// Explicit template instantiation
template void CmdHandler<
    CmdShowFabricReachabilityUncached,
    CmdShowFabricReachabilityUncachedTraits>::run();
template const ValidFilterMapType CmdHandler<
    CmdShowFabricReachabilityUncached,
    CmdShowFabricReachabilityUncachedTraits>::getValidFilters();
} // namespace facebook::fboss
