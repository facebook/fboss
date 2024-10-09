// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <memory>
#include <ostream>
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
void toAppend(const FabricEndpoint& endpoint, folly::fbstring* result);
void toAppend(const FabricEndpoint& endpoint, std::string* result);
void toAppend(
    const multiswitch::FabricConnectivityDelta& delta,
    folly::fbstring* result);
void toAppend(
    const multiswitch::FabricConnectivityDelta& delta,
    std::string* result);

std::ostream& operator<<(std::ostream& os, const RemoteEndpoint& endpoint);
std::ostream& operator<<(std::ostream& os, const FabricEndpoint& endpoint);
std::ostream& operator<<(
    std::ostream& os,
    const multiswitch::FabricConnectivityDelta& delta);

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
  std::optional<multiswitch::FabricConnectivityDelta>
  processConnectivityInfoForPort(
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
  static int virtualDevicesWithAsymmetricConnectivity(
      const std::map<int64_t, RemoteConnectionGroups>&
          virtualDevice2RemoteConnectionGroups);
  std::optional<PortID> getActualPortIdForSwitch(
      int32_t portId,
      uint64_t switchId,
      uint64_t baseSwitchId,
      const auto& switchName);
  std::pair<std::optional<std::string>, std::optional<std::string>>
  getActualSwitchNameAndPortName(uint64_t switchId, int32_t portId);

 private:
  void updateExpectedSwitchIdAndPortIdForPort(PortID portID);

  void addOrUpdatePort(const std::shared_ptr<Port>& swPort);
  void removePort(const std::shared_ptr<Port>& swPort);
  void updatePorts(const StateDelta& delta);

  void addDsfNode(const std::shared_ptr<DsfNode>& dsfNode);
  void removeDsfNode(const std::shared_ptr<DsfNode>& dsfNode);
  void updateDsfNodes(const StateDelta& delta);

  std::unordered_map<uint64_t, std::shared_ptr<DsfNode>> switchIdToDsfNode_;
  std::unordered_map<std::string, std::set<uint64_t>> switchNameToSwitchIDs_;
  std::unordered_map<uint64_t, std::pair<uint64_t, std::string>>
      switchIdToBaseSwitchIdAndSwitchName_;
  std::map<PortID, FabricEndpoint> currentNeighborConnectivity_;
  std::map<PortID, std::string> fabricPortId2Name_;
};
} // namespace facebook::fboss
