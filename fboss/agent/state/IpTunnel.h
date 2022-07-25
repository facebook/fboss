// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/agent/state/NodeBase.h"
#include "fboss/agent/state/Thrifty.h"
#include "fboss/agent/types.h"
#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"

#include <optional>

namespace facebook::fboss {

struct IpTunnelFields
    : public ThriftyFields<IpTunnelFields, state::IpTunnelFields> {
  // TODO(yijunli): BetterThriftyFields?
  explicit IpTunnelFields(std::string id) {
    auto& data = writableData();
    *data.ipTunnelId() = id;
  }

  template <typename Fn>
  void forEachChild(Fn /*fn*/) {}

  state::IpTunnelFields toThrift() const override {
    return data();
  }
  static IpTunnelFields fromThrift(const state::IpTunnelFields& ipTunnelThrift);
};

class IpTunnel
    : public ThriftyBaseT<state::IpTunnelFields, IpTunnel, IpTunnelFields> {
 private:
  // Inherit the constructors required for clone()
  using ThriftyBaseT<state::IpTunnelFields, IpTunnel, IpTunnelFields>::
      ThriftyBaseT;
  friend class CloneAllocator;
};
} // namespace facebook::fboss
