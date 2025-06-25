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

namespace {

constexpr auto kNumVirtualDevice = 4;

std::vector<std::vector<std::string>> groupPortsByVD(
    const std::vector<std::string>& ports,
    const std::unordered_map<std::string, int>& portToVirtualDevice) {
  std::vector<std::vector<std::string>> portsByVD;
  portsByVD.reserve(kNumVirtualDevice);
  for (int i = 0; i < kNumVirtualDevice; i++) {
    portsByVD.emplace_back();
  }
  for (const auto& port : ports) {
    portsByVD.at(portToVirtualDevice.at(port)).push_back(port);
  }
  return portsByVD;
}

std::vector<std::vector<std::string>> getLinkFailure(
    const std::vector<std::string>& neighbors,
    const std::unordered_map<std::string, std::vector<std::string>>&
        neighborToLinkFailure,
    const std::unordered_map<std::string, int>& portToVirtualDevice) {
  std::vector<std::string> linkFailures;
  for (const auto& neighbor : neighbors) {
    auto iter = neighborToLinkFailure.find(neighbor);
    if (iter != neighborToLinkFailure.end()) {
      linkFailures.insert(
          linkFailures.end(), iter->second.begin(), iter->second.end());
    }
  }
  return groupPortsByVD(linkFailures, portToVirtualDevice);
};

std::vector<std::string> getFdswsInCluster(
    int clusterID,
    const std::unordered_map<std::string, facebook::fboss::cfg::DsfNode>&
        nameToDsfNode) {
  std::vector<std::string> fdsws;
  for (const auto& [name, dsfNode] : nameToDsfNode) {
    if (dsfNode.clusterId().has_value() &&
        dsfNode.clusterId().value() == clusterID &&
        dsfNode.fabricLevel().has_value() &&
        dsfNode.fabricLevel().value() == 1) {
      fdsws.emplace_back(name);
    }
  }
  return fdsws;
}

std::vector<std::string> getSdswsInCluster(
    const std::unordered_map<std::string, facebook::fboss::cfg::DsfNode>&
        nameToDsfNode) {
  std::vector<std::string> sdsws;
  for (const auto& [name, dsfNode] : nameToDsfNode) {
    if (dsfNode.type() == facebook::fboss::cfg::DsfNodeType::FABRIC_NODE &&
        dsfNode.fabricLevel().has_value() &&
        dsfNode.fabricLevel().value() == 2) {
      sdsws.emplace_back(name);
    }
  }
  return sdsws;
}

} // namespace

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

std::vector<std::string> devicesToQueryInputCapacity(
    const std::vector<int64_t>& fabricSwitchIDs,
    const std::map<int64_t, cfg::DsfNode>& dsfNodeMap) {
  std::vector<std::string> devicesToQuery;
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
        devicesToQuery.emplace_back(dsfNode.name().value());
      }
    }
  }
  // TODO(zecheng): Implement functionality for dual stage
  return devicesToQuery;
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

std::unordered_map<int, std::vector<std::string>> groupFabricDevicesByCluster(
    const std::unordered_map<std::string, cfg::DsfNode>& nameToDsfNode) {
  std::unordered_map<int, std::vector<std::string>> clusterToFabricDevices;
  for (const auto& [name, dsfNode] : nameToDsfNode) {
    if (dsfNode.clusterId().has_value()) {
      auto clusterID = dsfNode.clusterId().value();
      if (dsfNode.type() == cfg::DsfNodeType::FABRIC_NODE) {
        auto iter = clusterToFabricDevices.find(clusterID);
        if (iter != clusterToFabricDevices.end()) {
          iter->second.emplace_back(name);
        } else {
          clusterToFabricDevices[clusterID] = {name};
        }
      }
    }
  }
  return clusterToFabricDevices;
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
    const std::unordered_map<std::string, int>& portToVirtualDevice,
    bool verbose) {
  std::vector<InputBalanceResult> inputBalanceResult;
  for (const auto& dstSwitch : dstSwitchNames) {
    auto outputCapacityIter = outputCapacity.find(dstSwitch);
    if (outputCapacityIter == outputCapacity.end()) {
      throw std::runtime_error(
          "No output capacity data for switch " + dstSwitch);
    }
    auto outputCapacityByVD =
        groupPortsByVD(outputCapacityIter->second, portToVirtualDevice);
    auto outputLinkFailure =
        getLinkFailure({dstSwitch}, neighborToLinkFailure, portToVirtualDevice);

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

      auto inputCapacityByVD =
          groupPortsByVD(neighborReachIter->second, portToVirtualDevice);
      auto inputLinkFailure = getLinkFailure(
          {neighborSwitch}, neighborToLinkFailure, portToVirtualDevice);

      for (int vd = 0; vd < kNumVirtualDevice; vd++) {
        auto localLinkFailure = std::max(
            0,
            static_cast<int>(inputLinkFailure.at(vd).size()) -
                static_cast<int>(outputLinkFailure.at(vd).size()));

        bool balanced = (inputCapacityByVD.at(vd).size() + localLinkFailure) ==
            outputCapacityByVD.at(vd).size();

        InputBalanceResult result;
        result.destinationSwitch = dstSwitch;
        result.sourceSwitch = {neighborSwitch};
        result.balanced = balanced;
        result.virtualDeviceID = vd;

        if (verbose || !balanced) {
          result.inputCapacity = inputCapacityByVD.at(vd);
          result.outputCapacity = outputCapacityByVD.at(vd);
          result.inputLinkFailure = inputLinkFailure.at(vd);
          result.outputLinkFailure = outputLinkFailure.at(vd);
        }
        inputBalanceResult.push_back(result);
      }
    }
  }
  return inputBalanceResult;
}

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
    bool verbose) {
  CHECK(
      inputBalanceDestType == InputBalanceDestType::DUAL_STAGE_SDSW_INTER ||
      inputBalanceDestType == InputBalanceDestType::DUAL_STAGE_FDSW_INTER);
  std::vector<InputBalanceResult> inputBalanceResult;
  for (const auto& dstSwitch : dstSwitchNames) {
    auto outputCapacityIter = outputCapacity.find(dstSwitch);
    if (outputCapacityIter == outputCapacity.end()) {
      throw std::runtime_error(
          "No output capacity data for switch " + dstSwitch);
    }
    auto outputCapacityByVD =
        groupPortsByVD(outputCapacityIter->second, portToVirtualDevice);
    auto dstDsfNodeIter = switchNameToDsfNode.find(dstSwitch);
    if (dstDsfNodeIter == switchNameToDsfNode.end()) {
      throw std::runtime_error("No dsfNode Found for " + dstSwitch);
    }

    std::vector<std::string> outputNeighbors;
    if (inputBalanceDestType == InputBalanceDestType::DUAL_STAGE_SDSW_INTER) {
      auto dstClusterID = dstDsfNodeIter->second.clusterId();
      CHECK(dstClusterID.has_value());
      outputNeighbors =
          getFdswsInCluster(dstClusterID.value(), switchNameToDsfNode);
    } else if (
        inputBalanceDestType == InputBalanceDestType::DUAL_STAGE_FDSW_INTER) {
      outputNeighbors = getSdswsInCluster(switchNameToDsfNode);
    }
    if (outputNeighbors.empty()) {
      throw std::runtime_error("Unexpected empty output neighbor.");
    }
    auto outputLinkFailure = getLinkFailure(
        outputNeighbors, neighborToLinkFailure, portToVirtualDevice);

    std::vector<std::string> inputNeighbors;
    std::vector<std::vector<std::string>> inputCapacityByVD;
    inputCapacityByVD.resize(kNumVirtualDevice);
    for (const auto& [neighborSwitch, neighborReachability] : inputCapacity) {
      inputNeighbors.emplace_back(neighborSwitch);
      auto neighborReachIter = neighborReachability.find(dstSwitch);
      if (neighborReachIter == neighborReachability.end()) {
        std::cout << "[WARNING] No input capacity data for switch " +
                dstSwitch + " from neighbor " + neighborSwitch;
        continue;
      }
      auto singleCapacity =
          groupPortsByVD(neighborReachIter->second, portToVirtualDevice);
      for (int vd = 0; vd < kNumVirtualDevice; vd++) {
        inputCapacityByVD.at(vd).insert(
            inputCapacityByVD.at(vd).end(),
            singleCapacity.at(vd).begin(),
            singleCapacity.at(vd).end());
      }
    }
    auto inputLinkFailure = getLinkFailure(
        inputNeighbors, neighborToLinkFailure, portToVirtualDevice);
    for (int vd = 0; vd < kNumVirtualDevice; vd++) {
      auto localLinkFailure = std::max(
          0,
          static_cast<int>(inputLinkFailure.at(vd).size()) -
              static_cast<int>(outputLinkFailure.at(vd).size()));
      bool balanced = (inputCapacityByVD.at(vd).size() + localLinkFailure) ==
          outputCapacityByVD.at(vd).size();
      InputBalanceResult result;
      result.destinationSwitch = dstSwitch;
      result.sourceSwitch = inputNeighbors;
      result.balanced = balanced;
      result.virtualDeviceID = vd;
      if (verbose || !balanced) {
        result.inputCapacity = inputCapacityByVD.at(vd);
        result.outputCapacity = outputCapacityByVD.at(vd);
        result.inputLinkFailure = inputLinkFailure.at(vd);
        result.outputLinkFailure = outputLinkFailure.at(vd);
      }
      inputBalanceResult.push_back(result);
    }
  }
  return inputBalanceResult;
}

} // namespace facebook::fboss::utility
