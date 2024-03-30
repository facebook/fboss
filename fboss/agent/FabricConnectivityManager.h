// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <memory>
#include "fboss/agent/HwSwitchCallback.h"
#include "fboss/agent/if/gen-cpp2/FbossCtrl.h"
#include "fboss/agent/if/gen-cpp2/FbossHwCtrl.h"
#include "fboss/agent/state/DsfNode.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {
class HwAsic;
class SwitchState;
class PlatformMapping;

void toAppend(const RemoteEndpoint& endpoint, folly::fbstring* result);
void toAppend(const RemoteEndpoint& endpoint, std::string* result);

class FabricConnectivityManager {
  struct CompareRemoteEndpoint {
    bool operator()(const RemoteEndpoint& l, const RemoteEndpoint& r) const {
      return l.switchId() < r.switchId();
    }
  };

 public:
  FabricConnectivityManager() = default;

  void stateUpdated(const StateDelta& stateDelta);
  const std::map<PortID, FabricEndpoint>& getConnectivityInfo() const;
  std::optional<FabricConnectivityDelta> processConnectivityInfoForPort(
      const PortID& portId,
      const FabricEndpoint& hwConnectivity);
  bool isConnectivityInfoMissing(const PortID& portId);
  bool isConnectivityInfoMismatch(const PortID& portId);
  static bool isConnectivityInfoMismatch(const FabricEndpoint& endpoint);

  using RemoteEndpoints = std::set<RemoteEndpoint, CompareRemoteEndpoint>;
  using RemoteConnectionGroups = std::map<int, RemoteEndpoints>;
  std::map<int64_t, RemoteConnectionGroups>
  getVirtualDeviceToRemoteConnectionGroups(
      const std::function<int(PortID)>& portToVirtualDevice) const;

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
  std::map<PortID, std::string> fabricPortId2Name_;
};
} // namespace facebook::fboss
