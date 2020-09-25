// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/state/Mirror.h"
#include "fboss/agent/state/NodeBase-defs.h"
#include "fboss/agent/state/Port.h"

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

  return tunnel;
}

folly::dynamic MirrorFields::toFollyDynamic() const {
  folly::dynamic mirrorFields = folly::dynamic::object;
  mirrorFields[kName] = name;
  if (egressPort) {
    mirrorFields[kEgressPort] = folly::to<std::string>(egressPort.value());
  } else {
    mirrorFields[kEgressPort] = folly::dynamic::object;
  }
  if (destinationIp) {
    mirrorFields[kDestinationIp] = destinationIp.value().str();
  } else {
    mirrorFields[kDestinationIp] = folly::dynamic::object;
  }
  if (srcIp) {
    mirrorFields[kSrcIp] = srcIp.value().str();
  }
  if (resolvedTunnel) {
    mirrorFields[kTunnel] = resolvedTunnel.value().toFollyDynamic();
  } else {
    mirrorFields[kTunnel] = folly::dynamic::object;
  }
  mirrorFields[kConfigHasEgressPort] = configHasEgressPort;
  mirrorFields[kDscp] = dscp;
  mirrorFields[kTruncate] = truncate;
  if (udpPorts.has_value()) {
    mirrorFields[kUdpSrcPort] = udpPorts.value().udpSrcPort;
    mirrorFields[kUdpDstPort] = udpPorts.value().udpDstPort;
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
    : NodeBaseT(
          name,
          egressPort,
          destinationIp,
          srcIp,
          udpPorts,
          dscp,
          truncate) {}

std::string Mirror::getID() const {
  return getFields()->name;
}

std::optional<PortID> Mirror::getEgressPort() const {
  return getFields()->egressPort;
}

std::optional<TunnelUdpPorts> Mirror::getTunnelUdpPorts() const {
  return getFields()->udpPorts;
}

std::optional<MirrorTunnel> Mirror::getMirrorTunnel() const {
  return getFields()->resolvedTunnel;
}

uint8_t Mirror::getDscp() const {
  return getFields()->dscp;
}

bool Mirror::getTruncate() const {
  return getFields()->truncate;
}

void Mirror::setTruncate(bool truncate) {
  writableFields()->truncate = truncate;
}

void Mirror::setEgressPort(PortID egressPort) {
  writableFields()->egressPort = egressPort;
}

void Mirror::setMirrorTunnel(const MirrorTunnel& tunnel) {
  writableFields()->resolvedTunnel = tunnel;
}

folly::dynamic Mirror::toFollyDynamic() const {
  folly::dynamic mirror = folly::dynamic::object;
  auto fields = getFields()->toFollyDynamic();
  mirror[kName] = fields[kName];
  mirror[kIsResolved] = isResolved();
  mirror[kConfigHasEgressPort] = configHasEgressPort();
  mirror[kEgressPort] = fields[kEgressPort];
  mirror[kDestinationIp] = fields[kDestinationIp];
  if (fields.find(kSrcIp) != fields.items().end()) {
    mirror[kSrcIp] = fields[kSrcIp];
  }
  mirror[kTunnel] = fields[kTunnel];
  mirror[kDscp] = fields[kDscp];
  mirror[kTruncate] = fields[kTruncate];
  // store kUdpSrcPort, kUdpDstPort in the mirror fields as well
  // as tunnel is not stored if unresolved
  // we can eventually remove storing it in tunnel struct
  // but need to keep both from warmboot perspective
  if (fields.find(kUdpSrcPort) != fields.items().end()) {
    mirror[kUdpSrcPort] = fields[kUdpSrcPort];
  }
  if (fields.find(kUdpDstPort) != fields.items().end()) {
    mirror[kUdpDstPort] = fields[kUdpDstPort];
  }
  return mirror;
}

std::shared_ptr<Mirror> Mirror::fromFollyDynamic(const folly::dynamic& json) {
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
  if (!json[kDestinationIp].empty()) {
    destinationIp = folly::IPAddress(json[kDestinationIp].asString());
  }
  if (json.find(kSrcIp) != json.items().end()) {
    srcIp = folly::IPAddress(json[kSrcIp].asString());
  }
  if (!json[kTunnel].empty()) {
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

  std::shared_ptr<Mirror> mirror = nullptr;
  if (configHasEgressPort) {
    mirror = std::make_shared<Mirror>(
        name, egressPort, destinationIp, srcIp, udpPorts, dscp, truncate);
  } else {
    mirror = std::make_shared<Mirror>(
        name, std::nullopt, destinationIp, srcIp, udpPorts, dscp, truncate);
    if (egressPort) {
      mirror->setEgressPort(egressPort.value());
    }
  }
  if (tunnel) {
    mirror->setMirrorTunnel(tunnel.value());
  }

  return mirror;
}

bool Mirror::operator==(const Mirror& rhs) const {
  return getID() == rhs.getID() &&
      (configHasEgressPort() == rhs.configHasEgressPort() ||
       getEgressPort() == rhs.getEgressPort()) &&
      getDestinationIp() == rhs.getDestinationIp() &&
      getSrcIp() == rhs.getSrcIp() &&
      getTunnelUdpPorts() == rhs.getTunnelUdpPorts() &&
      getMirrorTunnel() == rhs.getMirrorTunnel() &&
      getDscp() == rhs.getDscp() && getTruncate() == rhs.getTruncate();
}

bool Mirror::isResolved() const {
  return getMirrorTunnel().has_value() || !getDestinationIp().has_value();
}

bool Mirror::operator!=(const Mirror& rhs) const {
  return !(*this == rhs);
}

bool Mirror::configHasEgressPort() const {
  return getFields()->configHasEgressPort;
}

std::optional<folly::IPAddress> Mirror::getDestinationIp() const {
  return getFields()->destinationIp;
}

std::optional<folly::IPAddress> Mirror::getSrcIp() const {
  return getFields()->srcIp;
}

Mirror::Type Mirror::type() const {
  if (!getFields()->destinationIp) {
    return Mirror::Type::SPAN;
  }
  if (!getFields()->udpPorts) {
    return Mirror::Type::ERSPAN;
  }
  return Mirror::Type::SFLOW;
}

template class NodeBaseT<Mirror, MirrorFields>;

} // namespace facebook::fboss
