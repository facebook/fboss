// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/state/Mirror.h"
#include "fboss/agent/state/NodeBase-defs.h"
#include "fboss/agent/state/Port.h"

namespace facebook {
namespace fboss {

Mirror::Mirror(
    std::string name,
    folly::Optional<PortID> mirrorEgressPort,
    folly::Optional<folly::IPAddress> tunnelDestinationIp)
    : NodeBaseT(name, mirrorEgressPort, tunnelDestinationIp) {}

std::string Mirror::getID() const {
  return getFields()->name_;
}

folly::Optional<PortID> Mirror::getMirrorEgressPort() const {
  return getFields()->egressPort_;
}

folly::Optional<MirrorTunnel> Mirror::getMirrorTunnel() const {
  return getFields()->resolvedTunnel_;
}

void Mirror::setMirrorEgressPort(PortID egressPort) {
  writableFields()->egressPort_.assign(egressPort);
}

void Mirror::setMirrorTunnel(MirrorTunnel tunnel) {
  writableFields()->resolvedTunnel_.assign(tunnel);
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
       getMirrorEgressPort() == rhs.getMirrorEgressPort()) &&
      getMirrorTunnelDestinationIp() == rhs.getMirrorTunnelDestinationIp() &&
      getMirrorTunnel() == rhs.getMirrorTunnel();
}

bool Mirror::isResolved() const {
  return getMirrorTunnel().hasValue() ||
      !getMirrorTunnelDestinationIp().hasValue();
}

bool Mirror::operator!=(const Mirror& rhs) const {
  return !(*this == rhs);
}

bool Mirror::configHasEgressPort() const {
  return getFields()->configHasEgressPort_;
}

folly::Optional<folly::IPAddress> Mirror::getMirrorTunnelDestinationIp() const {
  return getFields()->tunnelDestinationIp_;
}

template class NodeBaseT<Mirror, MirrorFields>;

} // namespace fboss
} // namespace facebook
