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
#include "fboss/cli/fboss2/commands/show/fabric/reachability/CmdShowFabricReachability.h"

namespace facebook::fboss {

struct CmdShowFabricReachabilityUncachedTraits : public ReadCommandTraits {
  using ParentCmd = CmdShowFabricReachability;
  using RetType = cli::ShowFabricReachabilityModel;
};
class CmdShowFabricReachabilityUncached
    : public CmdHandler<
          CmdShowFabricReachabilityUncached,
          CmdShowFabricReachabilityUncachedTraits>,
      public CmdShowFabricReachabilityTraits {
 public:
  using RetType = CmdShowFabricReachabilityUncachedTraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const std::vector<std::string>& queriedSwitchNames) {
    if (queriedSwitchNames.empty()) {
      throw std::runtime_error(
          "Switch name(s) required to get reachability information from hardware.");
    }

    std::map<std::string, std::vector<std::string>> reachabilityMatrix;

    std::vector<std::string> switchNames;
    switchNames.reserve(queriedSwitchNames.size());
    for (const auto& queriedSwitchName : queriedSwitchNames) {
      switchNames.push_back(utils::removeFbDomains(queriedSwitchName));
    }

    // FbossHwCtrl thrift endpoint is available whether or not multi_switch
    // feature is enabled. Leverage it.
    // Collecting such information from HwAgent is more efficient, and thus
    // preferred as SwSwitch call would just be a passthrough.
    auto hwAgentQueryFn =
        [&reachabilityMatrix, &switchNames](
            apache::thrift::Client<facebook::fboss::FbossHwCtrl>& client) {
          std::map<std::string, std::vector<std::string>> reachability;
          client.sync_getHwSwitchReachability(reachability, switchNames);
          for (auto& [switchName, reachablePorts] : reachability) {
            reachabilityMatrix[switchName].insert(
                reachabilityMatrix[switchName].end(),
                reachablePorts.begin(),
                reachablePorts.end());
          }
        };

    utils::runOnAllHwAgents(hostInfo, hwAgentQueryFn);

    return createModel(reachabilityMatrix);
  }

  RetType createModel(
      std::map<std::string, std::vector<std::string>>& reachabilityMatrix) {
    return CmdShowFabricReachability::createModel(reachabilityMatrix);
  }

  void printOutput(const RetType& model, std::ostream& out = std::cout) {
    CmdShowFabricReachability::printOutput(model, out);
  }
};
} // namespace facebook::fboss
