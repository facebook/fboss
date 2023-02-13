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

namespace {
constexpr auto kSrcIp = "srcIp";
constexpr auto kDstIp = "dstIp";
constexpr auto kSrcMac = "srcMac";
constexpr auto kDstMac = "dstMac";
constexpr auto kName = "name";
constexpr auto kEgressPort = "egressPort";
constexpr auto kDestinationIp = "destinationIp";
constexpr auto kTunnel = "tunnel";
constexpr auto kConfigHasEgressPort = "configHasEgressPort";
constexpr auto kIsResolved = "isResolved";
constexpr auto kDscp = "dscp";
constexpr auto kUdpSrcPort = "udpSrcPort";
constexpr auto kUdpDstPort = "udpDstPort";
constexpr auto kTruncate = "truncate";
constexpr auto kTtl = "ttl";
} // namespace

folly::dynamic MirrorTunnel::toFollyDynamic() const {
  folly::dynamic tunnel = folly::dynamic::object;
  tunnel[kSrcIp] = srcIp.str();
  tunnel[kDstIp] = dstIp.str();
  tunnel[kSrcMac] = srcMac.toString();
  tunnel[kDstMac] = dstMac.toString();
  if (udpPorts.has_value()) {
    tunnel[kUdpSrcPort] = udpPorts.value().udpSrcPort;
    tunnel[kUdpDstPort] = udpPorts.value().udpDstPort;
  }
  tunnel[kTtl] = ttl;
  return tunnel;
}

MirrorTunnel MirrorTunnel::fromFollyDynamic(const folly::dynamic& json) {
  auto tunnel = MirrorTunnel(
      folly::IPAddress(json[kSrcIp].asString()),
      folly::IPAddress(json[kDstIp].asString()),
      folly::MacAddress(json[kSrcMac].asString()),
      folly::MacAddress(json[kDstMac].asString()));

  if (json.find(kUdpSrcPort) != json.items().end()) {
    tunnel.udpPorts =
        TunnelUdpPorts(json[kUdpSrcPort].asInt(), json[kUdpDstPort].asInt());
    tunnel.greProtocol = 0;
  }
  tunnel.ttl = json.getDefault(kTtl, MirrorTunnel::kTTL).asInt();
  return tunnel;
}

folly::dynamic MirrorFields::toFollyDynamicLegacy() const {
  folly::dynamic mirrorFields = folly::dynamic::object;
  mirrorFields[kName] = name();
  if (auto port = egressPort()) {
    mirrorFields[kEgressPort] = folly::to<std::string>(port.value());
  } else {
    mirrorFields[kEgressPort] = folly::dynamic::object;
  }
  if (auto ip = destinationIp()) {
    mirrorFields[kDestinationIp] = ip.value().str();
  } else {
    mirrorFields[kDestinationIp] = folly::dynamic::object;
  }
  if (auto ip = srcIp()) {
    mirrorFields[kSrcIp] = ip.value().str();
  }
  if (auto tunnel = resolvedTunnel()) {
    mirrorFields[kTunnel] = tunnel.value().toFollyDynamic();
  } else {
    mirrorFields[kTunnel] = folly::dynamic::object;
  }
  mirrorFields[kConfigHasEgressPort] = configHasEgressPort();
  mirrorFields[kDscp] = dscp();
  mirrorFields[kTruncate] = truncate();
  auto l4Ports = udpPorts();
  if (l4Ports.has_value()) {
    mirrorFields[kUdpSrcPort] = l4Ports.value().udpSrcPort;
    mirrorFields[kUdpDstPort] = l4Ports.value().udpDstPort;
  }

  return mirrorFields;
}

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

folly::dynamic Mirror::toFollyDynamicLegacy() const {
  auto fields = MirrorFields(toThrift());
  auto mirror = fields.toFollyDynamicLegacy();
  mirror[kIsResolved] = isResolved();
  return mirror;
}

MirrorFields MirrorFields::fromFollyDynamicLegacy(const folly::dynamic& json) {
  auto name = json[kName].asString();
  auto configHasEgressPort = json[kConfigHasEgressPort].asBool();
  uint8_t dscp = cfg::switch_config_constants::DEFAULT_MIRROR_DSCP_;
  if (json.find(kDscp) != json.items().end()) {
    dscp = json[kDscp].asInt();
  }
  bool truncate = false;
  if (json.find(kTruncate) != json.items().end()) {
    truncate = json[kTruncate].asBool();
  }
  auto egressPort = std::optional<PortID>();
  auto destinationIp = std::optional<folly::IPAddress>();
  auto srcIp = std::optional<folly::IPAddress>();
  auto tunnel = std::optional<MirrorTunnel>();
  if (!json[kEgressPort].empty()) {
    egressPort = PortID(json[kEgressPort].asInt());
  }
  bool isResolved = true;
  if (!json[kDestinationIp].empty()) {
    isResolved = false;
    destinationIp = folly::IPAddress(json[kDestinationIp].asString());
  }
  if (json.find(kSrcIp) != json.items().end()) {
    srcIp = folly::IPAddress(json[kSrcIp].asString());
  }
  if (!json[kTunnel].empty()) {
    isResolved = true;
    tunnel = MirrorTunnel::fromFollyDynamic(json[kTunnel]);
  }

  std::optional<TunnelUdpPorts> udpPorts = std::nullopt;
  if (tunnel.has_value()) {
    udpPorts = tunnel.value().udpPorts;
  } else if (
      (json.find(kUdpSrcPort) != json.items().end()) &&
      (json.find(kUdpDstPort) != json.items().end())) {
    // if the tunnel is not resolved and we warm-boot,
    // src/dst udp ports are needed, which are also stored directly
    // under the mirror config
    udpPorts =
        TunnelUdpPorts(json[kUdpSrcPort].asInt(), json[kUdpDstPort].asInt());
  }
  auto fields = MirrorFields(
      name, egressPort, destinationIp, srcIp, udpPorts, dscp, truncate);
  fields.writableData().configHasEgressPort() = configHasEgressPort;
  if (tunnel) {
    fields.writableData().tunnel() = tunnel->toThrift();
  }
  fields.writableData().isResolved() = isResolved;
  return fields;
}

std::shared_ptr<Mirror> Mirror::fromFollyDynamicLegacy(
    const folly::dynamic& json) {
  auto fields = MirrorFields::fromFollyDynamicLegacy(json);
  return std::make_shared<Mirror>(fields.toThrift());
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

state::MirrorFields MirrorFields::toThrift() const {
  return data();
}

MirrorFields MirrorFields::fromThrift(state::MirrorFields const& fields) {
  return MirrorFields(fields);
}

template class ThriftStructNode<Mirror, state::MirrorFields>;

} // namespace facebook::fboss
