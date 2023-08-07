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

namespace {
constexpr auto kSystemPortCountPerNode = 15;
}

namespace facebook::fboss::utility {

std::shared_ptr<SwitchState> addRemoteSysPort(
    std::shared_ptr<SwitchState> currState,
    const SwitchIdScopeResolver& scopeResolver,
    SystemPortID portId,
    SwitchID remoteSwitchId) {
  auto newState = currState->clone();
  const auto& localPorts = newState->getSystemPorts()->cbegin()->second;
  auto localPort = localPorts->cbegin()->second;
  auto remoteSystemPorts = newState->getRemoteSystemPorts()->modify(&newState);
  auto remoteSysPort = std::make_shared<SystemPort>(portId);
  remoteSysPort->setSwitchId(remoteSwitchId);
  remoteSysPort->setNumVoqs(localPort->getNumVoqs());
  remoteSysPort->setCoreIndex(localPort->getCoreIndex());
  remoteSysPort->setCorePortIndex(localPort->getCorePortIndex());
  remoteSysPort->setSpeedMbps(localPort->getSpeedMbps());
  remoteSysPort->setEnabled(true);
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
  return currState;
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
    ndp.isLocal() = encapIndex == std::nullopt;
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
    const cfg::SwitchConfig& config) {
  auto newState = currState->clone();
  for (const auto& [remoteSwitchId, dsfNode] : *config.dsfNodes()) {
    if (remoteSwitchId == 0) {
      continue;
    }
    CHECK(dsfNode.systemPortRange().has_value());
    const auto minPortID = *dsfNode.systemPortRange()->minimum();
    // 0th port for CPU and 1st port for recycle port
    for (int i = 2; i < kSystemPortCountPerNode; i++) {
      const auto newSysPortId = minPortID + i;
      const SystemPortID remoteSysPortId(newSysPortId);
      const InterfaceID remoteIntfId(newSysPortId);
      const PortDescriptor portDesc(remoteSysPortId);
      const uint64_t encapEndx = newSysPortId;

      // Use subnet 100:(dsfNodeId):(localIntfId)::1/64
      // and 100.(dsfNodeId).(localIntfId).1/24
      folly::IPAddressV6 neighborIp(
          folly::to<std::string>("100:", remoteSwitchId, ":", i, "::2"));

      newState = addRemoteSysPort(
          newState, scopeResolver, remoteSysPortId, SwitchID(remoteSwitchId));
      newState = addRemoteInterface(
          newState,
          scopeResolver,
          remoteIntfId,
          {
              {folly::IPAddress(folly::to<std::string>(
                   "100:", remoteSwitchId, ":", i, "::1")),
               64},
              {folly::IPAddress(folly::to<std::string>(
                   "100.", remoteSwitchId, ".", i, ".1")),
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
