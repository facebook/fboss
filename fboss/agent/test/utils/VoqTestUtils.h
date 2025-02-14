// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/SwitchIdScopeResolver.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {
class TestEnsembleIf;
class SwSwitch;

namespace utility {

std::shared_ptr<SwitchState> addRemoteSysPort(
    std::shared_ptr<SwitchState> currState,
    const SwitchIdScopeResolver& scopeResolver,
    SystemPortID portId,
    SwitchID remoteSwitchId,
    int coreIndex = 0,
    int corePortIndex = 1,
    HwAsic::InterfaceNodeRole intfRole =
        HwAsic::InterfaceNodeRole::IN_CLUSTER_NODE,
    cfg::PortType portType = cfg::PortType::INTERFACE_PORT);

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

QueueConfig getDefaultVoqConfig(cfg::PortType portType);

std::optional<uint64_t> getDummyEncapIndex(TestEnsembleIf* ensemble);

boost::container::flat_set<PortDescriptor> resolveRemoteNhops(
    TestEnsembleIf* ensemble,
    utility::EcmpSetupTargetedPorts6& ecmpHelper);

void populateRemoteIntfAndSysPorts(
    std::map<SwitchID, std::shared_ptr<SystemPortMap>>& switchId2SystemPorts,
    std::map<SwitchID, std::shared_ptr<InterfaceMap>>& switchId2Rifs,
    const cfg::SwitchConfig& config,
    bool useEncapIndex);

void setupRemoteIntfAndSysPorts(SwSwitch* swSwitch, bool useEncapIndex);

struct QueueConfigAndName {
  std::string name;
  std::vector<cfg::PortQueue> queueConfig;
};
std::optional<QueueConfigAndName> getNameAndDefaultVoqCfg(
    cfg::PortType portType);

uint8_t getDefaultQueue();

int getTrafficClassToVoqId(const HwAsic* hwAsic, int trafficClass);

int getTrafficClassToCpuVoqId(const HwAsic* hwAsic, int trafficClass);
} // namespace utility
} // namespace facebook::fboss
