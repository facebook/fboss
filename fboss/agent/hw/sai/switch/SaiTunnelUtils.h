// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

extern "C" {
#include <sai.h>
}

#include "fboss/agent/gen-cpp2/switch_config_types.h"

namespace facebook::fboss {

sai_tunnel_ttl_mode_t getSaiTtlMode(cfg::TunnelMode mode);
sai_tunnel_dscp_mode_t getSaiDscpMode(cfg::TunnelMode mode);
sai_tunnel_encap_ecn_mode_t getSaiEncapEcnMode(cfg::TunnelMode mode);

} // namespace facebook::fboss
