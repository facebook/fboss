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

enum InputBalanceDestType {
  SINGLE_STAGE_FDSW_INTRA,
  DUAL_STAGE_SDSW_INTER,
  DUAL_STAGE_FDSW_INTRA,
  DUAL_STAGE_FDSW_INTER,
};

struct InputBalanceResult {
  std::string destinationSwitch;
  std::vector<std::string> sourceSwitch;
  int virtualDeviceID{};
  bool balanced;
  // Detailed information will only be populated if not balanced, or verbose
  // option enabled (for fboss2)
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

std::vector<InputBalanceResult> checkInputBalanceSingleStage(
    const std::vector<std::string>& dstSwitchNames,
    const std::unordered_map<
        std::string,
        std::unordered_map<std::string, std::vector<std::string>>>&
        inputCapacity,
    const std::unordered_map<std::string, std::vector<std::string>>&
        outputCapacity,
    const std::unordered_map<std::string, std::vector<std::string>>&
        neighborToLinkFailure,
    const std::unordered_map<std::string, int>& portToVirtualDevice,
    bool verbose = false);

std::vector<InputBalanceResult> checkInputBalanceDualStage(
    const InputBalanceDestType& inputBalanceDestType,
    const std::vector<std::string>& dstSwitchNames,
    const std::unordered_map<
        std::string,
        std::unordered_map<std::string, std::vector<std::string>>>&
        inputCapacity,
    const std::unordered_map<std::string, std::vector<std::string>>&
        outputCapacity,
    const std::unordered_map<std::string, std::vector<std::string>>&
        neighborToLinkFailure,
    const std::unordered_map<std::string, int>& portToVirtualDevice,
    const std::unordered_map<std::string, cfg::DsfNode>& switchNameToDsfNode,
    bool verbose);

} // namespace utility
} // namespace facebook::fboss
