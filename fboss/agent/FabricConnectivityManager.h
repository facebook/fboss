// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <memory>
#include <ostream>
#include "fboss/agent/if/gen-cpp2/FbossCtrl.h"
#include "fboss/agent/state/DsfNode.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {
class HwAsic;
class SwitchState;
class PlatformMapping;

struct RemoteEndpoint {
  int64_t switchId;
  std::string switchName;
  std::vector<std::string> connectingPorts;
  bool operator<(const RemoteEndpoint& r) const {
    return switchId < r.switchId;
  }
  std::string toStr() const;
};
inline void toAppend(const RemoteEndpoint& endpoint, folly::fbstring* result) {
  result->append(endpoint.toStr());
}
inline void toAppend(const RemoteEndpoint& endpoint, std::string* result) {
  *result += endpoint.toStr();
}

class FabricConnectivityManager {
 public:
  FabricConnectivityManager() = default;

  void stateUpdated(const StateDelta& stateDelta);
  const std::map<PortID, FabricEndpoint>& getConnectivityInfo() const;
  std::map<PortID, FabricEndpoint> processConnectivityInfo(
      const std::map<PortID, FabricEndpoint>& hwConnectivity);
  FabricEndpoint processConnectivityInfoForPort(
      const PortID& portId,
      const FabricEndpoint& hwConnectivity);
  bool isConnectivityInfoMissing(const PortID& portId);
  bool isConnectivityInfoMismatch(const PortID& portId);

  using RemoteConnectionGroups = std::map<int, std::set<RemoteEndpoint>>;
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
