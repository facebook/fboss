// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/MirrorManagerImpl.h"

#include "folly/IPAddress.h"

#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/state/AggregatePort.h"
#include "fboss/agent/state/DeltaFunctions.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/Mirror.h"
#include "fboss/agent/state/Route.h"

#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/Vlan.h"

template <typename AddrT>
using NeighborEntryT =
    typename facebook::fboss::MirrorManagerImpl<AddrT>::NeighborEntryT;

namespace facebook::fboss {

template <typename AddrT>
std::shared_ptr<Mirror> MirrorManagerImpl<AddrT>::updateMirror(
    const std::shared_ptr<Mirror>& mirror) {
  const AddrT destinationIp =
      getIPAddress<AddrT>(mirror->getDestinationIp().value());
  const auto state = sw_->getState();
  const auto nexthops = resolveMirrorNextHops(state, destinationIp);

  auto newMirror = std::make_shared<Mirror>(
      mirror->getID(),
      mirror->configHasEgressPort() ? mirror->getEgressPort() : std::nullopt,
      mirror->getDestinationIp(),
      mirror->getSrcIp(),
      mirror->getTunnelUdpPorts(),
      mirror->getDscp(),
      mirror->getTruncate());

  for (const auto& nexthop : nexthops) {
    const auto entry =
        resolveMirrorNextHopNeighbor(state, mirror, destinationIp, nexthop);

    if (!entry || entry->zeroPort()) {
      // unresolved next hop
      continue;
    }
    auto neighborPort = entry->getPort();
    if (mirror->configHasEgressPort()) {
      if (!neighborPort.isPhysicalPort() ||
          neighborPort.phyPortID() != mirror->getEgressPort().value()) {
        // TODO: support configuring LAG egress for mirror
        continue;
      }
    }

    std::optional<PortID> egressPort{};
    switch (neighborPort.type()) {
      case PortDescriptor::PortType::PHYSICAL:
        egressPort = entry->getPort().phyPortID();
        break;
      case PortDescriptor::PortType::AGGREGATE: {
        // pick first forwarding member port
        auto aggPort =
            state->getAggregatePorts()->getNodeIf(entry->getPort().aggPortID());
        if (!aggPort) {
          XLOG(ERR) << "mirror resolved to non-existing aggregate port "
                    << entry->getPort().aggPortID();
          continue;
        }
        auto subportAndFwdStates = aggPort->subportAndFwdState();
        if (subportAndFwdStates.begin() == subportAndFwdStates.end()) {
          continue;
        }
        for (auto subPortAndFwdState : subportAndFwdStates) {
          if (subPortAndFwdState.second == AggregatePort::Forwarding::ENABLED) {
            egressPort = subPortAndFwdState.first;
            break;
          }
        }
      } break;
      case PortDescriptor::PortType::SYSTEM_PORT:
        XLOG(FATAL) << " No mirroring over system ports";
        break;
    }

    if (!egressPort) {
      continue;
    }

    std::optional<AddrT> srcIp = newMirror->getSrcIp()
        ? getIPAddress<AddrT>(mirror->getSrcIp().value())
        : std::optional<AddrT>(std::nullopt);
    newMirror->setMirrorTunnel(resolveMirrorTunnel(
        state,
        destinationIp,
        srcIp,
        nexthop,
        entry,
        newMirror->getTunnelUdpPorts()));
    newMirror->setEgressPort(egressPort.value());
    break;
  }

  if (*mirror == *newMirror) {
    return std::shared_ptr<Mirror>(nullptr);
  }
  return newMirror;
}

template <typename AddrT>
RouteNextHopEntry::NextHopSet MirrorManagerImpl<AddrT>::resolveMirrorNextHops(
    const std::shared_ptr<SwitchState>& state,
    const AddrT& destinationIp) {
  const auto route =
      sw_->longestMatch<AddrT>(state, destinationIp, RouterID(0));
  if (!route || !route->isResolved()) {
    return RouteNextHopEntry::NextHopSet();
  }
  return route->getForwardInfo().getNextHopSet();
}

template <typename AddrT>
std::shared_ptr<NeighborEntryT<AddrT>>
MirrorManagerImpl<AddrT>::resolveMirrorNextHopNeighbor(
    const std::shared_ptr<SwitchState>& state,
    const std::shared_ptr<Mirror>& mirror,
    const AddrT& destinationIp,
    const NextHop& nexthop) const {
  std::shared_ptr<NeighborEntryT> neighbor;
  if (!nexthop.isResolved()) {
    return std::shared_ptr<NeighborEntryT>(nullptr);
  }
  AddrT mirrorNextHopIp = getIPAddress<AddrT>(nexthop.addr());
  InterfaceID mirrorEgressInterface = nexthop.intf();

  auto interface = state->getInterfaces()->getNodeIf(mirrorEgressInterface);
  auto vlan = state->getVlans()->getNodeIf(interface->getVlanID());

  if (interface->hasAddress(mirrorNextHopIp)) {
    /* if mirror destination is directly connected */
    neighbor = vlan->template getNeighborEntryTable<AddrT>()->getEntryIf(
        destinationIp);
  } else {
    neighbor = vlan->template getNeighborEntryTable<AddrT>()->getEntryIf(
        mirrorNextHopIp);
  }
  return neighbor;
}

template <typename AddrT>
MirrorTunnel MirrorManagerImpl<AddrT>::resolveMirrorTunnel(
    const std::shared_ptr<SwitchState>& state,
    const AddrT& destinationIp,
    const std::optional<AddrT>& srcIp,
    const NextHop& nextHop,
    const std::shared_ptr<NeighborEntryT>& neighbor,
    const std::optional<TunnelUdpPorts>& udpPorts) {
  const auto interface = state->getInterfaces()->getNodeIf(nextHop.intf());
  const auto iter = interface->getAddressToReach(neighbor->getIP());

  if (udpPorts.has_value()) {
    return MirrorTunnel(
        srcIp ? srcIp.value() : iter->first,
        destinationIp,
        interface->getMac(),
        neighbor->getMac(),
        udpPorts.value());
  }
  return MirrorTunnel(
      srcIp ? srcIp.value() : iter->first,
      destinationIp,
      interface->getMac(),
      neighbor->getMac());
}

template class MirrorManagerImpl<folly::IPAddressV4>;
template class MirrorManagerImpl<folly::IPAddressV6>;

} // namespace facebook::fboss
