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
  std::map<PortID, FabricEndpoint> processReachabilityInfo(
      const std::map<PortID, FabricEndpoint>& hwReachability);
  FabricEndpoint processReachabilityInfoForPort(
      const PortID& portId,
      const FabricEndpoint& hwReachability);
  void addPort(const std::shared_ptr<Port> swPort);
  void removePort(const std::shared_ptr<Port> swPort);
  void addDsfNode(const std::shared_ptr<DsfNode> dsfNode);
  void removeDsfNode(const std::shared_ptr<DsfNode> dsfNode);

 private:
  void updatePorts(const StateDelta& delta);
  void updateDsfNodes(const StateDelta& delta);

  std::unordered_map<PortID, NeighborReachabilityList>
      expectedNeighborReachability_;
  std::unordered_map<std::string, std::shared_ptr<DsfNode>> dsfNodes_;
  std::map<PortID, FabricEndpoint> actualNeighborReachability_;
};
} // namespace facebook::fboss
