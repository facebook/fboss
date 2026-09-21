// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/state/Srv6TunnelMap.h"

#include "fboss/agent/state/NodeMap-defs.h"
#include "fboss/agent/state/Srv6Tunnel.h"
#include "fboss/agent/state/SwitchState.h"

namespace facebook::fboss {

Srv6TunnelMap::Srv6TunnelMap() = default;

Srv6TunnelMap::~Srv6TunnelMap() = default;

void Srv6TunnelMap::addTunnel(const std::shared_ptr<Srv6Tunnel>& tunnel) {
  addNode(tunnel);
}

void Srv6TunnelMap::updateTunnel(const std::shared_ptr<Srv6Tunnel>& tunnel) {
  updateNode(tunnel);
}

void Srv6TunnelMap::removeTunnel(std::string id) {
  removeNodeIf(id);
}

MultiSwitchSrv6TunnelMap* MultiSwitchSrv6TunnelMap::modify(
    std::shared_ptr<SwitchState>* state) {
  return SwitchState::modify<switch_state_tags::srv6TunnelMaps>(state);
}

template struct ThriftMapNode<Srv6TunnelMap, Srv6TunnelMapTraits>;
} // namespace facebook::fboss
