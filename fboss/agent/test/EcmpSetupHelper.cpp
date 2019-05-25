/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "EcmpSetupHelper.h"

#include <iterator>

#include "fboss/agent/state/AggregatePort.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/PortDescriptor.h"
#include "fboss/agent/state/RouteUpdater.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/Vlan.h"
#include "fboss/agent/types.h"

#include <folly/IPAddress.h>


using folly::IPAddress;
using folly::IPAddressV4;
using folly::IPAddressV6;
using folly::MacAddress;
using boost::container::flat_map;
using boost::container::flat_set;

namespace {

using namespace facebook::fboss;

folly::Optional<AggregatePortID> getAggPortID(
  const std::shared_ptr<SwitchState>& inputState,
  const PortID& portId) {
  for (auto aggPort : *inputState->getAggregatePorts().get()) {
    if (aggPort->isMemberPort(portId)) {
      return aggPort->getID();
    }
  }
  return folly::none;
}

flat_map<PortDescriptor, VlanID> computePortDesc2Vlan(
    const std::shared_ptr<SwitchState>& inputState) {
  boost::container::flat_map<PortDescriptor, VlanID> portDesc2Vlan;
  flat_set<PortID> portIds;
  for (const auto& port : *inputState->getPorts().get()) {
    portIds.insert(port->getID());
  }
  for (const auto portId : portIds) {
    PortDescriptor portDesc = PortDescriptor(portId);
    auto aggId = getAggPortID(inputState, portId);
    if (aggId) {
      portDesc = PortDescriptor(*aggId);
    }
    auto port = inputState->getPorts()->getPort(portId);
    for (const auto& vlanMember : port->getVlans()) {
      portDesc2Vlan.insert(std::make_pair(portDesc, VlanID(vlanMember.first)));
    }
  }
  return portDesc2Vlan;
}

flat_map<VlanID, folly::CIDRNetwork> computeVlan2Subnet(
    const std::shared_ptr<SwitchState>& inputState,
    bool v6) {
  boost::container::flat_map<VlanID, folly::CIDRNetwork> vlan2Network;
  for (const auto& intf : *inputState->getInterfaces().get()) {
    for (const auto cidrStr : intf->getAddresses()) {
      auto subnet = folly::IPAddress::createNetwork(cidrStr.first.str());
      if (!v6 && subnet.first.isV4()) {
        vlan2Network[intf->getVlanID()] = subnet;
      } else if (v6 && !subnet.first.isLinkLocal()) {
        vlan2Network[intf->getVlanID()] = subnet;
      }
    }
  }
  return vlan2Network;
}
} // namespace

namespace facebook {
namespace fboss {
namespace utility {

template <typename IPAddrT>
EcmpSetupTargetedPorts<IPAddrT>::EcmpSetupTargetedPorts(
    const std::shared_ptr<SwitchState>& inputState,
    folly::Optional<folly::MacAddress> nextHopMac,
    RouterID routerId,
    typename EcmpSetupTargetedPorts<IPAddrT>::RouteT routePrefix)
    : routerId_(routerId), routePrefix_(routePrefix) {
  computeNextHops(inputState, nextHopMac);
}

template <typename IPAddrT>
void EcmpSetupTargetedPorts<IPAddrT>::computeNextHops(
    const std::shared_ptr<SwitchState>& inputState,
    folly::Optional<folly::MacAddress> nextHopMac) {
  portDesc2Vlan_ = computePortDesc2Vlan(inputState);
  auto vlan2Subnet = computeVlan2Subnet(
      inputState, folly::IPAddress(routePrefix_.network).isV6());
  int offset = 0;
  auto baseMac = folly::MacAddress("01:00:00:00:00:00").u64HBO();
  for (const auto& portDescAndVlan : portDesc2Vlan_) {
    auto vlan = portDescAndVlan.second;
    auto subnetIp = IPAddrT(vlan2Subnet[vlan].first.str());
    auto bytes = subnetIp.toByteArray();
    // Add a offset to compute next in subnet next hop IP.
    // Essentially for vlan/l3 intf with subnet X, we
    // would compute next hops by incrementing last octet
    // of subnet.
    int lastOctet = bytes[bytes.size() - 1] + (++offset);
    // Fail if we goto 255 at the last oct
    CHECK_GT(255, lastOctet);
    bytes[bytes.size() - 1] = static_cast<uint8_t>(lastOctet);
    nhops_.push_back(EcmpNextHop(
        IPAddrT(bytes),
        portDescAndVlan.first,
        nextHopMac ? MacAddress::fromHBO(nextHopMac.value().u64HBO())
                   : MacAddress::fromHBO(baseMac + offset)));
  }
}

template <typename IPAddrT>
EcmpNextHop<IPAddrT> EcmpSetupTargetedPorts<IPAddrT>::nhop(
    PortDescriptor portDesc) const {
  auto it = std::find_if(
      nhops_.begin(), nhops_.end(), [portDesc](const EcmpNextHop& nh) {
        return nh.portDesc == portDesc;
      });
  if (it == nhops_.end()) {
    throw FbossError("Could not find a nhop for: ", portDesc);
  }
  return *it;
}

template <typename IPAddrT>
std::vector<PortDescriptor> EcmpSetupTargetedPorts<IPAddrT>::ecmpPortDescs(
    int width) const {
  std::vector<PortDescriptor> portDescs;
  for (auto i = 0; i < width; ++i) {
    portDescs.push_back(nhops_[i].portDesc);
  }
  return portDescs;
}

template <typename IPAddrT>
std::shared_ptr<SwitchState> EcmpSetupTargetedPorts<IPAddrT>::resolveNextHop(
    const std::shared_ptr<SwitchState>& inputState,
    const EcmpSetupTargetedPorts<IPAddrT>::EcmpNextHop& nhop) const {
  auto outputState{inputState->clone()};
  auto vlanId = portDesc2Vlan_.find(nhop.portDesc)->second;
  auto vlan = outputState->getVlans()->getVlan(vlanId);
  auto nbrTable = vlan->template getNeighborEntryTable<IPAddrT>()->modify(
      vlanId, &outputState);
  if (nbrTable->getEntryIf(nhop.ip)) {
    nbrTable->updateEntry(
        nhop.ip, nhop.mac, nhop.portDesc, vlan->getInterfaceID());
  } else {
    nbrTable->addEntry(
        nhop.ip, nhop.mac, nhop.portDesc, vlan->getInterfaceID());
  }
  return outputState;
}

template <typename IPAddrT>
std::shared_ptr<SwitchState> EcmpSetupTargetedPorts<IPAddrT>::unresolveNextHop(
    const std::shared_ptr<SwitchState>& inputState,
    const EcmpSetupTargetedPorts<IPAddrT>::EcmpNextHop& nhop) const {
  auto outputState{inputState->clone()};
  auto vlanId = portDesc2Vlan_.find(nhop.portDesc)->second;
  auto vlan = outputState->getVlans()->getVlan(vlanId);
  auto nbrTable = vlan->template getNeighborEntryTable<IPAddrT>()->modify(
      vlanId, &outputState);
  nbrTable->removeEntry(nhop.ip);
  return outputState;
}

template <typename IPAddrT>
std::shared_ptr<SwitchState> EcmpSetupTargetedPorts<IPAddrT>::resolveNextHops(
    const std::shared_ptr<SwitchState>& inputState,
    const flat_set<PortDescriptor>& portDescs) const {
  return resolveNextHopsImpl(inputState, portDescs, true);
}

template <typename IPAddrT>
std::shared_ptr<SwitchState> EcmpSetupTargetedPorts<IPAddrT>::unresolveNextHops(
    const std::shared_ptr<SwitchState>& inputState,
    const flat_set<PortDescriptor>& portDescs) const {
  return resolveNextHopsImpl(inputState, portDescs, false);
}

template <typename IPAddrT>
std::shared_ptr<SwitchState>
EcmpSetupTargetedPorts<IPAddrT>::resolveNextHopsImpl(
    const std::shared_ptr<SwitchState>& inputState,
    const flat_set<PortDescriptor>& portDescs,
    bool resolve) const {
  auto outputState{inputState};
  for (auto nhop : nhops_) {
    if (portDescs.find(nhop.portDesc) != portDescs.end()) {
      outputState = resolve ? resolveNextHop(outputState, nhop)
                            : unresolveNextHop(outputState, nhop);
    }
  }
  return outputState;
}

template <typename IPAddrT>
std::shared_ptr<SwitchState>
EcmpSetupTargetedPorts<IPAddrT>::setupECMPForwarding(
    const std::shared_ptr<SwitchState>& inputState,
    const flat_set<PortDescriptor>& portDescriptors,
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
  RouteNextHopSet nhops;
  {
    auto i = 0;
    for (const auto& portDescriptor : portDescriptors) {
      nhops.emplace(UnresolvedNextHop(ip(portDescriptor), hopWeights[i++]));
    }
  }
  const auto& tables1 = outputState->getRouteTables();
  RouteUpdater u2(tables1);
  u2.addRoute(
      routerId_,
      folly::IPAddress(routePrefix_.network),
      routePrefix_.mask,
      ClientID(1001),
      RouteNextHopEntry(nhops, AdminDistance::STATIC_ROUTE));

  auto tables2 = u2.updateDone();
  tables2->publish();

  outputState->resetRouteTables(tables2);
  return outputState;
}

template <typename IPAddrT>
flat_set<PortDescriptor>
EcmpSetupAnyNPorts<IPAddrT>::getPortDescs(int width) const {
  flat_set<PortDescriptor> portDescs;
  for (size_t w = 0; w < width; ++w) {
    portDescs.insert(getNextHops().at(w).portDesc);
  }
  return portDescs;
}

template <typename IPAddrT>
std::shared_ptr<SwitchState> EcmpSetupAnyNPorts<IPAddrT>::resolveNextHops(
    const std::shared_ptr<SwitchState>& inputState,
    size_t numNextHops) const {
  return ecmpSetupTargetedPorts_.resolveNextHops(
      inputState, getPortDescs(numNextHops));
}

template <typename IPAddrT>
std::shared_ptr<SwitchState> EcmpSetupAnyNPorts<IPAddrT>::resolveNextHops(
    const std::shared_ptr<SwitchState>& inputState,
    const flat_set<PortDescriptor>& portDescs) const {
  return ecmpSetupTargetedPorts_.resolveNextHops(inputState, portDescs);
}
template <typename IPAddrT>
std::shared_ptr<SwitchState> EcmpSetupAnyNPorts<IPAddrT>::unresolveNextHops(
    const std::shared_ptr<SwitchState>& inputState,
    size_t numNextHops) const {
  return ecmpSetupTargetedPorts_.unresolveNextHops(
      inputState, getPortDescs(numNextHops));
}

template <typename IPAddrT>
std::shared_ptr<SwitchState> EcmpSetupAnyNPorts<IPAddrT>::setupECMPForwarding(
    const std::shared_ptr<SwitchState>& inputState,
    size_t ecmpWidth,
    const std::vector<NextHopWeight>& weights) const {
  return ecmpSetupTargetedPorts_.setupECMPForwarding(
      inputState, getPortDescs(ecmpWidth), weights);
}

template <typename IPAddrT>
std::vector<PortDescriptor> EcmpSetupAnyNPorts<IPAddrT>::ecmpPortDescs(
    int width) const {
  return ecmpSetupTargetedPorts_.ecmpPortDescs(width);
}

template class EcmpSetupTargetedPorts<folly::IPAddressV4>;
template class EcmpSetupTargetedPorts<folly::IPAddressV6>;
template class EcmpSetupAnyNPorts<folly::IPAddressV4>;
template class EcmpSetupAnyNPorts<folly::IPAddressV6>;

} // namespace utility
} // namespace fboss
} // namespace facebook
