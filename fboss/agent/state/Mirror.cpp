// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/state/Mirror.h"
#include "fboss/agent/state/NodeBase-defs.h"
#include "fboss/agent/state/Port.h"

namespace facebook {
namespace fboss {

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
} // namespace

folly::dynamic MirrorTunnel::toFollyDynamic() const {
  folly::dynamic tunnel = folly::dynamic::object;
  tunnel[kSrcIp] = srcIp.str();
  tunnel[kDstIp] = dstIp.str();
  tunnel[kSrcMac] = srcMac.toString();
  tunnel[kDstMac] = dstMac.toString();
  return tunnel;
}

MirrorTunnel MirrorTunnel::fromFollyDynamic(const folly::dynamic& json) {
  return MirrorTunnel(
      folly::IPAddress(json[kSrcIp].asString()),
      folly::IPAddress(json[kDstIp].asString()),
      folly::MacAddress(json[kSrcMac].asString()),
      folly::MacAddress(json[kDstMac].asString()));
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
  if (resolvedTunnel) {
    mirrorFields[kTunnel] = resolvedTunnel.value().toFollyDynamic();
  } else {
    mirrorFields[kTunnel] = folly::dynamic::object;
  }
  mirrorFields[kConfigHasEgressPort] = configHasEgressPort;
  return mirrorFields;
}

Mirror::Mirror(
    std::string name,
    folly::Optional<PortID> egressPort,
    folly::Optional<folly::IPAddress> destinationIp)
    : NodeBaseT(name, egressPort, destinationIp) {}

std::string Mirror::getID() const {
  return getFields()->name;
}

folly::Optional<PortID> Mirror::getEgressPort() const {
  return getFields()->egressPort;
}

folly::Optional<MirrorTunnel> Mirror::getMirrorTunnel() const {
  return getFields()->resolvedTunnel;
}

void Mirror::setEgressPort(PortID egressPort) {
  writableFields()->egressPort.assign(egressPort);
}

void Mirror::setMirrorTunnel(const MirrorTunnel& tunnel) {
  writableFields()->resolvedTunnel.assign(tunnel);
}

folly::dynamic Mirror::toFollyDynamic() const {
  folly::dynamic mirror = folly::dynamic::object;
  auto fields = getFields()->toFollyDynamic();
  mirror[kName] = fields[kName];
  mirror[kIsResolved] = isResolved();
  mirror[kConfigHasEgressPort] = configHasEgressPort();
  mirror[kEgressPort] = fields[kEgressPort];
  mirror[kDestinationIp] = fields[kDestinationIp];
  mirror[kTunnel] = fields[kTunnel];
  return mirror;
}

std::shared_ptr<Mirror> Mirror::fromFollyDynamic(const folly::dynamic& json) {
  auto name = json[kName].asString();
  auto configHasEgressPort = json[kConfigHasEgressPort].asBool();
  auto egressPort = folly::Optional<PortID>();
  auto destinationIp = folly::Optional<folly::IPAddress>();
  auto tunnel = folly::Optional<MirrorTunnel>();
  if (!json[kEgressPort].empty()) {
    egressPort.assign(PortID(json[kEgressPort].asInt()));
  }
  if (!json[kDestinationIp].empty()) {
    destinationIp.assign(folly::IPAddress(json[kDestinationIp].asString()));
  }
  if (!json[kTunnel].empty()) {
    tunnel.assign(MirrorTunnel::fromFollyDynamic(json[kTunnel]));
  }

  std::shared_ptr<Mirror> mirror = nullptr;
  if (configHasEgressPort) {
    mirror = std::make_shared<Mirror>(name, egressPort, destinationIp);
  } else {
    mirror = std::make_shared<Mirror>(name, folly::none, destinationIp);
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
      getMirrorTunnel() == rhs.getMirrorTunnel();
}

bool Mirror::isResolved() const {
  return getMirrorTunnel().hasValue() ||
      !getDestinationIp().hasValue();
}

bool Mirror::operator!=(const Mirror& rhs) const {
  return !(*this == rhs);
}

bool Mirror::configHasEgressPort() const {
  return getFields()->configHasEgressPort;
}

folly::Optional<folly::IPAddress> Mirror::getDestinationIp() const {
  return getFields()->destinationIp;
}

template class NodeBaseT<Mirror, MirrorFields>;

} // namespace fboss
} // namespace facebook
