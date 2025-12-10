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
#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/DsfStateUpdaterUtil.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/VoqUtils.h"
#include "fboss/agent/platforms/common/meru800bia/Meru800biaPlatformMapping.h"
#include "fboss/agent/test/TestEnsembleIf.h"

namespace facebook::fboss::utility {

namespace {
constexpr auto kNumPortPerCore = 10;
// 0: CPU port, 1: gloabl rcy port, 2-5: local recycle port, 6: eventor port,
// 7: mgm port, 8-43 front panel nif
constexpr auto kRemoteSysPortOffset = 7;
constexpr auto kNumRdswSysPort = 44;
constexpr auto kNumEdswSysPort = 26;
// Prod range
constexpr auto kNumJangaSysPort = 22;
constexpr uint8_t kDefaultQueue = 0;
constexpr uint8_t kDualStage3Q2QDefaultQueue = 1;

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

std::vector<cfg::PortQueue> getDefaultNifVoqCfg() {
  std::vector<cfg::PortQueue> voqs;
  if (isDualStage3Q2QQos()) {
    cfg::PortQueue rdmaQueue;
    rdmaQueue.id() = 0;
    rdmaQueue.name() = "rdma";
    rdmaQueue.streamType() = cfg::StreamType::UNICAST;
    rdmaQueue.scheduling() = cfg::QueueScheduling::INTERNAL;
    voqs.push_back(rdmaQueue);

    cfg::PortQueue monitoringQueue;
    monitoringQueue.id() = 1;
    monitoringQueue.name() = "monitoring";
    monitoringQueue.streamType() = cfg::StreamType::UNICAST;
    monitoringQueue.scheduling() = cfg::QueueScheduling::INTERNAL;
    voqs.push_back(monitoringQueue);

    cfg::PortQueue ncQueue;
    ncQueue.id() = 2;
    ncQueue.name() = "nc";
    ncQueue.streamType() = cfg::StreamType::UNICAST;
    ncQueue.scheduling() = cfg::QueueScheduling::INTERNAL;
    voqs.push_back(ncQueue);
  } else {
    cfg::PortQueue defaultQueue;
    defaultQueue.id() = 0;
    defaultQueue.name() = "default";
    defaultQueue.streamType() = cfg::StreamType::UNICAST;
    defaultQueue.scheduling() = cfg::QueueScheduling::INTERNAL;
    voqs.push_back(defaultQueue);

    cfg::PortQueue rdmaQueue;
    rdmaQueue.id() = 2;
    rdmaQueue.name() = "rdma";
    rdmaQueue.streamType() = cfg::StreamType::UNICAST;
    rdmaQueue.scheduling() = cfg::QueueScheduling::INTERNAL;
    voqs.push_back(rdmaQueue);

    cfg::PortQueue monitoringQueue;
    monitoringQueue.id() = 6;
    monitoringQueue.name() = "monitoring";
    monitoringQueue.streamType() = cfg::StreamType::UNICAST;
    monitoringQueue.scheduling() = cfg::QueueScheduling::INTERNAL;
    voqs.push_back(monitoringQueue);

    cfg::PortQueue ncQueue;
    ncQueue.id() = 7;
    ncQueue.name() = "nc";
    ncQueue.streamType() = cfg::StreamType::UNICAST;
    ncQueue.scheduling() = cfg::QueueScheduling::INTERNAL;
    voqs.push_back(ncQueue);
  }
  return voqs;
}

std::vector<cfg::PortQueue> get2VoqCfg() {
  std::vector<cfg::PortQueue> voqs;
  cfg::PortQueue lowQueue;
  lowQueue.id() = 0;
  lowQueue.name() = "low";
  lowQueue.streamType() = cfg::StreamType::UNICAST;
  lowQueue.scheduling() = cfg::QueueScheduling::INTERNAL;
  voqs.push_back(lowQueue);

  cfg::PortQueue highQueue;
  highQueue.id() = 1;
  highQueue.name() = "high";
  highQueue.streamType() = cfg::StreamType::UNICAST;
  highQueue.scheduling() = cfg::QueueScheduling::INTERNAL;
  voqs.push_back(highQueue);
  return voqs;
}

std::vector<cfg::PortQueue> get3VoqCfg() {
  std::vector<cfg::PortQueue> voqs;
  cfg::PortQueue lowQueue;
  lowQueue.id() = 0;
  lowQueue.name() = "low";
  lowQueue.streamType() = cfg::StreamType::UNICAST;
  lowQueue.scheduling() = cfg::QueueScheduling::INTERNAL;
  voqs.push_back(lowQueue);

  cfg::PortQueue midQueue;
  midQueue.id() = 1;
  midQueue.name() = "mid";
  midQueue.streamType() = cfg::StreamType::UNICAST;
  midQueue.scheduling() = cfg::QueueScheduling::INTERNAL;
  voqs.push_back(midQueue);

  cfg::PortQueue highQueue;
  highQueue.id() = 2;
  highQueue.name() = "high";
  highQueue.streamType() = cfg::StreamType::UNICAST;
  highQueue.scheduling() = cfg::QueueScheduling::INTERNAL;
  voqs.push_back(highQueue);
  return voqs;
}

std::shared_ptr<PortQueue> makeSwitchStateVoq(const cfg::PortQueue& cfgQueue) {
  auto queue =
      std::make_shared<PortQueue>(static_cast<uint8_t>(cfgQueue.id().value()));
  queue->setStreamType(cfgQueue.streamType().value());
  queue->setScheduling(cfgQueue.scheduling().value());
  queue->setName(cfgQueue.name().value());
  queue->setScalingFactor(cfg::MMUScalingFactor::ONE_32768TH);
  return queue;
}
} // namespace

std::shared_ptr<SystemPort> makeRemoteSysPort(
    SystemPortID portId,
    SwitchID remoteSwitchId,
    int coreIndex,
    int corePortIndex,
    int64_t speed,
    HwAsic::InterfaceNodeRole intfRole,
    cfg::PortType portType,
    std::string remoteSwitchName) {
  if (remoteSwitchName.empty()) {
    remoteSwitchName = folly::to<std::string>("hwTestSwitch", remoteSwitchId);
  }
  auto remoteSysPort = std::make_shared<SystemPort>(portId);
  auto voqConfig = getDefaultVoqConfig(portType);
  remoteSysPort->setName(
      folly::to<std::string>(remoteSwitchName, ":eth1/", portId, "/1"));
  remoteSysPort->setSwitchId(remoteSwitchId);
  remoteSysPort->setNumVoqs(getRemotePortNumVoqs(intfRole, portType));
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

std::shared_ptr<SwitchState> addRemoteSysPort(
    std::shared_ptr<SwitchState> currState,
    const SwitchIdScopeResolver& scopeResolver,
    SystemPortID portId,
    SwitchID remoteSwitchId,
    int coreIndex,
    int corePortIndex,
    HwAsic::InterfaceNodeRole intfRole,
    cfg::PortType portType) {
  auto newState = currState->clone();
  const auto& localPorts = newState->getSystemPorts()->cbegin()->second;
  auto localPort = localPorts->cbegin()->second;
  auto remoteSystemPorts = newState->getRemoteSystemPorts()->modify(&newState);
  auto remoteSysPort = makeRemoteSysPort(
      portId,
      remoteSwitchId,
      coreIndex,
      corePortIndex,
      localPort->getSpeedMbps(),
      intfRole,
      portType);
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
    bool useEncapIndex,
    bool addNeighborToIntf) {
  for (const auto& [remoteSwitchId, remoteDsfNode] : *config.dsfNodes()) {
    if ((*config.switchSettings())
            .switchIdToSwitchInfo()
            ->contains(remoteSwitchId)) {
      continue;
    }
    std::shared_ptr<SystemPortMap> remoteSysPorts =
        std::make_shared<SystemPortMap>();
    std::shared_ptr<InterfaceMap> remoteRifs = std::make_shared<InterfaceMap>();
    CHECK(!remoteDsfNode.systemPortRanges()->systemPortRanges()->empty());

    auto addSingleStageSysPort = [&remoteSysPorts, &remoteRifs, useEncapIndex](
                                     const auto& dsfNode,
                                     const auto& switchId) {
      for (auto sysPortRange :
           *dsfNode.systemPortRanges()->systemPortRanges()) {
        const auto minPortID = *sysPortRange.minimum();
        const auto maxPortID = *sysPortRange.maximum();
        const auto numPorts = maxPortID - minPortID + 1;
        CHECK(
            numPorts == kNumRdswSysPort || numPorts == kNumEdswSysPort ||
            numPorts == kNumJangaSysPort);
        for (int i = minPortID + kRemoteSysPortOffset; i <= maxPortID; i++) {
          const SystemPortID remoteSysPortId(i);
          const InterfaceID remoteIntfId(i);
          const PortDescriptor portDesc(remoteSysPortId);
          const std::optional<uint64_t> encapEndx = useEncapIndex
              ? std::optional<uint64_t>(0x200001 + i)
              : std::nullopt;

          // Use subnet 100+(dsfNodeId/256):(dsfNodeId%256):(localIntfId)::1/64
          // and 100+(dsfNodeId/256).(dsfNodeId%256).(localIntfId).1/24
          auto firstOctet = 100 + switchId / 256;
          auto secondOctet = switchId % 256;
          auto thirdOctet = i - minPortID;
          folly::IPAddressV6 neighborIp(
              folly::to<std::string>(
                  firstOctet, ":", secondOctet, ":", thirdOctet, "::2"));
          auto portSpeed = i == minPortID + kRemoteSysPortOffset
              ? cfg::PortSpeed::HUNDREDG
              : numPorts == kNumRdswSysPort ? cfg::PortSpeed::FOURHUNDREDG
                                            : cfg::PortSpeed::EIGHTHUNDREDG;
          auto remoteSysPort = makeRemoteSysPort(
              remoteSysPortId,
              SwitchID(switchId),
              (i - minPortID - kRemoteSysPortOffset) / kNumPortPerCore,
              (i - minPortID) % kNumPortPerCore,
              static_cast<int64_t>(portSpeed),
              HwAsic::InterfaceNodeRole::IN_CLUSTER_NODE,
              cfg::PortType::INTERFACE_PORT,
              *dsfNode.name());
          remoteSysPorts->addSystemPort(remoteSysPort);

          auto remoteRif = makeRemoteInterface(
              remoteIntfId,
              {
                  {folly::IPAddress(
                       folly::to<std::string>(
                           firstOctet,
                           ":",
                           secondOctet,
                           ":",
                           thirdOctet,
                           "::1")),
                   64},
                  {folly::IPAddress(
                       folly::to<std::string>(
                           firstOctet,
                           ".",
                           secondOctet,
                           ".",
                           thirdOctet,
                           ".1")),
                   24},
              });

          updateRemoteIntfWithNeighbor(
              remoteRif, remoteIntfId, portDesc, neighborIp, encapEndx);
          remoteRifs->addNode(remoteRif);
        }
      }
    };

    auto addDualStageSysPort = [&remoteSysPorts,
                                &remoteRifs,
                                useEncapIndex,
                                addNeighborToIntf](
                                   const auto& dsfNode, const auto& switchId) {
      CHECK_EQ(dsfNode.systemPortRanges()->systemPortRanges()->size(), 2);
      // TODO(zecheng): Initialize correct global offset for DsfNode
      const auto globalOffset = dsfNode.systemPortRanges()
                                    ->systemPortRanges()
                                    ->at(0)
                                    .minimum()
                                    .value() -
          1;
      static const auto rdswPlatformMapping =
          Meru800biaPlatformMapping(HwAsic::InterfaceNodeRole::IN_CLUSTER_NODE);
      static const auto edswPlatformMapping = Meru800biaPlatformMapping(
          HwAsic::InterfaceNodeRole::DUAL_STAGE_EDGE_NODE);
      const auto& platformMapping =
          *dsfNode.clusterId() >= k2StageEdgePodClusterId ? edswPlatformMapping
                                                          : rdswPlatformMapping;

      for (const auto& [portID, platformPort] :
           platformMapping.getPlatformPorts()) {
        auto mapping = *platformPort.mapping();
        if (*mapping.scope() == cfg::Scope::GLOBAL &&
            (*mapping.portType() == cfg::PortType::INTERFACE_PORT ||
             *mapping.portType() == cfg::PortType::MANAGEMENT_PORT)) {
          const auto remoteSysPortId =
              static_cast<SystemPortID>(portID + globalOffset);
          const auto remoteIntfId = static_cast<InterfaceID>(remoteSysPortId);
          const PortDescriptor portDesc(remoteSysPortId);
          const std::optional<uint64_t> encapEndx = useEncapIndex
              ? std::optional<uint64_t>(0x200001 + portID)
              : std::nullopt;

          // Use subnet
          // 100+(dsfNodeId/256):(dsfNodeId%256):(localIntfId%256)::1/64 and
          // 100+(dsfNodeId/256).(dsfNodeId%256).(localIntfId%256).1/24
          auto firstOctet = 100 + switchId / 256;
          auto secondOctet = switchId % 256;
          // For >16K ports, use the second half of 128 range in the octet.
          auto thirdOctet = portID < 256 ? portID : (portID + 128) % 256;
          folly::IPAddressV6 neighborIp(
              folly::to<std::string>(
                  firstOctet, ":", secondOctet, ":", thirdOctet, "::2"));
          auto portSpeed = *mapping.portType() == cfg::PortType::MANAGEMENT_PORT
              ? cfg::PortSpeed::HUNDREDG
              : (*dsfNode.clusterId() >= k2StageEdgePodClusterId
                     ? cfg::PortSpeed::EIGHTHUNDREDG
                     : cfg::PortSpeed::FOURHUNDREDG);
          auto remoteSysPort = makeRemoteSysPort(
              remoteSysPortId,
              SwitchID(switchId),
              *mapping.attachedCoreId(),
              *mapping.attachedCorePortIndex(),
              static_cast<int64_t>(portSpeed),
              *dsfNode.clusterId() >= k2StageEdgePodClusterId
                  ? HwAsic::InterfaceNodeRole::DUAL_STAGE_EDGE_NODE
                  : HwAsic::InterfaceNodeRole::IN_CLUSTER_NODE,
              *mapping.portType(),
              *dsfNode.name());
          remoteSysPorts->addSystemPort(remoteSysPort);

          auto remoteRif = makeRemoteInterface(
              remoteIntfId,
              {
                  {folly::IPAddress(
                       folly::to<std::string>(
                           firstOctet,
                           ":",
                           secondOctet,
                           ":",
                           thirdOctet,
                           "::1")),
                   64},
                  {folly::IPAddress(
                       folly::to<std::string>(
                           firstOctet,
                           ".",
                           secondOctet,
                           ".",
                           thirdOctet,
                           ".1")),
                   24},
              });

          if (addNeighborToIntf) {
            updateRemoteIntfWithNeighbor(
                remoteRif, remoteIntfId, portDesc, neighborIp, encapEndx);
          }
          remoteRifs->addNode(remoteRif);
        }
      }
    };

    if (isDualStage3Q2QMode()) {
      addDualStageSysPort(remoteDsfNode, remoteSwitchId);
    } else {
      addSingleStageSysPort(remoteDsfNode, remoteSwitchId);
    }

    switchId2SystemPorts[SwitchID(remoteSwitchId)] = remoteSysPorts;
    switchId2Rifs[SwitchID(remoteSwitchId)] = remoteRifs;
  }
}

QueueConfig getDefaultVoqConfig(cfg::PortType portType) {
  QueueConfig queueCfg;
  auto nameAndDefaultVoq = getNameAndDefaultVoqCfg(portType);
  CHECK(nameAndDefaultVoq);
  for (const auto& cfgQueue : nameAndDefaultVoq.value().queueConfig) {
    queueCfg.push_back(makeSwitchStateVoq(cfgQueue));
  }
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

boost::container::flat_set<PortDescriptor> getRemoteSysPorts(
    TestEnsembleIf* ensemble) {
  auto remoteSysPorts =
      ensemble->getProgrammedState()->getRemoteSystemPorts()->getAllNodes();
  boost::container::flat_set<PortDescriptor> sysPortDescs;
  std::for_each(
      remoteSysPorts->begin(),
      remoteSysPorts->end(),
      [&sysPortDescs](const auto& idAndPort) {
        if (!idAndPort.second->isStatic()) {
          sysPortDescs.insert(
              PortDescriptor(static_cast<SystemPortID>(idAndPort.first)));
        }
      });
  return sysPortDescs;
}

// Resolve and return list of remote nhops
boost::container::flat_set<PortDescriptor> resolveRemoteNhops(
    TestEnsembleIf* ensemble,
    utility::EcmpSetupTargetedPorts6& ecmpHelper) {
  auto remoteSysPorts =
      ensemble->getProgrammedState()->getRemoteSystemPorts()->getAllNodes();
  auto sysPortDescs = getRemoteSysPorts(ensemble);
  ensemble->applyNewState([&](const std::shared_ptr<SwitchState>& in) {
    return ecmpHelper.resolveNextHops(
        in, sysPortDescs, false, getDummyEncapIndex(ensemble));
  });
  return sysPortDescs;
}

boost::container::flat_set<PortDescriptor> unresolveRemoteNhops(
    TestEnsembleIf* ensemble,
    utility::EcmpSetupTargetedPorts6& ecmpHelper) {
  auto remoteSysPorts =
      ensemble->getProgrammedState()->getRemoteSystemPorts()->getAllNodes();
  auto sysPortDescs = getRemoteSysPorts(ensemble);
  ensemble->applyNewState([&](const std::shared_ptr<SwitchState>& in) {
    return ecmpHelper.unresolveNextHops(in, sysPortDescs, false);
  });
  return sysPortDescs;
}

void setupRemoteIntfAndSysPorts(SwSwitch* swSwitch, bool useEncapIndex) {
  auto updateDsfStateFn =
      [swSwitch, useEncapIndex](const std::shared_ptr<SwitchState>& in) {
        std::map<SwitchID, std::shared_ptr<SystemPortMap>> switchId2SystemPorts;
        std::map<SwitchID, std::shared_ptr<InterfaceMap>> switchId2Rifs;
        utility::populateRemoteIntfAndSysPorts(
            switchId2SystemPorts,
            switchId2Rifs,
            swSwitch->getConfig(),
            useEncapIndex);
        return DsfStateUpdaterUtil::getUpdatedState(
            in,
            swSwitch->getScopeResolver(),
            swSwitch->getRib(),
            switchId2SystemPorts,
            switchId2Rifs);
      };
  swSwitch->getRib()->updateStateInRibThread([swSwitch, updateDsfStateFn]() {
    swSwitch->updateStateWithHwFailureProtection(
        folly::sformat("Update state for node: {}", 0), updateDsfStateFn);
  });
}

std::optional<QueueConfigAndName> getNameAndDefaultVoqCfg(
    cfg::PortType portType) {
  switch (portType) {
    case cfg::PortType::INTERFACE_PORT:
    case cfg::PortType::HYPER_PORT_MEMBER:
      return QueueConfigAndName{"defaultVoqCofig", getDefaultNifVoqCfg()};
    case cfg::PortType::CPU_PORT:
      if (isDualStage3Q2QMode()) {
        return QueueConfigAndName{"3VoqConfig", get3VoqCfg()};
      }
      break;
    case cfg::PortType::MANAGEMENT_PORT:
    case cfg::PortType::RECYCLE_PORT:
    case cfg::PortType::EVENTOR_PORT:
    case cfg::PortType::HYPER_PORT:
      if (isDualStage3Q2QMode()) {
        return QueueConfigAndName{"2VoqConfig", get2VoqCfg()};
      }
      break;
    case cfg::PortType::FABRIC_PORT:
      XLOG(FATAL) << " No VOQ configs for fabric ports";
      break;
  }
  return std::nullopt;
}

uint8_t getDefaultQueue() {
  return isDualStage3Q2QMode() ? kDualStage3Q2QDefaultQueue : kDefaultQueue;
}

uint8_t getGlobalRcyDefaultQueue() {
  // For both single stage and dual stage, packet destined to Gloabl RCY goes to
  // queue 0 by default. In dual stage we follow 2Q model, where non RDMA and
  // non NC goes to queue zero.
  return kDefaultQueue;
}

int getTrafficClassToVoqId(const HwAsic* hwAsic, int trafficClass) {
  if (hwAsic->isSupported(HwAsic::Feature::VOQ) && isDualStage3Q2QQos()) {
    if (trafficClass == 7) {
      // tc 7 -> nc voq 2
      return 2;
    } else if (trafficClass == 2) {
      // tc 2 -> rdma voq 0
      return 0;
    }
    // all other tc -> default voq 1
    return 1;
  }
  return trafficClass;
}

int getTrafficClassToCpuVoqId(const HwAsic* hwAsic, int trafficClass) {
  if (hwAsic->isSupported(HwAsic::Feature::VOQ) && isDualStage3Q2QQos()) {
    if (trafficClass == 7) {
      // tc 7 -> high queue 2
      return 2;
    } else if (trafficClass == 3) {
      // tc 3 -> mid queue 1
      return 1;
    }
    // all other tc -> low queue 0
    return 0;
  }
  return trafficClass;
}

} // namespace facebook::fboss::utility
