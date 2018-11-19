// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/state/Mirror.h"
#include "fboss/agent/state/NodeBase-defs.h"
#include "fboss/agent/state/Port.h"

namespace facebook {
namespace fboss {

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
  /* TODO - implement this */
  return folly::dynamic();
}

std::shared_ptr<Mirror> Mirror::fromFollyDynamic(
    const folly::dynamic& /*json*/) {
  /* TODO - implement this */
  return nullptr;
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
