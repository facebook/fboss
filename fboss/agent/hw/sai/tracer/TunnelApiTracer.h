// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/hw/sai/tracer/SaiTracer.h"
#include "fboss/agent/hw/sai/tracer/Utils.h"

namespace facebook::fboss {

sai_tunnel_api_t* wrappedTunnelApi();

SET_ATTRIBUTE_FUNC_DECLARATION(Tunnel);
SET_ATTRIBUTE_FUNC_DECLARATION(TunnelTerm);

} // namespace facebook::fboss
