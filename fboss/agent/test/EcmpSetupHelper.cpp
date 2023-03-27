/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/test/EcmpSetupHelper.h"

#include <iterator>

#include "fboss/agent/ApplyThriftConfig.h"
#include "fboss/agent/RouteUpdateWrapper.h"
#include "fboss/agent/state/AggregatePort.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/PortDescriptor.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/Vlan.h"
#include "fboss/agent/types.h"

#include <folly/IPAddress.h>

using boost::container::flat_map;
using boost::container::flat_set;
using folly::IPAddress;
using folly::IPAddressV4;
using folly::IPAddressV6;
using folly::MacAddress;

namespace {

using namespace facebook::fboss;

std::optional<AggregatePortID> getAggPortID(
    const std::shared_ptr<SwitchState>& inputState,
    const PortID& portId) {
  for (auto idAndAggPort : std::as_const(*inputState->getAggregatePorts())) {
    if (idAndAggPort.second->isMemberPort(portId)) {
      return idAndAggPort.second->getID();
    }
  }
  return std::nullopt;
}

flat_map<InterfaceID, folly::CIDRNetwork> computeInterface2Subnet(
    const std::shared_ptr<SwitchState>& inputState,
    bool v6) {
  boost::container::flat_map<InterfaceID, folly::CIDRNetwork> intf2Network;
  for (auto iter : std::as_const(*inputState->getInterfaces())) {
    const auto& intf = iter.second;
    for (const auto& cidrStr : intf->getAddressesCopy()) {
      auto subnet = folly::IPAddress::createNetwork(cidrStr.first.str());
      if (!v6 && subnet.first.isV4()) {
        intf2Network[intf->getID()] = subnet;
      } else if (v6 && subnet.first.isV6() && !subnet.first.isLinkLocal()) {
        intf2Network[intf->getID()] = subnet;
      }
    }
  }
  return intf2Network;
}
} // namespace

namespace facebook::fboss::utility {

boost::container::flat_set<PortDescriptor> getPortsWithExclusiveVlanMembership(
    const std::shared_ptr<SwitchState>& state) {
  boost::container::flat_set<PortDescriptor> ports;
  for (auto [id, vlan] : std::as_const(*state->getVlans())) {
    auto memberPorts = vlan->getPorts();
    if (memberPorts.size() == 1) {
      ports.insert(PortDescriptor{PortID(memberPorts.begin()->first)});
    }
  }
  return ports;
}

template <typename AddrT, typename NextHopT>
BaseEcmpSetupHelper<AddrT, NextHopT>::BaseEcmpSetupHelper() {}

template <typename AddrT, typename NextHopT>
flat_map<PortDescriptor, InterfaceID>
BaseEcmpSetupHelper<AddrT, NextHopT>::computePortDesc2Interface(
    const std::shared_ptr<SwitchState>& inputState) const {
  boost::container::flat_map<PortDescriptor, InterfaceID> portDesc2Interface;
  flat_set<PortID> portIds;
  for (const auto& port : std::as_const(*inputState->getPorts())) {
    if (port.second->getPortType() == cfg::PortType::INTERFACE_PORT) {
      portIds.insert(port.second->getID());
    }
  }
  for (const auto& portId : portIds) {
    PortDescriptor portDesc = PortDescriptor(portId);
    auto aggId = getAggPortID(inputState, portId);
    if (aggId) {
      portDesc = PortDescriptor(*aggId);
    }

    if (auto intf = getInterface(portDesc, inputState)) {
      portDesc2Interface.insert(std::make_pair(portDesc, *intf));
    }
  }
  return portDesc2Interface;
}

template <typename AddrT, typename NextHopT>
std::shared_ptr<SwitchState>
BaseEcmpSetupHelper<AddrT, NextHopT>::resolveNextHops(
    const std::shared_ptr<SwitchState>& inputState,
    const boost::container::flat_set<PortDescriptor>& portDescs,
    bool useLinkLocal,
    std::optional<int64_t> encapIdx) const {
  return resolveNextHopsImpl(
      inputState, portDescs, true, useLinkLocal, encapIdx);
}

template <typename AddrT, typename NextHopT>
std::shared_ptr<SwitchState>
BaseEcmpSetupHelper<AddrT, NextHopT>::unresolveNextHops(
    const std::shared_ptr<SwitchState>& inputState,
    const boost::container::flat_set<PortDescriptor>& portDescs,
    bool useLinkLocal) const {
  return resolveNextHopsImpl(
      inputState, portDescs, false, useLinkLocal, std::nullopt);
}

template <typename AddrT, typename NextHopT>
std::shared_ptr<SwitchState>
BaseEcmpSetupHelper<AddrT, NextHopT>::resolveNextHopsImpl(
    const std::shared_ptr<SwitchState>& inputState,
    const boost::container::flat_set<PortDescriptor>& portDescs,
    bool resolve,
    bool useLinkLocal,
    std::optional<int64_t> encapIdx) const {
  auto outputState{inputState};
  for (auto nhop : nhops_) {
    if (portDescs.find(nhop.portDesc) != portDescs.end()) {
      outputState = resolve
          ? resolveNextHop(outputState, nhop, useLinkLocal, encapIdx)
          : unresolveNextHop(outputState, nhop, useLinkLocal);
    }
  }
  return outputState;
}

template <typename AddrT, typename NextHopT>
std::shared_ptr<SwitchState>
BaseEcmpSetupHelper<AddrT, NextHopT>::resolveVlanRifNextHop(
    const std::shared_ptr<SwitchState>& inputState,
    const NextHopT& nhop,
    const std::shared_ptr<Interface>& intf,
    bool useLinkLocal) const {
  auto outputState{inputState->clone()};
  auto vlan = outputState->getVlans()->getVlan(intf->getVlanID());
  auto nbrTable = vlan->template getNeighborEntryTable<AddrT>()->modify(
      vlan->getID(), &outputState);
  auto nhopIp = useLinkLocal ? nhop.linkLocalNhopIp.value() : nhop.ip;
  if (nbrTable->getEntryIf(nhop.ip)) {
    nbrTable->updateEntry(
        nhopIp,
        nhop.mac,
        nhop.portDesc,
        vlan->getInterfaceID(),
        NeighborState::REACHABLE);
  } else {
    nbrTable->addEntry(nhopIp, nhop.mac, nhop.portDesc, vlan->getInterfaceID());
  }
  return outputState;
}

template <typename AddrT, typename NextHopT>
std::shared_ptr<SwitchState>
BaseEcmpSetupHelper<AddrT, NextHopT>::resolvePortRifNextHop(
    const std::shared_ptr<SwitchState>& inputState,
    const NextHopT& nhop,
    const std::shared_ptr<Interface>& intf,
    bool useLinkLocal,
    std::optional<int64_t> encapIdx) const {
  auto outputState{inputState->clone()};
  auto nbrTable = intf->getNeighborEntryTable<AddrT>()->toThrift();
  auto nhopIp = useLinkLocal ? nhop.linkLocalNhopIp.value() : nhop.ip;
  state::NeighborEntryFields nbr;
  nbr.mac() = nhop.mac.toString();
  nbr.interfaceId() = static_cast<int>(intf->getID());
  nbr.ipaddress() = nhopIp.str();
  nbr.portId() = nhop.portDesc.toThrift();
  nbr.state() = state::NeighborState::Reachable;
  if (encapIdx) {
    nbr.encapIndex() = *encapIdx;
  }
  nbrTable.insert({*nbr.ipaddress(), nbr});
  auto interfaceMap = outputState->getInterfaces()->modify(&outputState);
  auto interface = interfaceMap->getInterface(intf->getID())->clone();
  interface->setNeighborEntryTable<AddrT>(nbrTable);
  interfaceMap->updateNode(interface);
  return outputState;
}

template <typename AddrT, typename NextHopT>
std::shared_ptr<SwitchState>
BaseEcmpSetupHelper<AddrT, NextHopT>::unresolveVlanRifNextHop(
    const std::shared_ptr<SwitchState>& inputState,
    const NextHopT& nhop,
    const std::shared_ptr<Interface>& intf,
    bool useLinkLocal) const {
  auto outputState{inputState->clone()};
  auto vlan = outputState->getVlans()->getVlan(intf->getVlanID());
  auto nbrTable = vlan->template getNeighborEntryTable<AddrT>()->modify(
      vlan->getID(), &outputState);
  auto nhopIp = useLinkLocal ? nhop.linkLocalNhopIp.value() : nhop.ip;
  nbrTable->removeEntry(nhopIp);
  return outputState;
}

template <typename AddrT, typename NextHopT>
std::shared_ptr<SwitchState>
BaseEcmpSetupHelper<AddrT, NextHopT>::unresolvePortRifNextHop(
    const std::shared_ptr<SwitchState>& inputState,
    const NextHopT& nhop,
    const std::shared_ptr<Interface>& intf,
    bool useLinkLocal) const {
  auto outputState{inputState->clone()};
  auto nbrTable = intf->getNeighborEntryTable<AddrT>()->toThrift();
  auto nhopIp = useLinkLocal ? nhop.linkLocalNhopIp.value() : nhop.ip;
  nbrTable.erase(nhopIp.str());
  auto interfaceMap = outputState->getInterfaces()->modify(&outputState);
  auto interface = interfaceMap->getInterface(intf->getID())->clone();
  interface->setNeighborEntryTable<AddrT>(nbrTable);
  interfaceMap->updateNode(interface);
  return outputState;
}

template <typename AddrT, typename NextHopT>
std::shared_ptr<SwitchState>
BaseEcmpSetupHelper<AddrT, NextHopT>::resolveNextHop(
    const std::shared_ptr<SwitchState>& inputState,
    const NextHopT& nhop,
    bool useLinkLocal,
    std::optional<int64_t> encapIdx) const {
  auto intfID = portDesc2Interface_.find(nhop.portDesc)->second;
  auto intf = inputState->getInterfaces()->getInterface(intfID);
  switch (intf->getType()) {
    case cfg::InterfaceType::VLAN:
      CHECK(!encapIdx.has_value())
          << " Encap index not supported for VLAN rifs";
      return resolveVlanRifNextHop(inputState, nhop, intf, useLinkLocal);
    case cfg::InterfaceType::SYSTEM_PORT:
      return resolvePortRifNextHop(
          inputState, nhop, intf, useLinkLocal, encapIdx);
  }
  CHECK(false) << " Unhandled interface type: ";
  return nullptr;
}

template <typename AddrT, typename NextHopT>
std::shared_ptr<SwitchState>
BaseEcmpSetupHelper<AddrT, NextHopT>::unresolveNextHop(
    const std::shared_ptr<SwitchState>& inputState,
    const NextHopT& nhop,
    bool useLinkLocal) const {
  auto intfID = portDesc2Interface_.find(nhop.portDesc)->second;
  auto intf = inputState->getInterfaces()->getInterface(intfID);
  switch (intf->getType()) {
    case cfg::InterfaceType::VLAN:
      return unresolveVlanRifNextHop(inputState, nhop, intf, useLinkLocal);
    case cfg::InterfaceType::SYSTEM_PORT:
      return unresolvePortRifNextHop(inputState, nhop, intf, useLinkLocal);
  }
  CHECK(false) << " Unhandled interface type: ";
  return nullptr;
}

template <typename AddrT, typename NextHopT>
std::vector<PortDescriptor> BaseEcmpSetupHelper<AddrT, NextHopT>::ecmpPortDescs(
    int width) const {
  std::vector<PortDescriptor> portDescs;
  for (auto i = 0; i < width; ++i) {
    portDescs.push_back(nhops_[i].portDesc);
  }
  return portDescs;
}

template <typename AddrT, typename NextHopT>
std::optional<InterfaceID> BaseEcmpSetupHelper<AddrT, NextHopT>::getInterface(
    const PortDescriptor& port,
    const std::shared_ptr<SwitchState>& state) const {
  if (auto vlan = getVlan(port, state)) {
    auto intf = state->getInterfaces()->getInterfaceIf(
        InterfaceID(static_cast<int>(*vlan)));
    if (!intf) {
      // No interface config for this vlan
      return std::nullopt;
    }
    CHECK(intf->getType() == cfg::InterfaceType::VLAN);
    CHECK(intf->getVlanID() == *vlan);
    return intf->getID();
  } else {
    // Look for port RIF
    if (!port.isPhysicalPort()) {
      return std::nullopt;
    }
    if (!state->getSwitchSettings()->getSystemPortRange()) {
      return std::nullopt;
    }
    auto sysPortBase =
        *state->getSwitchSettings()->getSystemPortRange()->minimum();
    SystemPortID sysPortId{// static_cast to avoid spurious narrowing conversion
                           // compiler warning. PortID is just 16 bits
                           static_cast<int64_t>(port.intID()) + sysPortBase};
    if (auto intf = state->getInterfaces()->getInterfaceIf(
            InterfaceID(static_cast<int>(sysPortId)))) {
      return intf->getID();
    }
  }
  return std::nullopt;
}
template <typename AddrT, typename NextHopT>
std::optional<VlanID> BaseEcmpSetupHelper<AddrT, NextHopT>::getVlan(
    const PortDescriptor& port,
    const std::shared_ptr<SwitchState>& state) const {
  // Get physical port and extract first vlan from it. We assume
  // port is always part of a single vlan
  auto getPhysicalPortId = [&port, &state]() -> std::optional<PortID> {
    std::optional<PortID> portId;
    switch (port.type()) {
      case PortDescriptor::PortType::PHYSICAL:
        portId = port.phyPortID();
        break;
      case PortDescriptor::PortType::AGGREGATE: {
        auto aggPort =
            state->getAggregatePorts()->getAggregatePort(port.aggPortID());
        portId = aggPort->sortedSubports().begin()->portID;
      } break;

      case PortDescriptor::PortType::SYSTEM_PORT:
        break;
    }
    return portId;
  };
  if (auto phyPortId = getPhysicalPortId()) {
    auto phyPort = state->getPorts()->getPort(*phyPortId);
    for (const auto& vlanMember : phyPort->getVlans()) {
      return vlanMember.first;
    }
  }
  return std::nullopt;
}

template <typename AddrT, typename NextHopT>
AddrT BaseEcmpSetupHelper<AddrT, NextHopT>::addrWithOffset(
    const AddrT& subnetIp,
    int& offset) {
  auto bytes = subnetIp.toByteArray();
  // Add a offset to compute next in subnet next hop IP.
  // Essentially for l3 intf with subnet X, we
  // would compute next hops by incrementing last octet
  // of subnet.
  int lastOctet = (bytes[bytes.size() - 1] + (++offset)) % 255;
  // Fail if we goto 255 at the last oct
  CHECK_GT(255, lastOctet);
  bytes[bytes.size() - 1] = static_cast<uint8_t>(lastOctet);
  return AddrT(bytes);
}

template <typename IPAddrT>
EcmpSetupTargetedPorts<IPAddrT>::EcmpSetupTargetedPorts(
    const std::shared_ptr<SwitchState>& inputState,
    std::optional<folly::MacAddress> nextHopMac,
    RouterID routerId,
    bool forProdConfig)
    : BaseEcmpSetupHelper<IPAddrT, EcmpNextHopT>(), routerId_(routerId) {
  computeNextHops(inputState, nextHopMac, forProdConfig);
}

template <typename IPAddrT>
void EcmpSetupTargetedPorts<IPAddrT>::computeNextHops(
    const std::shared_ptr<SwitchState>& inputState,
    std::optional<folly::MacAddress> nextHopMac,
    bool forProdConfig) {
  BaseEcmpSetupHelperT::portDesc2Interface_ =
      BaseEcmpSetupHelperT::computePortDesc2Interface(inputState);
  auto intf2Subnet =
      computeInterface2Subnet(inputState, BaseEcmpSetupHelperT::kIsV6);
  int offset = 0;
  // Locally administered, unicast MACs
  auto baseMac = folly::MacAddress("06:00:00:00:00:00").u64HBO();
  for (const auto& portDescAndInterface :
       BaseEcmpSetupHelperT::portDesc2Interface_) {
    auto intf = portDescAndInterface.second;
    auto ipAddrStr = intf2Subnet[intf].first.str();
    if (ipAddrStr.empty()) {
      // Ignore intfs without ip addresses
      continue;
    }
    auto subnetIp = IPAddrT(ipAddrStr);
    auto nextHopIp = subnetIp;
    if (!forProdConfig) {
      nextHopIp = BaseEcmpSetupHelperT::addrWithOffset(subnetIp, offset);
    }
    BaseEcmpSetupHelperT::nhops_.push_back(EcmpNextHopT(
        nextHopIp,
        portDescAndInterface.first,
        nextHopMac ? MacAddress::fromHBO(nextHopMac.value().u64HBO())
                   : MacAddress::fromHBO(baseMac + offset),
        intf));
  }
}

template <typename IPAddrT>
typename EcmpSetupTargetedPorts<IPAddrT>::EcmpNextHopT
EcmpSetupTargetedPorts<IPAddrT>::nhop(PortDescriptor portDesc) const {
  auto it = std::find_if(
      BaseEcmpSetupHelperT::nhops_.begin(),
      BaseEcmpSetupHelperT::nhops_.end(),
      [portDesc](const EcmpNextHopT& nh) { return nh.portDesc == portDesc; });
  if (it == BaseEcmpSetupHelperT::nhops_.end()) {
    throw FbossError("Could not find a nhop for: ", portDesc);
  }
  return *it;
}

template <typename IPAddrT>
void EcmpSetupTargetedPorts<IPAddrT>::programRoutes(
    std::unique_ptr<RouteUpdateWrapper> updater,
    const flat_set<PortDescriptor>& portDescriptors,
    const std::vector<RouteT>& prefixes,
    const std::vector<NextHopWeight>& weights,
    std::optional<RouteCounterID> counterID) const {
  if (prefixes.empty()) {
    return;
  }
  std::vector<NextHopWeight> hopWeights;
  if (!weights.size()) {
    for (auto i = 0; i < portDescriptors.size(); ++i) {
      hopWeights.push_back(ECMP_WEIGHT);
    }
  } else {
    hopWeights = weights;
  }

  CHECK_EQ(portDescriptors.size(), hopWeights.size());
  RouteNextHopSet nhops;
  {
    auto i = 0;
    for (const auto& portDescriptor : portDescriptors) {
      nhops.emplace(UnresolvedNextHop(
          BaseEcmpSetupHelperT::ip(portDescriptor), hopWeights[i++]));
    }
  }
  for (const auto& prefix : prefixes) {
    updater->addRoute(
        routerId_,
        folly::IPAddress(prefix.network()),
        prefix.mask(),
        ClientID::BGPD,
        RouteNextHopEntry(nhops, AdminDistance::EBGP, counterID));
  }
  updater->program();
}

template <typename IPAddrT>
RouteNextHopSet EcmpSetupTargetedPorts<IPAddrT>::setupMplsNexthops(
    const flat_set<PortDescriptor>& portDescriptors,
    std::map<PortDescriptor, LabelForwardingAction::LabelStack>& stacks,
    LabelForwardingAction::LabelForwardingType labelActionType,
    const std::vector<NextHopWeight>& weights) const {
  RouteNextHopSet nhops;
  std::vector<NextHopWeight> hopWeights;
  if (!weights.size()) {
    for (auto i = 0; i < portDescriptors.size(); ++i) {
      hopWeights.push_back(ECMP_WEIGHT);
    }
  } else {
    hopWeights = weights;
  }

  CHECK_EQ(portDescriptors.size(), hopWeights.size());
  auto i = 0;
  for (const auto& portDescriptor : portDescriptors) {
    auto itr = stacks.find(portDescriptor);
    if (itr != stacks.end() && !itr->second.empty()) {
      if (labelActionType == LabelForwardingAction::LabelForwardingType::PUSH) {
        nhops.emplace(UnresolvedNextHop(
            BaseEcmpSetupHelperT::ip(portDescriptor),
            hopWeights[i++],
            LabelForwardingAction(labelActionType, itr->second)));
      } else {
        CHECK(
            labelActionType ==
            LabelForwardingAction::LabelForwardingType::SWAP);
        nhops.emplace(UnresolvedNextHop(
            BaseEcmpSetupHelperT::ip(portDescriptor),
            hopWeights[i++],
            LabelForwardingAction(labelActionType, itr->second[0])));
      }
    } else if (
        labelActionType == LabelForwardingAction::LabelForwardingType::PHP ||
        labelActionType ==
            LabelForwardingAction::LabelForwardingType::POP_AND_LOOKUP) {
      nhops.emplace(UnresolvedNextHop(
          BaseEcmpSetupHelperT::ip(portDescriptor),
          hopWeights[i++],
          LabelForwardingAction(labelActionType)));
    } else {
      nhops.emplace(UnresolvedNextHop(
          BaseEcmpSetupHelperT::ip(portDescriptor), hopWeights[i++]));
    }
  }

  return nhops;
}

template <typename IPAddrT>
void EcmpSetupTargetedPorts<IPAddrT>::programMplsRoutes(
    std::unique_ptr<RouteUpdateWrapper> updater,
    const flat_set<PortDescriptor>& portDescriptors,
    std::map<PortDescriptor, LabelForwardingAction::LabelStack> stacks,
    const std::vector<LabelID>& labels,
    LabelForwardingAction::LabelForwardingType labelActionType,
    const std::vector<NextHopWeight>& weights,
    std::optional<RouteCounterID> /* counterID */) const {
  if (labels.empty()) {
    return;
  }
  auto nhops =
      setupMplsNexthops(portDescriptors, stacks, labelActionType, weights);
  for (const auto& label : labels) {
    MplsRoute route;
    route.topLabel() = label;
    route.nextHops() = util::fromRouteNextHopSet(nhops);
    updater->addRoute(ClientID::BGPD, route);
  }
  updater->program();
}
template <typename IPAddrT>
void EcmpSetupTargetedPorts<IPAddrT>::programIp2MplsRoutes(
    std::unique_ptr<RouteUpdateWrapper> updater,
    const boost::container::flat_set<PortDescriptor>& portDescriptors,
    std::map<PortDescriptor, LabelForwardingAction::LabelStack> stacks,
    const std::vector<RouteT>& prefixes,
    const std::vector<NextHopWeight>& weights) const {
  std::vector<NextHopWeight> hopWeights;
  std::vector<NextHopWeight> forwardingActions;

  auto nhops = setupMplsNexthops(
      portDescriptors,
      stacks,
      LabelForwardingAction::LabelForwardingType::PUSH,
      weights);
  for (const auto& prefix : prefixes) {
    updater->addRoute(
        routerId_,
        folly::IPAddress(prefix.network()),
        prefix.mask(),
        ClientID::BGPD,
        RouteNextHopEntry(nhops, AdminDistance::EBGP));
  }
  updater->program();
}

template <typename IPAddrT>
void EcmpSetupTargetedPorts<IPAddrT>::unprogramRoutes(
    std::unique_ptr<RouteUpdateWrapper> wrapper,
    const std::vector<RouteT>& prefixes) const {
  for (const auto& prefix : prefixes) {
    wrapper->delRoute(
        routerId_,
        folly::IPAddress(prefix.network()),
        prefix.mask(),
        ClientID::BGPD);
  }
  wrapper->program();
}

template <typename IPAddrT>
flat_set<PortDescriptor> EcmpSetupAnyNPorts<IPAddrT>::getPortDescs(
    int width) const {
  flat_set<PortDescriptor> portDescs;
  for (size_t w = 0; w < width; ++w) {
    portDescs.insert(getNextHops().at(w).portDesc);
  }
  return portDescs;
}

template <typename IPAddrT>
std::shared_ptr<SwitchState> EcmpSetupAnyNPorts<IPAddrT>::resolveNextHops(
    const std::shared_ptr<SwitchState>& inputState,
    size_t numNextHops,
    bool useLinkLocal,
    std::optional<int64_t> encapIdx) const {
  return ecmpSetupTargetedPorts_.resolveNextHops(
      inputState, getPortDescs(numNextHops), useLinkLocal, encapIdx);
}

template <typename IPAddrT>
std::shared_ptr<SwitchState> EcmpSetupAnyNPorts<IPAddrT>::resolveNextHops(
    const std::shared_ptr<SwitchState>& inputState,
    const flat_set<PortDescriptor>& portDescs,
    bool useLinkLocal,
    std::optional<int64_t> encapIdx) const {
  return ecmpSetupTargetedPorts_.resolveNextHops(
      inputState, portDescs, useLinkLocal, encapIdx);
}

template <typename IPAddrT>
std::shared_ptr<SwitchState> EcmpSetupAnyNPorts<IPAddrT>::unresolveNextHops(
    const std::shared_ptr<SwitchState>& inputState,
    const flat_set<PortDescriptor>& portDescs,
    bool useLinkLocal) const {
  return ecmpSetupTargetedPorts_.unresolveNextHops(
      inputState, portDescs, useLinkLocal);
}

template <typename IPAddrT>
std::shared_ptr<SwitchState> EcmpSetupAnyNPorts<IPAddrT>::unresolveNextHops(
    const std::shared_ptr<SwitchState>& inputState,
    size_t numNextHops,
    bool useLinkLocal) const {
  return ecmpSetupTargetedPorts_.unresolveNextHops(
      inputState, getPortDescs(numNextHops), useLinkLocal);
}

template <typename IPAddrT>
void EcmpSetupAnyNPorts<IPAddrT>::programRoutes(
    std::unique_ptr<RouteUpdateWrapper> updater,
    size_t width,
    const std::vector<RouteT>& prefixes,
    const std::vector<NextHopWeight>& weights) const {
  ecmpSetupTargetedPorts_.programRoutes(
      std::move(updater), getPortDescs(width), prefixes, weights);
}

template <typename IPAddrT>
void EcmpSetupAnyNPorts<IPAddrT>::programRoutes(
    std::unique_ptr<RouteUpdateWrapper> updater,
    const flat_set<PortDescriptor>& portDescs,
    const std::vector<RouteT>& prefixes,
    const std::vector<NextHopWeight>& weights) const {
  ecmpSetupTargetedPorts_.programRoutes(
      std::move(updater), portDescs, prefixes, weights);
}

template <typename IPAddrT>
void EcmpSetupAnyNPorts<IPAddrT>::programIp2MplsRoutes(
    std::unique_ptr<RouteUpdateWrapper> updater,
    size_t width,
    const std::vector<RouteT>& prefixes,
    std::vector<LabelForwardingAction::LabelStack> stacks,
    const std::vector<NextHopWeight>& weights) const {
  auto ports = getPortDescs(width);
  std::map<PortDescriptor, LabelForwardingAction::LabelStack> port2Stack;
  auto i = 0;
  for (auto port : ports) {
    i = (i % stacks.size());
    port2Stack.emplace(port, stacks[i++]);
  }
  ecmpSetupTargetedPorts_.programIp2MplsRoutes(
      std::move(updater), ports, port2Stack, prefixes, weights);
}

template <typename IPAddrT>
void EcmpSetupAnyNPorts<IPAddrT>::unprogramRoutes(
    std::unique_ptr<RouteUpdateWrapper> wrapper,
    const std::vector<RouteT>& prefixes) const {
  ecmpSetupTargetedPorts_.unprogramRoutes(std::move(wrapper), prefixes);
}

template <typename IPAddrT>
std::vector<PortDescriptor> EcmpSetupAnyNPorts<IPAddrT>::ecmpPortDescs(
    int width) const {
  return ecmpSetupTargetedPorts_.ecmpPortDescs(width);
}

template <typename IPAddrT>
void MplsEcmpSetupTargetedPorts<IPAddrT>::setupECMPForwarding(
    const std::shared_ptr<SwitchState>& inputState,
    std::unique_ptr<RouteUpdateWrapper> updater,
    const boost::container::flat_set<PortDescriptor>& portDescriptors,
    const std::vector<NextHopWeight>& weights) const {
  std::vector<NextHopWeight> hopWeights;
  if (!weights.size()) {
    for (auto i = 0; i < portDescriptors.size(); ++i) {
      hopWeights.push_back(ECMP_WEIGHT);
    }
  } else {
    hopWeights = weights;
  }

  CHECK_EQ(portDescriptors.size(), hopWeights.size());
  auto outputState{inputState->clone()};
  LabelNextHopSet nhops;
  {
    auto i = 0;
    for (const auto& portDescriptor : portDescriptors) {
      auto iter =
          BaseEcmpSetupHelperT::portDesc2Interface_.find(portDescriptor);
      auto intfId = iter->second;
      auto nexthop = nhop(portDescriptor);
      nhops.emplace(
          LabelNextHop(nexthop.ip, intfId, hopWeights[i++], nexthop.action));
    }
  }

  MplsRoute route;
  route.topLabel() = topLabel_.value();
  route.nextHops() = util::fromRouteNextHopSet(nhops);
  updater->addRoute(ClientID(0), route);
  updater->program();
}

template <typename IPAddrT>
void MplsEcmpSetupTargetedPorts<IPAddrT>::computeNextHops(
    const std::shared_ptr<SwitchState>& inputState,
    std::optional<folly::MacAddress> nextHopMac,
    bool forProdConfig) {
  BaseEcmpSetupHelperT::portDesc2Interface_ =
      BaseEcmpSetupHelperT::computePortDesc2Interface(inputState);
  auto intf2Subnet =
      computeInterface2Subnet(inputState, BaseEcmpSetupHelperT::kIsV6);
  int offset = 0;
  auto baseMac = folly::MacAddress("06:00:00:00:00:00").u64HBO();
  for (const auto& portDescAndInterface :
       BaseEcmpSetupHelperT::portDesc2Interface_) {
    auto intf = portDescAndInterface.second;
    auto subnetIp = IPAddrT(intf2Subnet[intf].first.str());
    auto nextHopIp = subnetIp;
    if (!forProdConfig) {
      nextHopIp = BaseEcmpSetupHelperT::addrWithOffset(subnetIp, offset);
    }
    BaseEcmpSetupHelperT::nhops_.push_back(EcmpMplsNextHop<IPAddrT>(
        nextHopIp,
        portDescAndInterface.first,
        nextHopMac ? MacAddress::fromHBO(nextHopMac.value().u64HBO())
                   : MacAddress::fromHBO(baseMac + offset),
        intf,
        getLabelForwardingAction(portDescAndInterface.first)));
  }
}

template <typename IPAddrT>
EcmpMplsNextHop<IPAddrT> MplsEcmpSetupTargetedPorts<IPAddrT>::nhop(
    PortDescriptor portDesc) const {
  auto it = std::find_if(
      BaseEcmpSetupHelperT::nhops_.begin(),
      BaseEcmpSetupHelperT::nhops_.end(),
      [portDesc](const auto& nh) { return nh.portDesc == portDesc; });
  if (it == BaseEcmpSetupHelperT::nhops_.end()) {
    throw FbossError("Could not find a nhop for: ", portDesc);
  }
  return *it;
}

template <typename IPAddrT>
LabelForwardingAction
MplsEcmpSetupTargetedPorts<IPAddrT>::getLabelForwardingAction(
    PortDescriptor port) const {
  auto label = port.isAggregatePort() ? port.aggPortID() : port.phyPortID();

  LabelForwardingAction::LabelStack pushStack;
  for (auto i = 1; i <= 3; i++) {
    pushStack.push_back(label * 10 + i);
  }

  switch (actionType_) {
    case LabelForwardingAction::LabelForwardingType::PUSH:
      return LabelForwardingAction(actionType_, std::move(pushStack));

    case LabelForwardingAction::LabelForwardingType::SWAP:
      return LabelForwardingAction(actionType_, pushStack[0]);
    case LabelForwardingAction::LabelForwardingType::PHP:
    case LabelForwardingAction::LabelForwardingType::POP_AND_LOOKUP:
    case LabelForwardingAction::LabelForwardingType::NOOP:
      return LabelForwardingAction(actionType_);
  }
  return LabelForwardingAction(
      LabelForwardingAction::LabelForwardingType::NOOP);
}

template <typename IPAddrT>
flat_set<PortDescriptor> MplsEcmpSetupAnyNPorts<IPAddrT>::getPortDescs(
    int width) const {
  flat_set<PortDescriptor> portDescs;
  for (size_t w = 0; w < width; ++w) {
    portDescs.insert(getNextHops().at(w).portDesc);
  }
  return portDescs;
}

template <typename IPAddrT>
std::vector<PortDescriptor> MplsEcmpSetupAnyNPorts<IPAddrT>::ecmpPortDescs(
    int width) const {
  return mplsEcmpSetupTargetedPorts_.ecmpPortDescs(width);
}

template <typename IPAddrT>
std::shared_ptr<SwitchState> MplsEcmpSetupAnyNPorts<IPAddrT>::resolveNextHops(
    const std::shared_ptr<SwitchState>& inputState,
    size_t numNextHops,
    bool useLinkLocal) const {
  return mplsEcmpSetupTargetedPorts_.resolveNextHops(
      inputState, getPortDescs(numNextHops), useLinkLocal);
}

template <typename IPAddrT>
std::shared_ptr<SwitchState> MplsEcmpSetupAnyNPorts<IPAddrT>::unresolveNextHops(
    const std::shared_ptr<SwitchState>& inputState,
    size_t numNextHops,
    bool useLinkLocal) const {
  return mplsEcmpSetupTargetedPorts_.unresolveNextHops(
      inputState, getPortDescs(numNextHops), useLinkLocal);
}

template <typename IPAddrT>
std::shared_ptr<SwitchState> MplsEcmpSetupAnyNPorts<IPAddrT>::resolveNextHops(
    const std::shared_ptr<SwitchState>& inputState,
    const boost::container::flat_set<PortDescriptor>& portDescs,
    bool useLinkLocal) const {
  return mplsEcmpSetupTargetedPorts_.resolveNextHops(
      inputState, portDescs, useLinkLocal);
}

template <typename IPAddrT>
std::shared_ptr<SwitchState> MplsEcmpSetupAnyNPorts<IPAddrT>::unresolveNextHops(
    const std::shared_ptr<SwitchState>& inputState,
    const boost::container::flat_set<PortDescriptor>& portDescs,
    bool useLinkLocal) const {
  return mplsEcmpSetupTargetedPorts_.unresolveNextHops(
      inputState, portDescs, useLinkLocal);
}

template <typename IPAddrT>
void MplsEcmpSetupAnyNPorts<IPAddrT>::setupECMPForwarding(
    const std::shared_ptr<SwitchState>& inputState,
    std::unique_ptr<RouteUpdateWrapper> updater,
    size_t width,
    const std::vector<NextHopWeight>& weights) const {
  mplsEcmpSetupTargetedPorts_.setupECMPForwarding(
      inputState, std::move(updater), getPortDescs(width), weights);
}

template class BaseEcmpSetupHelper<
    folly::IPAddressV4,
    EcmpNextHop<folly::IPAddressV4>>;
template class BaseEcmpSetupHelper<
    folly::IPAddressV6,
    EcmpNextHop<folly::IPAddressV6>>;

template class BaseEcmpSetupHelper<
    folly::IPAddressV4,
    EcmpMplsNextHop<folly::IPAddressV4>>;
template class BaseEcmpSetupHelper<
    folly::IPAddressV6,
    EcmpMplsNextHop<folly::IPAddressV6>>;

template class EcmpSetupTargetedPorts<folly::IPAddressV4>;
template class EcmpSetupTargetedPorts<folly::IPAddressV6>;
template class EcmpSetupAnyNPorts<folly::IPAddressV4>;
template class EcmpSetupAnyNPorts<folly::IPAddressV6>;

template class MplsEcmpSetupTargetedPorts<folly::IPAddressV4>;
template class MplsEcmpSetupTargetedPorts<folly::IPAddressV6>;
template class MplsEcmpSetupAnyNPorts<folly::IPAddressV4>;
template class MplsEcmpSetupAnyNPorts<folly::IPAddressV6>;

} // namespace facebook::fboss::utility
