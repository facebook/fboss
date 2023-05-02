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
  if (!isPublished()) {
    CHECK(!(*state)->isPublished());
    return this;
  }

  SwitchState::modify(state);
  auto newMnpuMap = clone();
  for (auto mnitr = cbegin(); mnitr != cend(); ++mnitr) {
    (*newMnpuMap)[mnitr->first] = mnitr->second->clone();
  }
  auto* ptr = newMnpuMap.get();
  (*state)->resetTunnels(std::move(newMnpuMap));
  return ptr;
}

template class ThriftMapNode<IpTunnelMap, IpTunnelMapTraits>;
} // namespace facebook::fboss
