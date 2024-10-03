/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/test/utils/VoqTestUtils.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/test/TestEnsembleIf.h"

namespace facebook::fboss::utility {

namespace {
constexpr auto kNumPortPerCore = 10;
// 0: CPU port, 1: gloabl rcy port, 2-5: local recycle port, 6: eventor port,
// 7: mgm port, 8-43 front panel nif
constexpr auto kRemoteSysPortOffset = 7;
constexpr auto kNumVoq = 8;

std::shared_ptr<SystemPort> makeRemoteSysPort(
    SystemPortID portId,
    SwitchID remoteSwitchId,
    int coreIndex,
    int corePortIndex,
    int64_t speed) {
  auto remoteSysPort = std::make_shared<SystemPort>(portId);
  auto voqConfig = getDefaultVoqConfig();
  remoteSysPort->setName(folly::to<std::string>(
      "hwTestSwitch", remoteSwitchId, ":eth/", portId, "/1"));
  remoteSysPort->setSwitchId(remoteSwitchId);
  remoteSysPort->setNumVoqs(kNumVoq);
  remoteSysPort->setCoreIndex(coreIndex);
  remoteSysPort->setCorePortIndex(corePortIndex);
  remoteSysPort->setSpeedMbps(speed);
  remoteSysPort->resetPortQueues(voqConfig);
  remoteSysPort->setScope(cfg::Scope::GLOBAL);
  return remoteSysPort;
}

std::shared_ptr<Interface> makeRemoteInterface(
    InterfaceID intfId,
    const Interface::Addresses& subnets) {
  auto remoteIntf = std::make_shared<Interface>(
      intfId,
      RouterID(0),
      std::optional<VlanID>(std::nullopt),
      folly::StringPiece("RemoteIntf"),
      folly::MacAddress("c6:ca:2b:2a:b1:b6"),
      9000,
      false,
      false,
      cfg::InterfaceType::SYSTEM_PORT);
  remoteIntf->setAddresses(subnets);
  remoteIntf->setScope(cfg::Scope::GLOBAL);
  return remoteIntf;
}

void updateRemoteIntfWithNeighbor(
    std::shared_ptr<Interface>& remoteIntf,
    InterfaceID intfId,
    PortDescriptor port,
    const folly::IPAddressV6& neighborIp,
    std::optional<int64_t> encapIndex) {
  const folly::MacAddress kNeighborMac{"2:3:4:5:6:7"};
  state::NeighborEntryFields ndp;
  auto ndpTable = remoteIntf->getNdpTable()->clone();
  ndp.mac() = kNeighborMac.toString();
  ndp.ipaddress() = neighborIp.str();
  ndp.portId() = port.toThrift();
  ndp.interfaceId() = static_cast<int>(intfId);
  ndp.state() = state::NeighborState::Reachable;
  if (encapIndex) {
    ndp.encapIndex() = *encapIndex;
  }
  ndp.isLocal() = false;
  ndpTable->emplace(neighborIp.str(), std::move(ndp));
  remoteIntf->setNdpTable(ndpTable->toThrift());
}
} // namespace

std::shared_ptr<SwitchState> addRemoteSysPort(
    std::shared_ptr<SwitchState> currState,
    const SwitchIdScopeResolver& scopeResolver,
    SystemPortID portId,
    SwitchID remoteSwitchId,
    int coreIndex,
    int corePortIndex) {
  auto newState = currState->clone();
  const auto& localPorts = newState->getSystemPorts()->cbegin()->second;
  auto localPort = localPorts->cbegin()->second;
  auto remoteSystemPorts = newState->getRemoteSystemPorts()->modify(&newState);
  auto remoteSysPort = makeRemoteSysPort(
      portId,
      remoteSwitchId,
      coreIndex,
      corePortIndex,
      localPort->getSpeedMbps());
  remoteSystemPorts->addNode(remoteSysPort, scopeResolver.scope(remoteSysPort));
  return newState;
}

std::shared_ptr<SwitchState> removeRemoteSysPort(
    std::shared_ptr<SwitchState> currState,
    SystemPortID portId) {
  auto newState = currState->clone();
  auto remoteSystemPorts = newState->getRemoteSystemPorts()->modify(&newState);
  remoteSystemPorts->removeNode(portId);
  return newState;
}

std::shared_ptr<SwitchState> addRemoteInterface(
    std::shared_ptr<SwitchState> currState,
    const SwitchIdScopeResolver& scopeResolver,
    InterfaceID intfId,
    const Interface::Addresses& subnets) {
  auto newState = currState;
  auto newRemoteInterfaces = newState->getRemoteInterfaces()->modify(&newState);
  auto newRemoteInterface = makeRemoteInterface(intfId, subnets);
  newRemoteInterfaces->addNode(
      newRemoteInterface, scopeResolver.scope(newRemoteInterface, newState));
  return newState;
}

std::shared_ptr<SwitchState> removeRemoteInterface(
    std::shared_ptr<SwitchState> currState,
    InterfaceID intfId) {
  auto newState = currState;
  auto newRemoteInterfaces = newState->getRemoteInterfaces()->modify(&newState);
  newRemoteInterfaces->removeNode(intfId);
  return newState;
}

std::shared_ptr<SwitchState> addRemoveRemoteNeighbor(
    std::shared_ptr<SwitchState> currState,
    const SwitchIdScopeResolver& scopeResolver,
    const folly::IPAddressV6& neighborIp,
    InterfaceID intfID,
    PortDescriptor port,
    bool add,
    std::optional<int64_t> encapIndex) {
  auto outState = currState;
  auto interfaceMap = outState->getRemoteInterfaces()->modify(&outState);
  auto interface = interfaceMap->getNode(intfID)->clone();
  auto ndpTable = interfaceMap->getNode(intfID)->getNdpTable()->clone();
  if (add) {
    const folly::MacAddress kNeighborMac{"2:3:4:5:6:7"};
    state::NeighborEntryFields ndp;
    ndp.mac() = kNeighborMac.toString();
    ndp.ipaddress() = neighborIp.str();
    ndp.portId() = port.toThrift();
    ndp.interfaceId() = static_cast<int>(intfID);
    ndp.state() = state::NeighborState::Reachable;
    if (encapIndex) {
      ndp.encapIndex() = *encapIndex;
    }
    ndp.isLocal() = false;
    ndpTable->emplace(neighborIp.str(), std::move(ndp));
  } else {
    ndpTable->remove(neighborIp.str());
  }
  interface->setNdpTable(ndpTable->toThrift());
  interfaceMap->updateNode(interface, scopeResolver.scope(interface, outState));
  return outState;
}

void populateRemoteIntfAndSysPorts(
    std::map<SwitchID, std::shared_ptr<SystemPortMap>>& switchId2SystemPorts,
    std::map<SwitchID, std::shared_ptr<InterfaceMap>>& switchId2Rifs,
    const cfg::SwitchConfig& config,
    bool useEncapIndex) {
  for (const auto& [remoteSwitchId, dsfNode] : *config.dsfNodes()) {
    if ((*config.switchSettings())
            .switchIdToSwitchInfo()
            ->contains(remoteSwitchId)) {
      continue;
    }
    std::shared_ptr<SystemPortMap> remoteSysPorts =
        std::make_shared<SystemPortMap>();
    std::shared_ptr<InterfaceMap> remoteRifs = std::make_shared<InterfaceMap>();
    CHECK(dsfNode.systemPortRange().has_value());
    const auto minPortID = *dsfNode.systemPortRange()->minimum();
    const auto maxPortID = *dsfNode.systemPortRange()->maximum();
    // 0th port for CPU and 1st port for recycle port
    for (int i = minPortID + kRemoteSysPortOffset; i <= maxPortID; i++) {
      const SystemPortID remoteSysPortId(i);
      const InterfaceID remoteIntfId(i);
      const PortDescriptor portDesc(remoteSysPortId);
      const std::optional<uint64_t> encapEndx =
          useEncapIndex ? std::optional<uint64_t>(0x200001 + i) : std::nullopt;

      // Use subnet 100+(dsfNodeId/256):(dsfNodeId%256):(localIntfId)::1/64
      // and 100+(dsfNodeId/256).(dsfNodeId%256).(localIntfId).1/24
      auto firstOctet = 100 + remoteSwitchId / 256;
      auto secondOctet = remoteSwitchId % 256;
      auto thirdOctet = i - minPortID;
      folly::IPAddressV6 neighborIp(folly::to<std::string>(
          firstOctet, ":", secondOctet, ":", thirdOctet, "::2"));
      auto remoteSysPort = makeRemoteSysPort(
          remoteSysPortId,
          SwitchID(remoteSwitchId),
          (i - minPortID - kRemoteSysPortOffset) / kNumPortPerCore,
          (i - minPortID) % kNumPortPerCore,
          static_cast<int64_t>(cfg::PortSpeed::FOURHUNDREDG));
      remoteSysPorts->addSystemPort(remoteSysPort);

      auto remoteRif = makeRemoteInterface(
          remoteIntfId,
          {
              {folly::IPAddress(folly::to<std::string>(
                   firstOctet, ":", secondOctet, ":", thirdOctet, "::1")),
               64},
              {folly::IPAddress(folly::to<std::string>(
                   firstOctet, ".", secondOctet, ".", thirdOctet, ".1")),
               24},
          });

      updateRemoteIntfWithNeighbor(
          remoteRif, remoteIntfId, portDesc, neighborIp, encapEndx);
      remoteRifs->addNode(remoteRif);
    }
    switchId2SystemPorts[SwitchID(remoteSwitchId)] = remoteSysPorts;
    switchId2Rifs[SwitchID(remoteSwitchId)] = remoteRifs;
  }
}

QueueConfig getDefaultVoqConfig() {
  QueueConfig queueCfg;

  auto defaultQueue = std::make_shared<PortQueue>(static_cast<uint8_t>(0));
  defaultQueue->setStreamType(cfg::StreamType::UNICAST);
  defaultQueue->setScheduling(cfg::QueueScheduling::INTERNAL);
  defaultQueue->setName("default");
  defaultQueue->setScalingFactor(cfg::MMUScalingFactor::ONE_32768TH);
  queueCfg.push_back(defaultQueue);

  auto rdmaQueue = std::make_shared<PortQueue>(static_cast<uint8_t>(2));
  rdmaQueue->setStreamType(cfg::StreamType::UNICAST);
  rdmaQueue->setScheduling(cfg::QueueScheduling::INTERNAL);
  rdmaQueue->setName("rdma");
  defaultQueue->setScalingFactor(cfg::MMUScalingFactor::ONE_32768TH);
  queueCfg.push_back(rdmaQueue);

  auto monitoringQueue = std::make_shared<PortQueue>(static_cast<uint8_t>(6));
  monitoringQueue->setStreamType(cfg::StreamType::UNICAST);
  monitoringQueue->setScheduling(cfg::QueueScheduling::INTERNAL);
  monitoringQueue->setName("monitoring");
  defaultQueue->setScalingFactor(cfg::MMUScalingFactor::ONE_32768TH);
  queueCfg.push_back(monitoringQueue);

  auto ncQueue = std::make_shared<PortQueue>(static_cast<uint8_t>(7));
  ncQueue->setStreamType(cfg::StreamType::UNICAST);
  ncQueue->setScheduling(cfg::QueueScheduling::INTERNAL);
  ncQueue->setName("nc");
  defaultQueue->setScalingFactor(cfg::MMUScalingFactor::ONE_32768TH);
  queueCfg.push_back(ncQueue);

  return queueCfg;
}

std::optional<uint64_t> getDummyEncapIndex(TestEnsembleIf* ensemble) {
  std::optional<uint64_t> dummyEncapIndex;
  if (ensemble->getHwAsicTable()->isFeatureSupportedOnAllAsic(
          HwAsic::Feature::RESERVED_ENCAP_INDEX_RANGE)) {
    dummyEncapIndex = 0x200001;
  }
  return dummyEncapIndex;
}

// Resolve and return list of remote nhops
boost::container::flat_set<PortDescriptor> resolveRemoteNhops(
    TestEnsembleIf* ensemble,
    utility::EcmpSetupTargetedPorts6& ecmpHelper) {
  auto remoteSysPorts =
      ensemble->getProgrammedState()->getRemoteSystemPorts()->getAllNodes();
  boost::container::flat_set<PortDescriptor> sysPortDescs;
  std::for_each(
      remoteSysPorts->begin(),
      remoteSysPorts->end(),
      [&sysPortDescs](const auto& idAndPort) {
        sysPortDescs.insert(
            PortDescriptor(static_cast<SystemPortID>(idAndPort.first)));
      });
  ensemble->applyNewState([&](const std::shared_ptr<SwitchState>& in) {
    return ecmpHelper.resolveNextHops(
        in, sysPortDescs, false, getDummyEncapIndex(ensemble));
  });
  return sysPortDescs;
}

} // namespace facebook::fboss::utility
