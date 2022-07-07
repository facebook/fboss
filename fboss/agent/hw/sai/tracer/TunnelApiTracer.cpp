// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <typeindex>
#include <utility>

#include "fboss/agent/hw/sai/api/TunnelApi.h"
#include "fboss/agent/hw/sai/tracer/TunnelApiTracer.h"
#include "fboss/agent/hw/sai/tracer/Utils.h"

using folly::to;

namespace {
std::map<int32_t, std::pair<std::string, std::size_t>> _TunnelMap{
    SAI_ATTR_MAP(Tunnel, Type),
    SAI_ATTR_MAP(Tunnel, UnderlayInterface),
    SAI_ATTR_MAP(Tunnel, OverlayInterface),
    SAI_ATTR_MAP(Tunnel, DecapTtlMode),
    SAI_ATTR_MAP(Tunnel, DecapDscpMode),
    SAI_ATTR_MAP(Tunnel, DecapEcnMode),
};

std::map<int32_t, std::pair<std::string, std::size_t>> _TunnelTermMap{
    SAI_ATTR_MAP(TunnelTerm, Type),
    SAI_ATTR_MAP(TunnelTerm, EntryAttrVrId),
    SAI_ATTR_MAP(TunnelTerm, DstIp),
    SAI_ATTR_MAP(TunnelTerm, DstIpMask),
    SAI_ATTR_MAP(TunnelTerm, SrcIp),
    SAI_ATTR_MAP(TunnelTerm, SrcIpMask),
    SAI_ATTR_MAP(TunnelTerm, TunnelType),
    SAI_ATTR_MAP(TunnelTerm, ActionTunnelId),
};
} // namespace

namespace facebook::fboss {
sai_tunnel_api_t* wrappedTunnelApi() {
  static sai_tunnel_api_t tunnelWrappers;

  return &tunnelWrappers;
}

SET_SAI_ATTRIBUTES(Tunnel);
SET_SAI_ATTRIBUTES(TunnelTerm);

} // namespace facebook::fboss
