/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/test/HwVoqUtils.h"
#include "fboss/agent/hw/test/ConfigFactory.h"

namespace facebook::fboss::utility {

namespace {

constexpr auto kNumPortPerCore = 10;
constexpr auto kNifPortOffset = 2;

int getPerNodeSysPorts(cfg::AsicType asicType) {
  return asicType == cfg::AsicType::ASIC_TYPE_JERICHO2 ? 20 : 40;
}
int getPerNodeSysPorts(const HwAsic* asic) {
  return getPerNodeSysPorts(asic->getAsicType());
}
} // namespace

int getDsfNodeCount(const HwAsic* asic) {
  // J3 supports up to 6k system port. For each node with 40 system ports,
  // 148 remote nodes gives (148 + 1) * 40 = 5960 system ports.
  return asic->getAsicType() == cfg::AsicType::ASIC_TYPE_JERICHO2 ? 128 : 148;
}

std::optional<std::map<int64_t, cfg::DsfNode>> addRemoteDsfNodeCfg(
    const std::map<int64_t, cfg::DsfNode>& curDsfNodes,
    std::optional<int> numRemoteNodes) {
  CHECK(!curDsfNodes.empty());
  auto dsfNodes = curDsfNodes;
  const auto& firstDsfNode = dsfNodes.begin()->second;
  CHECK(firstDsfNode.systemPortRange().has_value());
  CHECK(firstDsfNode.nodeMac().has_value());
  folly::MacAddress mac(*firstDsfNode.nodeMac());
  auto asic = HwAsic::makeAsic(
      *firstDsfNode.asicType(),
      cfg::SwitchType::VOQ,
      *firstDsfNode.switchId(),
      0,
      *firstDsfNode.systemPortRange(),
      mac,
      std::nullopt);
  int numCores = asic->getNumCores();
  CHECK(
      !numRemoteNodes.has_value() ||
      numRemoteNodes.value() < getDsfNodeCount(asic.get()));
  int totalNodes = numRemoteNodes.has_value() ? numRemoteNodes.value() + 1
                                              : getDsfNodeCount(asic.get());
  for (int remoteSwitchId = numCores; remoteSwitchId < totalNodes * numCores;
       remoteSwitchId += numCores) {
    const auto systemPortMin =
        (remoteSwitchId / numCores) * getPerNodeSysPorts(asic.get());
    cfg::Range64 systemPortRange;
    systemPortRange.minimum() = systemPortMin;
    systemPortRange.maximum() =
        systemPortMin + getPerNodeSysPorts(asic.get()) - 1;
    auto remoteDsfNodeCfg =
        dsfNodeConfig(*asic, SwitchID(remoteSwitchId), systemPortMin);
    dsfNodes.insert({remoteSwitchId, remoteDsfNodeCfg});
  }
  return dsfNodes;
}

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
  auto remoteSysPort = std::make_shared<SystemPort>(portId);
  remoteSysPort->setPortName(folly::to<std::string>(
      "hwTestSwitch", remoteSwitchId, ":eth/", portId, "/1"));
  remoteSysPort->setSwitchId(remoteSwitchId);
  remoteSysPort->setNumVoqs(localPort->getNumVoqs());
  remoteSysPort->setCoreIndex(coreIndex);
  remoteSysPort->setCorePortIndex(corePortIndex);
  remoteSysPort->setSpeedMbps(localPort->getSpeedMbps());
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
  auto newRemoteInterface = std::make_shared<Interface>(
      intfId,
      RouterID(0),
      std::optional<VlanID>(std::nullopt),
      folly::StringPiece("RemoteIntf"),
      folly::MacAddress("c6:ca:2b:2a:b1:b6"),
      9000,
      false,
      false,
      cfg::InterfaceType::SYSTEM_PORT);
  newRemoteInterface->setAddresses(subnets);
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

std::shared_ptr<SwitchState> setupRemoteIntfAndSysPorts(
    std::shared_ptr<SwitchState> currState,
    const SwitchIdScopeResolver& scopeResolver,
    const cfg::SwitchConfig& config,
    bool useEncapIndex) {
  auto newState = currState->clone();
  for (const auto& [remoteSwitchId, dsfNode] : *config.dsfNodes()) {
    if (remoteSwitchId == 0) {
      continue;
    }
    CHECK(dsfNode.systemPortRange().has_value());
    const auto minPortID = *dsfNode.systemPortRange()->minimum();
    // 0th port for CPU and 1st port for recycle port
    for (int i = kNifPortOffset; i < getPerNodeSysPorts(*dsfNode.asicType());
         i++) {
      const auto newSysPortId = minPortID + i;
      const SystemPortID remoteSysPortId(newSysPortId);
      const InterfaceID remoteIntfId(newSysPortId);
      const PortDescriptor portDesc(remoteSysPortId);
      const std::optional<uint64_t> encapEndx =
          useEncapIndex ? std::optional<uint64_t>(0x200001 + i) : std::nullopt;

      // Use subnet 100+(dsfNodeId/256):(dsfNodeId%256):(localIntfId)::1/64
      // and 100+(dsfNodeId/256).(dsfNodeId%256).(localIntfId).1/24
      auto firstOctet = 100 + remoteSwitchId / 256;
      auto secondOctet = remoteSwitchId % 256;
      folly::IPAddressV6 neighborIp(
          folly::to<std::string>(firstOctet, ":", secondOctet, ":", i, "::2"));

      newState = addRemoteSysPort(
          newState,
          scopeResolver,
          remoteSysPortId,
          SwitchID(remoteSwitchId),
          (i - kNifPortOffset) / kNumPortPerCore,
          (i - kNifPortOffset) % kNumPortPerCore + kNifPortOffset);
      newState = addRemoteInterface(
          newState,
          scopeResolver,
          remoteIntfId,
          {
              {folly::IPAddress(folly::to<std::string>(
                   firstOctet, ":", secondOctet, ":", i, "::1")),
               64},
              {folly::IPAddress(folly::to<std::string>(
                   firstOctet, ".", secondOctet, ".", i, ".1")),
               24},
          });
      newState = addRemoveRemoteNeighbor(
          newState,
          scopeResolver,
          neighborIp,
          remoteIntfId,
          portDesc,
          true /* add */,
          encapEndx);
    }
  }
  return newState;
}

} // namespace facebook::fboss::utility
