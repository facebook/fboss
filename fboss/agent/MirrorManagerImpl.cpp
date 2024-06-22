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

#include "fboss/agent/platforms/common/PlatformMappingUtils.h"

DECLARE_bool(intf_nbr_tables);

template <typename AddrT>
using NeighborEntryT =
    typename facebook::fboss::MirrorManagerImpl<AddrT>::NeighborEntryT;

namespace {

using facebook::fboss::Interface;
using facebook::fboss::PortID;
using facebook::fboss::SwitchState;

template <typename AddrT>
auto getNeighborEntryTableHelper(
    const std::shared_ptr<SwitchState>& state,
    const std::shared_ptr<Interface>& interface) {
  if (FLAGS_intf_nbr_tables) {
    return interface->template getNeighborEntryTable<AddrT>();
  } else {
    auto vlan = state->getVlans()->getNodeIf(interface->getVlanID());
    return vlan->template getNeighborEntryTable<AddrT>();
  }
}

folly::MacAddress getEventorPortInterfaceMac(
    const std::shared_ptr<SwitchState>& state,
    PortID portId) {
  auto intfID = state->getInterfaceIDForPort(portId);
  auto intf = state->getInterfaces()->getNodeIf(intfID);
  return intf->getMac();
}

} // namespace

namespace facebook::fboss {

template <typename AddrT>
PortID MirrorManagerImpl<AddrT>::getEventorPortForSflowMirror(
    SwitchID switchId) {
  const auto& portIds =
      sw_->getPlatformMapping()->getPlatformPorts(cfg::PortType::EVENTOR_PORT);
  HwSwitchMatcher matcher(std::unordered_set<SwitchID>({switchId}));
  for (const auto& portId : portIds) {
    if (sw_->getScopeResolver()->scope(portId) == matcher) {
      return portId;
    }
  }
  throw FbossError("No eventor port found for sflow mirror");
}

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

  auto asic = sw_->getHwAsicTable()->getHwAsic(mirror->getSwitchId());
  if (newMirror && newMirror->type() == Mirror::Type::SFLOW &&
      asic->isSupported(HwAsic::Feature::EVENTOR_PORT_FOR_SFLOW)) {
    // TODO: Destination mac address of the mirror session also
    // should be overridden to router mac so that the packet can
    // loop back and hit the eventor port and get routed to the
    // mirror destination
    auto eventorPort = getEventorPortForSflowMirror(mirror->getSwitchId());
    newMirror->setEgressPort(eventorPort);
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
  if (interface->hasAddress(mirrorNextHopIp)) {
    /* if mirror destination is directly connected */
    neighbor = getNeighborEntryTableHelper<AddrT>(state, interface)
                   ->getEntryIf(destinationIp);
  } else {
    neighbor = getNeighborEntryTableHelper<AddrT>(state, interface)
                   ->getEntryIf(mirrorNextHopIp);
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
