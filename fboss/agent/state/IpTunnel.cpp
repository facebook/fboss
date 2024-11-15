// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/state/IpTunnel.h"

namespace facebook::fboss {

template struct ThriftStructNode<IpTunnel, state::IpTunnelFields>;

} // namespace facebook::fboss
