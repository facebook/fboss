// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/FsdbStateDeltaConverter.h"

namespace facebook::fboss {
std::vector<fsdb::OperDeltaUnit> FsdbStateDeltaConverter::computeDeltas(
    const StateDelta& /* stateDelta */) const {
  return std::vector<fsdb::OperDeltaUnit>();
}

fsdb::OperDeltaUnit FsdbStateDeltaConverter::createConfigDelta(
    const std::optional<cfg::SwitchConfig>& /* oldState */,
    const std::optional<cfg::SwitchConfig>& /* newState */) const {
  return fsdb::OperDeltaUnit();
}

fsdb::OperDeltaUnit FsdbStateDeltaConverter::createSwitchStateDelta(
    const std::optional<state::SwitchState>& /* oldState */,
    const std::optional<state::SwitchState>& /* newState */) const {
  return fsdb::OperDeltaUnit();
}

fsdb::OperDeltaUnit FsdbStateDeltaConverter::createBitsflowLockdownLevelDelta(
    const std::optional<std::string>& /* oldState */,
    const std::optional<std::string>& /* newState */) const {
  return fsdb::OperDeltaUnit();
}

template <typename Path, typename Node>
fsdb::OperDeltaUnit FsdbStateDeltaConverter::createDeltaUnit(
    const Path& /* path */,
    const std::optional<Node>& /* oldState */,
    const std::optional<Node>& /* newState */) const {
  return fsdb::OperDeltaUnit();
}

void FsdbStateDeltaConverter::processVlanMapDelta(
    std::vector<fsdb::OperDeltaUnit>& /* deltas */,
    const VlanMapDelta& /* vlanMapDelta */) const {}

void FsdbStateDeltaConverter::processInterfaceMapDelta(
    std::vector<fsdb::OperDeltaUnit>& /* deltas */,
    const InterfaceMapDelta& /* interfaceMapDelta */) const {}

template <typename Path, typename MapDeltaT, typename GetKey>
void FsdbStateDeltaConverter::processNodeMapDelta(
    std::vector<fsdb::OperDeltaUnit>& /* operDeltas */,
    const MapDeltaT& /* nodeMapDelta */,
    const GetKey& /* getKey */,
    const Path& /* basePath */) const {}

template <typename Path, typename MapDeltaT>
void FsdbStateDeltaConverter::processThriftyNodeMapDelta(
    std::vector<fsdb::OperDeltaUnit>& /* operDeltas */,
    const MapDeltaT& /* nodeMapDelta */,
    const Path& /* basePath */) const {}

template <typename Path, typename Node, typename Key>
void FsdbStateDeltaConverter::processNodeDelta(
    std::vector<fsdb::OperDeltaUnit>& /* deltas */,
    const Path& /* basePath */,
    const Key& /* nodeID */,
    const Node& /* oldNode */,
    const Node& /* newNode */) const {}

} // namespace facebook::fboss
