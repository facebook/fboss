// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/SwitchIdScopeResolver.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/gen-cpp2/hardware_stats_types.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {
class TestEnsembleIf;
class SwSwitch;

namespace utility {

std::shared_ptr<SystemPort> makeRemoteSysPort(
    SystemPortID portId,
    SwitchID remoteSwitchId,
    int coreIndex = 0,
    int corePortIndex = 1,
    int64_t speed = 800000,
    HwAsic::InterfaceNodeRole intfRole =
        HwAsic::InterfaceNodeRole::IN_CLUSTER_NODE,
    cfg::PortType portType = cfg::PortType::INTERFACE_PORT,
    std::string remoteSwitchName = "");

std::shared_ptr<Interface> makeRemoteInterface(
    InterfaceID intfId,
    const Interface::Addresses& subnets);

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

boost::container::flat_set<PortDescriptor> getRemoteSysPorts(
    TestEnsembleIf* ensemble);

boost::container::flat_set<PortDescriptor> resolveRemoteNhops(
    TestEnsembleIf* ensemble,
    utility::EcmpSetupTargetedPorts6& ecmpHelper);

boost::container::flat_set<PortDescriptor> resolveRemoteNhops(
    TestEnsembleIf* ensemble,
    utility::EcmpSetupTargetedPorts6& ecmpHelper,
    const boost::container::flat_set<PortDescriptor>& sysPortDescs);

boost::container::flat_set<PortDescriptor> unresolveRemoteNhops(
    TestEnsembleIf* ensemble,
    utility::EcmpSetupTargetedPorts6& ecmpHelper);

boost::container::flat_set<PortDescriptor> unresolveRemoteNhops(
    TestEnsembleIf* ensemble,
    utility::EcmpSetupTargetedPorts6& ecmpHelper,
    const boost::container::flat_set<PortDescriptor>& sysPortDescs);

void populateRemoteIntfAndSysPorts(
    std::map<SwitchID, std::shared_ptr<SystemPortMap>>& switchId2SystemPorts,
    std::map<SwitchID, std::shared_ptr<InterfaceMap>>& switchId2Rifs,
    const cfg::SwitchConfig& config,
    bool useEncapIndex,
    bool addNeighborToIntf = true,
    bool useHyperPort = false);

void setupRemoteIntfAndSysPorts(
    SwSwitch* swSwitch,
    bool useEncapIndex,
    bool useHyperPort = false);

struct QueueConfigAndName {
  std::string name;
  std::vector<cfg::PortQueue> queueConfig;
};
std::optional<QueueConfigAndName> getNameAndDefaultVoqCfg(
    cfg::PortType portType);

uint8_t getDefaultQueue();

uint8_t getGlobalRcyDefaultQueue();

int getTrafficClassToVoqId(const HwAsic* hwAsic, int trafficClass);

int getTrafficClassToCpuVoqId(const HwAsic* hwAsic, int trafficClass);

SwitchID getRemoteVoqSwitchId(SwSwitch* sw);

SystemPortID getFirstRemoteGlobalSystemPortId(const SwSwitch& sw);

SystemPortID getRemoteSysPortId(
    SwSwitch* sw,
    const std::shared_ptr<SwitchState>& state,
    int offset = 0);

SystemPortID getAvailableRemoteSysPortId(
    SwSwitch* sw,
    const std::shared_ptr<SwitchState>& state,
    int offset = 0);

InterfaceID getRemoteIntfId(SystemPortID sysPortId);

// When refreshStats is true, the helper first triggers a switch-wide
// updateStats() so callers reading immediately after programming a remote
// sysport see populated queue maps. This is expensive, so it defaults to
// false; only opt in when the cached stats may not yet be populated.
std::optional<HwSysPortStats> getRemoteSysPortStatsForSwitchUnderTest(
    SwSwitch* sw,
    const std::shared_ptr<SwitchState>& state,
    uint16_t switchIndex,
    SystemPortID sysPortId,
    bool refreshStats = false);

void addRemoteSysPortAndInterface(
    SwSwitch* sw,
    const SwitchID& remoteSwitchID,
    const SystemPortID& remoteSysPortId,
    const InterfaceID& remoteIntfId,
    const Interface::Addresses& intfAddrs);

void resolveRouteToRemoteSysPort(
    const std::shared_ptr<SwitchState>& state,
    SwSwitch* sw,
    TestEnsembleIf* ensemble,
    const SystemPortID& remoteSysPortId,
    const folly::IPAddressV6& dstIp);
} // namespace utility
} // namespace facebook::fboss
