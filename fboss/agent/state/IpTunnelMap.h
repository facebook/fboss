// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <string>
#pragma once

#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/agent/state/IpTunnel.h"
#include "fboss/agent/state/NodeMap.h"
#include "fboss/agent/state/Thrifty.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {

class SwitchState;

using IpTunnelMapTraits = NodeMapTraits<std::string, IpTunnel>;

/*
 * A container for all the present IpTunnels
 */
class IpTunnelMap
    : public ThriftyNodeMapT<
          IpTunnelMap,
          IpTunnelMapTraits,
          ThriftyNodeMapTraits<std::string, state::IpTunnelFields>> {
 public:
  IpTunnelMap();
  ~IpTunnelMap() override;

  const std::shared_ptr<IpTunnel>& getTunnel(std::string id) const {
    return getNode(id);
  }
  std::shared_ptr<IpTunnel> getTunnelIf(std::string id) const {
    return getNodeIf(id);
  }

  void addTunnel(const std::shared_ptr<IpTunnel>& Tunnel);
  void updateTunnel(const std::shared_ptr<IpTunnel>& Tunnel);
  void removeTunnel(std::string id);

 private:
  // Inherit the constructors required for clone()
  using ThriftyNodeMapT::ThriftyNodeMapT;
  friend class CloneAllocator;
};
} // namespace facebook::fboss
