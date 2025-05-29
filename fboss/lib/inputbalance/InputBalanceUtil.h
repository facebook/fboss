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
#include <vector>
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"

namespace facebook::fboss {

class MultiSwitchPortMap;

namespace utility {

struct InputBalanceResult {
  std::string destinationSwitch;
  std::vector<std::string> sourceSwitch;
  bool balanced;
  // Detailed information will only be populated if not balanced
  std::optional<std::vector<std::string>> inputCapacity;
  std::optional<std::vector<std::string>> outputCapacity;
  std::optional<std::vector<std::string>> inputLinkFailure;
  std::optional<std::vector<std::string>> outputLinkFailure;
};

bool isDualStage(const std::map<int64_t, cfg::DsfNode>& dsfNodeMap);

std::unordered_map<std::string, cfg::DsfNode> switchNameToDsfNode(
    const std::map<int64_t, cfg::DsfNode>& dsfNodes);

std::vector<std::pair<int64_t, std::string>> deviceToQueryInputCapacity(
    const std::vector<int64_t>& fabricSwitchIDs,
    const std::map<int64_t, cfg::DsfNode>& dsfNodeMap);

// map<neighborName, map<neighborPort, localPort>>
std::unordered_map<std::string, std::unordered_map<std::string, std::string>>
getNeighborFabricPortsToSelf(
    const std::map<int32_t, PortInfoThrift>& myPortInfo);

std::map<std::string, std::string> getPortToNeighbor(
    const std::shared_ptr<MultiSwitchPortMap>& portMap);

std::unordered_map<std::string, std::vector<std::string>>
getNeighborToLinkFailure(const std::map<int32_t, PortInfoThrift>& myPortInfo);

std::unordered_map<std::string, int> getPortToVirtualDeviceId(
    const std::map<int32_t, PortInfoThrift>& myPortInfo);

// TODO(zecheng): Add check to link failure
std::vector<InputBalanceResult> checkInputBalanceSingleStage(
    const std::vector<std::string>& dstSwitchNames,
    const std::unordered_map<
        std::string,
        std::unordered_map<std::string, std::vector<std::string>>>&
        inputCpacity,
    const std::unordered_map<std::string, std::vector<std::string>>&
        outputCpacity,
    const std::unordered_map<std::string, std::vector<std::string>>&
        neighborToLinkFailure,
    bool verbose = false);
} // namespace utility
} // namespace facebook::fboss
