// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <memory>
#include "fboss/agent/if/gen-cpp2/FbossCtrl.h"
#include "fboss/agent/state/DsfNode.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {
class SwitchState;
class PlatformMapping;

class FabricReachabilityManager {
  using NeighborReachabilityList = std::vector<cfg::PortNeighbor>;

 public:
  explicit FabricReachabilityManager() {}
  void stateUpdated(const StateDelta& stateDelta);
  std::map<PortID, FabricEndpoint> getReachabilityInfo();
  std::map<PortID, FabricEndpoint> processReachabilityInfo(
      const std::map<PortID, FabricEndpoint>& hwReachability);
  FabricEndpoint processReachabilityInfoForPort(
      const PortID& portId,
      const FabricEndpoint& hwReachability);
  bool isReachabilityInfoMissing(const PortID& portId);
  bool isReachabilityInfoMismatch(const PortID& portId);

 private:
  void updatePorts(const StateDelta& delta);
  void updateDsfNodes(const StateDelta& delta);
  void addPort(const std::shared_ptr<Port>& swPort);
  void removePort(const std::shared_ptr<Port>& swPort);
  void addDsfNode(const std::shared_ptr<DsfNode>& dsfNode);
  void removeDsfNode(const std::shared_ptr<DsfNode>& dsfNode);

  void updateExpectedPortId(
      FabricEndpoint& endpoint,
      const cfg::AsicType asicType,
      const PlatformType platformType,
      const cfg::DsfNodeType dsfNodeType);
  void updatePortName(
      FabricEndpoint& endpoint,
      const cfg::AsicType asicType,
      PlatformType platformType);
  void updateHwFabricEndpointInfo(FabricEndpoint& resEndpoint);
  void updateExpectedFabricEndpointInfo(
      FabricEndpoint& resEndpoint,
      const FabricEndpoint& expectedEndpoint);

  std::unordered_map<std::string, std::shared_ptr<DsfNode>> dsfNameToNodesMap_;
  std::unordered_map<uint64_t, std::shared_ptr<DsfNode>> dsfSwitchIdToNodesMap_;
  std::map<PortID, FabricEndpoint> actualNeighborReachability_;
};
} // namespace facebook::fboss
