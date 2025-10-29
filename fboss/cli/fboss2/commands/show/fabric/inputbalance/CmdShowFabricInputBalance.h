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
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/fabric/inputbalance/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"
#include "fboss/lib/inputbalance/InputBalanceUtil.h"

namespace facebook::fboss {

struct CmdShowFabricInputBalanceTraits : public ReadCommandTraits {
  using ParentCmd = CmdShowFabric;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_SWITCH_NAME_LIST;
  using ObjectArgType = std::vector<std::string>;
  using RetType = cli::ShowFabricInputBalanceModel;
};
class CmdShowFabricInputBalance : public CmdHandler<
                                      CmdShowFabricInputBalance,
                                      CmdShowFabricInputBalanceTraits> {
 public:
  using ObjectArgType = CmdShowFabricInputBalanceTraits::ObjectArgType;
  using RetType = CmdShowFabricInputBalanceTraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const ObjectArgType& queriedSwitchNames) {
    if (queriedSwitchNames.empty()) {
      throw std::runtime_error(
          "Please provide the list of switch name(s) to query reachability.");
    }
    std::vector<std::string> dstSwitchName;
    dstSwitchName.reserve(queriedSwitchNames.size());
    for (const auto& queriedSwitchName : queriedSwitchNames) {
      dstSwitchName.push_back(utils::removeFbDomains(queriedSwitchName));
    }

    auto fbossCtrlClient =
        utils::createClient<apache::thrift::Client<FbossCtrl>>(hostInfo);

    std::map<int64_t, cfg::SwitchInfo> switchIdToSwitchInfo;
    fbossCtrlClient->sync_getSwitchIdToSwitchInfo(switchIdToSwitchInfo);
    if (std::none_of(
            switchIdToSwitchInfo.cbegin(),
            switchIdToSwitchInfo.cend(),
            [](const auto& entry) {
              return entry.second.switchType() == cfg::SwitchType::FABRIC;
            })) {
      throw std::runtime_error(
          "No Fabric device found in SwitchIdToSwitchInfo. "
          "Input Balance only applicable to FABRIC devices.");
    }

    std::map<int64_t, cfg::DsfNode> dsfNodeMap;
    fbossCtrlClient->sync_getDsfNodes(dsfNodeMap);
    bool isDualStage = utility::isDualStage(dsfNodeMap);

    std::map<int32_t, facebook::fboss::PortInfoThrift> myPortInfo;
    fbossCtrlClient->sync_getAllPortInfo(myPortInfo);
    auto neighborToPorts = utility::getNeighborFabricPortsToSelf(myPortInfo);
    auto neighborToLinkFailure = utility::getNeighborToLinkFailure(myPortInfo);
    auto portToVirtualDevice = utility::getPortToVirtualDeviceId(myPortInfo);
    auto selfReachability =
        utils::getCachedSwSwitchReachabilityInfo(hostInfo, dstSwitchName);

    if (!isDualStage) {
      std::vector<int64_t> fabricSwitchIDs;
      for (const auto& [switchId, switchInfo] : switchIdToSwitchInfo) {
        if (switchInfo.switchType() == cfg::SwitchType::FABRIC) {
          fabricSwitchIDs.push_back(switchId);
        }
      }

      auto devicesToQueryInputCapacity =
          utility::devicesToQueryInputCapacity(fabricSwitchIDs, dsfNodeMap);
      if (devicesToQueryInputCapacity.empty()) {
        throw std::runtime_error(
            "Failed to find devices to query input capacity");
      }

      auto neighborReachability = getNeighborReachability(
          devicesToQueryInputCapacity, neighborToPorts, dstSwitchName);

      return createModel(
          utility::checkInputBalanceSingleStage(
              dstSwitchName,
              neighborReachability,
              selfReachability,
              neighborToLinkFailure,
              portToVirtualDevice,
              true /* verbose */));
    } else {
      auto switchID = switchIdToSwitchInfo.begin()->first;
      auto dsfNode = dsfNodeMap.at(switchID);

      if (!dsfNode.fabricLevel().has_value()) {
        throw std::runtime_error(
            "Failed to find fabric level for device " + hostInfo.getName());
      }

      std::vector<utility::InputBalanceResult> inputBalanceResult;
      auto nameToDsfNode = utility::switchNameToDsfNode(dsfNodeMap);

      // TODO(zecheng): Handle dst switches in different clusters.
      auto dstClusterID = *nameToDsfNode.at(dstSwitchName.at(0)).clusterId();
      if (dsfNode.fabricLevel() == 2) {
        // Dual stage SDSW
        auto clusterIDToFabricDevices =
            utility::groupFabricDevicesByCluster(nameToDsfNode);

        for (const auto& [clusterID, fabricDevices] :
             clusterIDToFabricDevices) {
          if (clusterID != dstClusterID) {
            auto neighborReachability = getNeighborReachability(
                fabricDevices, neighborToPorts, dstSwitchName);
            auto result = utility::checkInputBalanceDualStage(
                utility::InputBalanceDestType::DUAL_STAGE_SDSW_INTER,
                dstSwitchName,
                neighborReachability,
                selfReachability,
                neighborToLinkFailure,
                portToVirtualDevice,
                nameToDsfNode,
                true /* verbose */);
            inputBalanceResult.insert(
                inputBalanceResult.end(), result.begin(), result.end());
          }
        }
      } else {
        // Dual stage FDSW
        auto localClusterID = dsfNodeMap.at(switchID).clusterId();
        CHECK(localClusterID.has_value());

        auto localRDSW = utility::getInterfaceDevicesInCluster(
            nameToDsfNode, localClusterID.value());

        if (localClusterID.value() != dstClusterID) {
          // Inter-zone destination

          auto neighborReachability = getNeighborReachability(
              localRDSW, neighborToPorts, dstSwitchName);
          return createModel(
              utility::checkInputBalanceDualStage(
                  utility::InputBalanceDestType::DUAL_STAGE_FDSW_INTER,
                  dstSwitchName,
                  neighborReachability,
                  selfReachability,
                  neighborToLinkFailure,
                  portToVirtualDevice,
                  nameToDsfNode,
                  true /* verbose */));
        } else {
          // Intra-zone destination - need to filter out output reachability
          // towards SDSW

          selfReachability = utility::filterReachabilityByDst(
              dstSwitchName, selfReachability, neighborToPorts);

          // Also need to check reachability from SDSW and local RDSW
          auto neighbors =
              utility::getLayer2FabricDevicesInCluster(nameToDsfNode);
          neighbors.insert(neighbors.end(), localRDSW.begin(), localRDSW.end());
          auto neighborReachability = getNeighborReachability(
              neighbors, neighborToPorts, dstSwitchName);

          return createModel(
              utility::checkInputBalanceDualStage(
                  utility::InputBalanceDestType::DUAL_STAGE_FDSW_INTRA,
                  dstSwitchName,
                  neighborReachability,
                  selfReachability,
                  neighborToLinkFailure,
                  portToVirtualDevice,
                  nameToDsfNode,
                  true /* verbose */));
        }
      }

      return createModel(inputBalanceResult);
    }
  }

  RetType createModel(
      const std::vector<utility::InputBalanceResult>& inputBalanceResults) {
    RetType ret;
    std::vector<cli::InputBalanceEntry> entries;

    for (const auto& result : inputBalanceResults) {
      cli::InputBalanceEntry entry;
      entry.destinationSwitchName() = result.destinationSwitch;
      entry.sourceSwitchName() = result.sourceSwitch;
      entry.virtualDeviceID() = result.virtualDeviceID;
      entry.balanced() = result.balanced;
      entry.inputCapacity() = result.inputCapacity.value();
      entry.outputCapacity() = result.outputCapacity.value();
      entry.inputLinkFailure() = result.inputLinkFailure.value();
      entry.outputLinkFailure() = result.outputLinkFailure.value();
      entries.push_back(entry);
    }
    ret.inputBalanceEntry() = entries;
    return ret;
  }

  void printOutput(const RetType& model, std::ostream& out = std::cout) {
    Table table;
    table.setHeader({
        "Destination",
        "Source",
        "VirtualDeviceID",
        "Balanced",
        "InputCapacity",
        "OutputCapacity",
        "InputLinkFailure",
        "OutputLinkFailure",
    });
    for (const auto& entry : *model.inputBalanceEntry()) {
      table.addRow(
          {*entry.destinationSwitchName(),
           entry.sourceSwitchName()->size() == 1
               ? folly::join(" ", *entry.sourceSwitchName())
               : folly::to<std::string>(
                     entry.sourceSwitchName()->size(), " devices"),
           folly::to<std::string>(*entry.virtualDeviceID()),
           *entry.balanced() ? "True" : "False",
           folly::to<std::string>(entry.inputCapacity()->size()),
           folly::to<std::string>(entry.outputCapacity()->size()),
           folly::to<std::string>(entry.inputLinkFailure()->size()),
           folly::to<std::string>(entry.outputLinkFailure()->size())});
    }
    out << table << std::endl;
  }

 private:
  // Returns unordered_map<neighborSwitch, unordered_map<destinationSwitch,
  // std::vector<neighboringPorts>>>
  std::unordered_map<
      std::string,
      std::unordered_map<std::string, std::vector<std::string>>>
  getNeighborReachability(
      const std::vector<std::string>& devicesToQueryInputCapacity,
      const std::unordered_map<
          std::string,
          std::unordered_map<std::string, std::string>>& neighborName2Ports,
      const std::vector<std::string>& dstSwitchNames) {
    std::unordered_map<
        std::string,
        std::unordered_map<std::string, std::vector<std::string>>>
        neighborReachability;
    std::map<
        std::string,
        std::shared_future<
            std::unordered_map<std::string, std::vector<std::string>>>>
        neighborReachabilityFutureMap;
    for (const auto& switchToQuery : devicesToQueryInputCapacity) {
      neighborReachabilityFutureMap[switchToQuery] = std::async(
          std::launch::async,
          &utils::getCachedSwSwitchReachabilityInfo,
          HostInfo(switchToQuery),
          dstSwitchNames);
    }

    for (const auto& [neighborName, neighborReachabilityFuture] :
         neighborReachabilityFutureMap) {
      auto neighborReachabilityMap = neighborReachabilityFuture.get();
      std::unordered_map<std::string, std::vector<std::string>>
          filteredReachabilityMap;
      for (const auto& [dstSwitch, ports] : neighborReachabilityMap) {
        std::vector<std::string> filteredPorts;
        for (const auto& port : ports) {
          // Filter ports that are not connected to the source switch
          auto neighbor2PortIter = neighborName2Ports.find(neighborName);
          if (neighbor2PortIter == neighborName2Ports.end()) {
            throw std::runtime_error(
                "Unable to find Neighbor " + neighborName +
                " in neighborName2Ports.");
          }
          auto portIter = neighbor2PortIter->second.find(port);
          if (portIter != neighbor2PortIter->second.end()) {
            filteredPorts.push_back(portIter->second);
          }
        }
        filteredReachabilityMap[dstSwitch] = std::move(filteredPorts);
      }
      neighborReachability[neighborName] = std::move(filteredReachabilityMap);
    }
    return neighborReachability;
  }
};
} // namespace facebook::fboss
