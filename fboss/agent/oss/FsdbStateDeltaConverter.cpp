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

template <typename T>
fsdb::OperDeltaUnit FsdbStateDeltaConverter::createDeltaUnit(
    const std::vector<std::string>& /* path */,
    const std::optional<T>& /* oldState */,
    const std::optional<T>& /* newState */) const {
  return fsdb::OperDeltaUnit();
}

void FsdbStateDeltaConverter::processVlanMapDelta(
    std::vector<fsdb::OperDeltaUnit>& /* deltas */,
    const VlanMapDelta& /* vlanMapDelta */) const {}

template <typename MapDelta>
void FsdbStateDeltaConverter::processNodeMapDelta(
    std::vector<fsdb::OperDeltaUnit>& /* operDeltas */,
    const MapDelta& /* nodeMapDelta */,
    const std::vector<std::string>& /* basePath */) const {}

template <typename T>
void FsdbStateDeltaConverter::processNodeDelta(
    std::vector<fsdb::OperDeltaUnit>& /* deltas */,
    const std::vector<std::string>& /* basePath */,
    const std::string& /* nodeID */,
    const std::shared_ptr<T>& /* oldNode */,
    const std::shared_ptr<T>& /* newNode */) const {}

} // namespace facebook::fboss
