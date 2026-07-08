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

sai_tunnel_encap_ecn_mode_t getSaiEncapEcnMode(cfg::TunnelMode mode) {
  switch (mode) {
    case cfg::TunnelMode::UNIFORM:
      return SAI_TUNNEL_ENCAP_ECN_MODE_STANDARD;
    case cfg::TunnelMode::PIPE:
    case cfg::TunnelMode::USER:
      return SAI_TUNNEL_ENCAP_ECN_MODE_USER_DEFINED;
  }
  throw FbossError("Failed to convert encap ECN mode to SAI type: ", mode);
}

} // namespace facebook::fboss
