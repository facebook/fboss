// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/SwitchIdScopeResolver.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/types.h"

namespace facebook::fboss::utility {

int getDsfNodeCount(const HwAsic* asic);

// Returns config with remote DSF node added. If numRemoteNodes is not
// specified, it will check the asic type and use max DSF node count
// (128 for J2 and 256 for J3).
std::optional<std::map<int64_t, cfg::DsfNode>> addRemoteDsfNodeCfg(
    const std::map<int64_t, cfg::DsfNode>& curDsfNodes,
    std::optional<int> numRemoteNodes = std::nullopt);

std::shared_ptr<SwitchState> addRemoteSysPort(
    std::shared_ptr<SwitchState> currState,
    const SwitchIdScopeResolver& scopeResolver,
    SystemPortID portId,
    SwitchID remoteSwitchId = SwitchID(4));

std::shared_ptr<SwitchState> removeRemoteSysPort(
    std::shared_ptr<SwitchState> currState,
    SystemPortID portId);

std::shared_ptr<SwitchState> addRemoteInterface(
    std::shared_ptr<SwitchState> currState,
    const SwitchIdScopeResolver& scopeResolver,
    InterfaceID intfId,
    const Interface::Addresses& subnets);

std::shared_ptr<SwitchState> removeRemoteInterface(
    std::shared_ptr<SwitchState> currState,
    InterfaceID intfId);

std::shared_ptr<SwitchState> addRemoveRemoteNeighbor(
    std::shared_ptr<SwitchState> currState,
    const SwitchIdScopeResolver& scopeResolver,
    const folly::IPAddressV6& neighborIp,
    InterfaceID intfID,
    PortDescriptor port,
    bool add,
    std::optional<int64_t> encapIndex = std::nullopt);

std::shared_ptr<SwitchState> setupRemoteIntfAndSysPorts(
    std::shared_ptr<SwitchState> currState,
    const SwitchIdScopeResolver& scopeResolver,
    const cfg::SwitchConfig& config);

} // namespace facebook::fboss::utility
