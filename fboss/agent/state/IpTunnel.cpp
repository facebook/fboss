// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/state/IpTunnel.h"

namespace facebook::fboss {
IpTunnelFields IpTunnelFields::fromThrift(
    const state::IpTunnelFields& ipTunnelThrift) {
  IpTunnelFields ipTunnel(*ipTunnelThrift.ipTunnelId());
  ipTunnel.writableData() = ipTunnelThrift;
  return ipTunnel;
}

} // namespace facebook::fboss
