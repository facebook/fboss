/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/lib/inputbalance/InputBalanceUtil.h"
#include "fboss/agent/state/PortMap.h"

#include <algorithm>

namespace facebook::fboss::utility {

bool isDualStage(const std::map<int64_t, cfg::DsfNode>& dsfNodeMap) {
  for (const auto& [_, dsfNode] : dsfNodeMap) {
    if (dsfNode.fabricLevel().has_value() && dsfNode.fabricLevel() == 2) {
      return true;
    }
  }
  return false;
}

std::unordered_map<std::string, cfg::DsfNode> switchNameToDsfNode(
    const std::map<int64_t, cfg::DsfNode>& dsfNodes) {
  std::unordered_map<std::string, cfg::DsfNode> nameToDsfNode;
  for (const auto& [_, dsfNode] : dsfNodes) {
    nameToDsfNode[dsfNode.name().value()] = dsfNode;
  }
  return nameToDsfNode;
}

std::vector<std::pair<int64_t, std::string>> deviceToQueryInputCapacity(
    const std::vector<int64_t>& fabricSwitchIDs,
    const std::map<int64_t, cfg::DsfNode>& dsfNodeMap) {
  std::vector<std::pair<int64_t, std::string>> switchIDAndName2Query;
  if (fabricSwitchIDs.empty()) {
    throw std::runtime_error("No fabric switch ID provided.");
  }
  bool dualStage = isDualStage(dsfNodeMap);

  // Query all local RDSW for single stage
  if (!dualStage) {
    auto clusterID = dsfNodeMap.at(*fabricSwitchIDs.cbegin()).clusterId();
    CHECK(clusterID.has_value());
    if (!std::all_of(
            fabricSwitchIDs.cbegin(),
            fabricSwitchIDs.cend(),
            [&](int64_t switchID) {
              return dsfNodeMap.at(switchID).clusterId() == *clusterID;
            })) {
      throw std::runtime_error(
          "Expected all fabric switches to have same cluster ID");
    }

    for (const auto& [switchId, dsfNode] : dsfNodeMap) {
      if (dsfNode.type() == cfg::DsfNodeType::INTERFACE_NODE) {
        if (!dsfNode.clusterId() || dsfNode.clusterId().value() != *clusterID) {
          throw std::runtime_error(
              "Non-matching cluster ID for FDSW and RDSW in single stage.");
        }
        switchIDAndName2Query.emplace_back(switchId, *dsfNode.name());
      }
    }
  }
  // TODO(zecheng): Implement functionality for dual stage
  return switchIDAndName2Query;
}

std::unordered_map<std::string, std::unordered_map<std::string, std::string>>
getNeighborFabricPortsToSelf(
    const std::map<int32_t, PortInfoThrift>& myPortInfo) {
  std::unordered_map<std::string, std::unordered_map<std::string, std::string>>
      switchNameToPorts;
  std::vector<std::string> noExpectedNeighbor;
  for (const auto& [_, portInfo] : myPortInfo) {
    if (portInfo.portType() == cfg::PortType::FABRIC_PORT) {
      if (portInfo.expectedNeighborReachability()->size() != 1) {
        noExpectedNeighbor.push_back(*portInfo.name());
        continue;
      }

      const auto& neighborName =
          *portInfo.expectedNeighborReachability()->at(0).remoteSystem();
      const auto& neighborPortName =
          *portInfo.expectedNeighborReachability()->at(0).remotePort();
      auto iter = switchNameToPorts.find(neighborName);
      if (iter != switchNameToPorts.end()) {
        iter->second.insert({neighborPortName, *portInfo.name()});
      } else {
        switchNameToPorts[neighborName] = {
            {neighborPortName, *portInfo.name()}};
      }
    }
  }
  std::cout
      << "[WARNING] No expected neighbor or more than one expected neighbor for following port(s). "
      << "This could happen on LAB devices but should not occur in PROD. \n"
      << "Port: " << folly::join(" ", noExpectedNeighbor) << "\n\n"
      << std::endl;
  return switchNameToPorts;
}

std::map<std::string, std::string> getPortToNeighbor(
    const std::shared_ptr<MultiSwitchPortMap>& portMap) {
  std::map<std::string, std::string> portToNeighbor;
  for (const auto& [switchID, ports] : std::as_const(*portMap)) {
    for (const auto& [portID, port] : std::as_const(*ports)) {
      if (port->getPortType() == cfg::PortType::FABRIC_PORT) {
        const auto& neighborReachability = port->getExpectedNeighborValues();
        if (neighborReachability->size() != 1) {
          throw std::runtime_error(
              "No expected neighbor or more than one expected neighbor for port " +
              port->getName());
        }

        std::string expectedNeighborName = *port->getExpectedNeighborValues()
                                                ->cbegin()
                                                ->get()
                                                ->toThrift()
                                                .remoteSystem();
        portToNeighbor[port->getName()] = expectedNeighborName;
      }
    }
  }
  return portToNeighbor;
}

std::unordered_map<std::string, std::vector<std::string>>
getNeighborToLinkFailure(const std::map<int32_t, PortInfoThrift>& myPortInfo) {
  std::unordered_map<std::string, std::vector<std::string>>
      neighborToLinkFailure;
  for (const auto& [_, portInfo] : myPortInfo) {
    // DOWN or INACTIVE fabric ports
    if (portInfo.portType() == cfg::PortType::FABRIC_PORT &&
        (portInfo.operState().value() == PortOperState::DOWN ||
         (portInfo.activeState().has_value() &&
          portInfo.activeState().value() == PortActiveState::INACTIVE))) {
      if (portInfo.expectedNeighborReachability()->size() != 1) {
        continue;
      }

      auto neighborName =
          *portInfo.expectedNeighborReachability()->at(0).remoteSystem();
      auto iter = neighborToLinkFailure.find(neighborName);
      if (iter != neighborToLinkFailure.end()) {
        iter->second.push_back(*portInfo.name());
      } else {
        neighborToLinkFailure[neighborName] = {*portInfo.name()};
      }
    }
  }
  return neighborToLinkFailure;
}

std::unordered_map<std::string, int> getPortToVirtualDeviceId(
    const std::map<int32_t, PortInfoThrift>& myPortInfo) {
  std::unordered_map<std::string, int> portToVD;
  for (const auto& [_, portInfo] : myPortInfo) {
    if (portInfo.portType() == cfg::PortType::FABRIC_PORT) {
      CHECK(portInfo.virtualDeviceId().has_value());
      portToVD[*portInfo.name()] = portInfo.virtualDeviceId().value();
    }
  }
  return portToVD;
}

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
    bool verbose) {
  auto getLinkFailure = [&](const std::string& neighbor) {
    auto iter = neighborToLinkFailure.find(neighbor);
    if (iter != neighborToLinkFailure.end()) {
      return iter->second;
    }
    return std::vector<std::string>{};
  };

  std::vector<InputBalanceResult> inputBalanceResult;
  for (const auto& dstSwitch : dstSwitchNames) {
    auto outputCapacityIter = outputCapacity.find(dstSwitch);
    if (outputCapacityIter == outputCapacity.end()) {
      throw std::runtime_error(
          "No output capacity data for switch " + dstSwitch);
    }
    auto outputLinkFailure = getLinkFailure(dstSwitch);

    for (const auto& [neighborSwitch, neighborReachability] : inputCapacity) {
      if (neighborSwitch == dstSwitch) {
        continue;
      }

      auto neighborReachIter = neighborReachability.find(dstSwitch);
      if (neighborReachIter == neighborReachability.end()) {
        throw std::runtime_error(
            "No input capacity data for switch " + dstSwitch +
            " from neighbor " + neighborSwitch);
      }

      auto inputLinkFailure = getLinkFailure(neighborSwitch);
      auto localLinkFailure = std::max(
          0,
          static_cast<int>(inputLinkFailure.size()) -
              static_cast<int>(outputLinkFailure.size()));

      bool balanced = (neighborReachIter->second.size() + localLinkFailure) ==
          outputCapacityIter->second.size();

      InputBalanceResult result;
      result.destinationSwitch = dstSwitch;
      result.sourceSwitch = {neighborSwitch};
      result.balanced = balanced;

      if (verbose || !balanced) {
        result.inputCapacity = neighborReachIter->second;
        result.outputCapacity = outputCapacityIter->second;
        result.inputLinkFailure = inputLinkFailure;
        result.outputLinkFailure = outputLinkFailure;
      }
      inputBalanceResult.push_back(result);
    }
  }
  return inputBalanceResult;
}

} // namespace facebook::fboss::utility
