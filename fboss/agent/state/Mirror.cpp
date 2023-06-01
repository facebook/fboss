// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/state/Mirror.h"
#include <fboss/agent/state/Thrifty.h>
#include <memory>
#include "common/network/if/gen-cpp2/Address_types.h"
#include "fboss/agent/AddressUtil.h"
#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/agent/state/NodeBase-defs.h"
#include "fboss/agent/state/Port.h"
#include "folly/IPAddress.h"
#include "folly/MacAddress.h"
#include "folly/dynamic.h"

namespace facebook::fboss {

Mirror::Mirror(
    std::string name,
    std::optional<PortID> egressPort,
    std::optional<folly::IPAddress> destinationIp,
    std::optional<folly::IPAddress> srcIp,
    std::optional<TunnelUdpPorts> udpPorts,
    uint8_t dscp,
    bool truncate)
    : ThriftStructNode<Mirror, state::MirrorFields>() {
  // span mirror is resolved as soon as it is created
  // erspan and sflow are resolved when tunnel is set
  set<switch_state_tags::name>(name);
  set<switch_state_tags::dscp>(dscp);
  set<switch_state_tags::truncate>(truncate);
  set<switch_state_tags::configHasEgressPort>(false);
  set<switch_state_tags::isResolved>(false);

  if (egressPort) {
    set<switch_state_tags::egressPort>(*egressPort);
    set<switch_state_tags::configHasEgressPort>(true);
  }
  if (destinationIp) {
    set<switch_state_tags::destinationIp>(
        network::toBinaryAddress(*destinationIp));
  }
  if (srcIp) {
    set<switch_state_tags::srcIp>(network::toBinaryAddress(*srcIp));
  }
  if (udpPorts) {
    set<switch_state_tags::udpSrcPort>(udpPorts->udpSrcPort);
    set<switch_state_tags::udpDstPort>(udpPorts->udpDstPort);
  }
  markResolved();
}

std::string Mirror::getID() const {
  return get<switch_state_tags::name>()->cref();
}

std::optional<PortID> Mirror::getEgressPort() const {
  if (auto port = get<switch_state_tags::egressPort>()) {
    return PortID(port->cref());
  }
  return std::nullopt;
}

std::optional<TunnelUdpPorts> Mirror::getTunnelUdpPorts() const {
  auto srcPort = get<switch_state_tags::udpSrcPort>();
  auto dstPort = get<switch_state_tags::udpDstPort>();
  if (srcPort && dstPort) {
    return TunnelUdpPorts(srcPort->cref(), dstPort->cref());
  }
  return std::nullopt;
}

std::optional<MirrorTunnel> Mirror::getMirrorTunnel() const {
  auto tunnel = get<switch_state_tags::tunnel>();
  if (tunnel) {
    return MirrorTunnel::fromThrift(tunnel->toThrift());
  }
  return std::nullopt;
}

uint8_t Mirror::getDscp() const {
  return get<switch_state_tags::dscp>()->cref();
}

bool Mirror::getTruncate() const {
  return get<switch_state_tags::truncate>()->cref();
}

void Mirror::setTruncate(bool truncate) {
  set<switch_state_tags::truncate>(truncate);
}

void Mirror::setEgressPort(PortID egressPort) {
  set<switch_state_tags::egressPort>(egressPort);
}

void Mirror::setMirrorTunnel(const MirrorTunnel& tunnel) {
  set<switch_state_tags::tunnel>(tunnel.toThrift());
  // sflow or erspan mirror is resolved.
  markResolved();
}

bool Mirror::isResolved() const {
  // either mirror has no destination ip (which means it is span)
  // or its destination ip is resolved.
  return get<switch_state_tags::isResolved>()->cref();
}

void Mirror::markResolved() {
  set<switch_state_tags::isResolved>(false);
  if (type() == Mirror::Type::SPAN || get<switch_state_tags::tunnel>()) {
    // either mirror has no destination ip (which means it is span)
    // or its destination ip is resolved.
    set<switch_state_tags::isResolved>(true);
  }
}

bool Mirror::configHasEgressPort() const {
  return get<switch_state_tags::configHasEgressPort>()->cref();
}

std::optional<folly::IPAddress> Mirror::getDestinationIp() const {
  auto ip = get<switch_state_tags::destinationIp>();
  if (!ip) {
    return std::nullopt;
  }
  return network::toIPAddress(ip->toThrift());
}

std::optional<folly::IPAddress> Mirror::getSrcIp() const {
  auto ip = get<switch_state_tags::srcIp>();
  if (!ip) {
    return std::nullopt;
  }
  return network::toIPAddress(ip->toThrift());
}

Mirror::Type Mirror::type() const {
  if (!getDestinationIp()) {
    return Mirror::Type::SPAN;
  }
  if (!getTunnelUdpPorts()) {
    return Mirror::Type::ERSPAN;
  }
  return Mirror::Type::SFLOW;
}

template class ThriftStructNode<Mirror, state::MirrorFields>;

} // namespace facebook::fboss
