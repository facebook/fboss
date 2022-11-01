// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/state/StateDelta.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h"

namespace facebook::fboss {

class FsdbStateDeltaConverter {
 public:
  std::vector<fsdb::OperDeltaUnit> computeDeltas(
      const StateDelta& stateDelta) const;

  fsdb::OperDeltaUnit createConfigDelta(
      const std::optional<cfg::SwitchConfig>& oldState,
      const std::optional<cfg::SwitchConfig>& newState) const;

  fsdb::OperDeltaUnit createSwitchStateDelta(
      const std::optional<state::SwitchState>& oldState,
      const std::optional<state::SwitchState>& newState) const;

 private:
  template <typename Path, typename Node>
  fsdb::OperDeltaUnit createDeltaUnit(
      const Path& path,
      const std::optional<Node>& oldState,
      const std::optional<Node>& newState) const;

  void processVlanMapDelta(
      std::vector<fsdb::OperDeltaUnit>& deltas,
      const VlanMapDelta& vlanDelta) const;

  void processInterfaceMapDelta(
      std::vector<fsdb::OperDeltaUnit>& deltas,
      const InterfaceMapDelta& interfaceMapDelta) const;

  // creates deltas for each node in node map, does not traverse to child nodes
  template <typename Path, typename MapDeltaT, typename GetKey>
  void processNodeMapDelta(
      std::vector<fsdb::OperDeltaUnit>& operDeltas,
      const MapDeltaT& nodeMapDelta,
      const GetKey& getKey,
      const Path& basePath) const;

  // helper for nodes that define getThriftNodeKey
  template <typename Path, typename MapDeltaT>
  void processThriftyNodeMapDelta(
      std::vector<fsdb::OperDeltaUnit>& operDeltas,
      const MapDeltaT& nodeMapDelta,
      const Path& basePath) const;

  template <typename Path, typename Node, typename Key>
  void processNodeDelta(
      std::vector<fsdb::OperDeltaUnit>& deltas,
      const Path& basePath,
      const Key& nodeID,
      const Node& oldNode,
      const Node& newNode) const;
};

} // namespace facebook::fboss
