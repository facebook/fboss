// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/hw/sai/tracer/TunnelApiTracer.h"
#include <typeindex>
#include <utility>

#include "fboss/agent/hw/sai/api/TunnelApi.h"
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
    SAI_ATTR_MAP(P2MPTunnelTerm, Type),
    SAI_ATTR_MAP(P2MPTunnelTerm, VrId),
    SAI_ATTR_MAP(P2MPTunnelTerm, DstIp),
    SAI_ATTR_MAP(P2MPTunnelTerm, DstIpMask),
    SAI_ATTR_MAP(P2MPTunnelTerm, SrcIp),
    SAI_ATTR_MAP(P2MPTunnelTerm, SrcIpMask),
    SAI_ATTR_MAP(P2MPTunnelTerm, TunnelType),
    SAI_ATTR_MAP(P2MPTunnelTerm, ActionTunnelId),
};
} // namespace

namespace facebook::fboss {

WRAP_CREATE_FUNC(tunnel, SAI_OBJECT_TYPE_TUNNEL, tunnel);
WRAP_REMOVE_FUNC(tunnel, SAI_OBJECT_TYPE_TUNNEL, tunnel);
WRAP_SET_ATTR_FUNC(tunnel, SAI_OBJECT_TYPE_TUNNEL, tunnel);
WRAP_GET_ATTR_FUNC(tunnel, SAI_OBJECT_TYPE_TUNNEL, tunnel);

WRAP_CREATE_FUNC(
    tunnel_term_table_entry,
    SAI_OBJECT_TYPE_TUNNEL_TERM_TABLE_ENTRY,
    tunnel);
WRAP_REMOVE_FUNC(
    tunnel_term_table_entry,
    SAI_OBJECT_TYPE_TUNNEL_TERM_TABLE_ENTRY,
    tunnel);
WRAP_SET_ATTR_FUNC(
    tunnel_term_table_entry,
    SAI_OBJECT_TYPE_TUNNEL_TERM_TABLE_ENTRY,
    tunnel);
WRAP_GET_ATTR_FUNC(
    tunnel_term_table_entry,
    SAI_OBJECT_TYPE_TUNNEL_TERM_TABLE_ENTRY,
    tunnel);

sai_tunnel_api_t* wrappedTunnelApi() {
  static sai_tunnel_api_t tunnelWrappers;

  tunnelWrappers.create_tunnel = &wrap_create_tunnel;
  tunnelWrappers.remove_tunnel = &wrap_remove_tunnel;
  tunnelWrappers.set_tunnel_attribute = &wrap_set_tunnel_attribute;
  tunnelWrappers.get_tunnel_attribute = &wrap_get_tunnel_attribute;

  tunnelWrappers.create_tunnel_term_table_entry =
      &wrap_create_tunnel_term_table_entry;
  tunnelWrappers.remove_tunnel_term_table_entry =
      &wrap_remove_tunnel_term_table_entry;
  tunnelWrappers.get_tunnel_term_table_entry_attribute =
      &wrap_get_tunnel_term_table_entry_attribute;
  tunnelWrappers.set_tunnel_term_table_entry_attribute =
      &wrap_set_tunnel_term_table_entry_attribute;

  return &tunnelWrappers;
}

SET_SAI_ATTRIBUTES(Tunnel);
SET_SAI_ATTRIBUTES(TunnelTerm);

} // namespace facebook::fboss
