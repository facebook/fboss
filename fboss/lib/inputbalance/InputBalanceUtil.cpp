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

namespace facebook::fboss::utility {

bool isDualStage(const std::map<int64_t, cfg::DsfNode>& dsfNodeMap) {
  for (const auto& [_, dsfNode] : dsfNodeMap) {
    if (dsfNode.fabricLevel().has_value() && dsfNode.fabricLevel() == 2) {
      return true;
    }
  }
  return false;
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

std::map<std::string, std::vector<std::string>> getNeighborFabricPortsToSelf(
    const std::map<int32_t, PortInfoThrift>& myPortInfo) {
  std::map<std::string, std::vector<std::string>> switchNameToPorts;
  for (const auto& [_, portInfo] : myPortInfo) {
    if (portInfo.portType() == cfg::PortType::FABRIC_PORT) {
      if (portInfo.expectedNeighborReachability()->size() != 1) {
        throw std::runtime_error(
            "No expected neighbor or more than one expected neighbor for port " +
            *portInfo.name());
      }

      auto neighborName =
          *portInfo.expectedNeighborReachability()->at(0).remoteSystem();
      auto portName =
          *portInfo.expectedNeighborReachability()->at(0).remotePort();
      auto iter = switchNameToPorts.find(neighborName);
      if (iter != switchNameToPorts.end()) {
        iter->second.emplace_back(portName);
      } else {
        switchNameToPorts[neighborName] = {portName};
      }
    }
  }
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

} // namespace facebook::fboss::utility
