// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/state/IpTunnelMap.h"

#include "fboss/agent/state/IpTunnel.h"
#include "fboss/agent/state/NodeMap-defs.h"
#include "fboss/agent/state/SwitchState.h"

namespace facebook::fboss {

IpTunnelMap::IpTunnelMap() {}

IpTunnelMap::~IpTunnelMap() {}

void IpTunnelMap::addTunnel(const std::shared_ptr<IpTunnel>& tunnel) {
  addNode(tunnel);
}

void IpTunnelMap::updateTunnel(const std::shared_ptr<IpTunnel>& tunnel) {
  updateNode(tunnel);
}

void IpTunnelMap::removeTunnel(std::string id) {
  removeNodeIf(id);
}

MultiSwitchIpTunnelMap* MultiSwitchIpTunnelMap::modify(
    std::shared_ptr<SwitchState>* state) {
  return SwitchState::modify<switch_state_tags::ipTunnelMaps>(state);
}

template class ThriftMapNode<IpTunnelMap, IpTunnelMapTraits>;
} // namespace facebook::fboss
