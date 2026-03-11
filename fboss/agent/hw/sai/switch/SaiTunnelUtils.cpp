// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/hw/sai/switch/SaiTunnelUtils.h"

#include "fboss/agent/FbossError.h"

namespace facebook::fboss {

sai_tunnel_ttl_mode_t getSaiTtlMode(cfg::TunnelMode mode) {
  switch (mode) {
    case cfg::TunnelMode::UNIFORM:
      return SAI_TUNNEL_TTL_MODE_UNIFORM_MODEL;
    case cfg::TunnelMode::PIPE:
      return SAI_TUNNEL_TTL_MODE_PIPE_MODEL;
    case cfg::TunnelMode::USER:
      break;
  }
  throw FbossError("Failed to convert TTL mode to SAI type: ", mode);
}

sai_tunnel_dscp_mode_t getSaiDscpMode(cfg::TunnelMode mode) {
  switch (mode) {
    case cfg::TunnelMode::UNIFORM:
      return SAI_TUNNEL_DSCP_MODE_UNIFORM_MODEL;
    case cfg::TunnelMode::PIPE:
      return SAI_TUNNEL_DSCP_MODE_PIPE_MODEL;
    case cfg::TunnelMode::USER:
      break;
  }
  throw FbossError("Failed to convert DSCP mode to SAI type: ", mode);
}

} // namespace facebook::fboss
