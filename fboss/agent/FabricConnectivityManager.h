// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <memory>
#include "fboss/agent/if/gen-cpp2/FbossCtrl.h"
#include "fboss/agent/state/DsfNode.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {
class HwAsic;
class SwitchState;
class PlatformMapping;

class FabricConnectivityManager {
 public:
  explicit FabricConnectivityManager() {}
  void stateUpdated(const StateDelta& stateDelta);
  std::map<PortID, FabricEndpoint> getConnectivityInfo();
  std::map<PortID, FabricEndpoint> processConnectivityInfo(
      const std::map<PortID, FabricEndpoint>& hwConnectivity);
  FabricEndpoint processConnectivityInfoForPort(
      const PortID& portId,
      const FabricEndpoint& hwConnectivity);
  bool isConnectivityInfoMissing(const PortID& portId);
  bool isConnectivityInfoMismatch(const PortID& portId);

 private:
  void updateExpectedSwitchIdAndPortIdForPort(PortID portID);

  void addPort(const std::shared_ptr<Port>& swPort);
  void removePort(const std::shared_ptr<Port>& swPort);
  void updatePorts(const StateDelta& delta);

  void addDsfNode(const std::shared_ptr<DsfNode>& dsfNode);
  void removeDsfNode(const std::shared_ptr<DsfNode>& dsfNode);
  void updateDsfNodes(const StateDelta& delta);

  std::unordered_map<uint64_t, std::shared_ptr<DsfNode>> switchIdToDsfNode_;
  std::unordered_map<std::string, std::set<uint64_t>> switchNameToSwitchIDs_;
  std::map<PortID, FabricEndpoint> currentNeighborConnectivity_;
};
} // namespace facebook::fboss
