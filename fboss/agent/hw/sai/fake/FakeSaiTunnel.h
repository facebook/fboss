// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <optional>
#include "fboss/agent/hw/sai/fake/FakeManager.h"

extern "C" {
#include <sai.h>
}

namespace facebook::fboss {

class FakeSaiTunnel {
 public:
  FakeSaiTunnel(
      sai_tunnel_type_t type,
      sai_object_id_t underlay,
      sai_object_id_t overlay,
      std::optional<sai_tunnel_ttl_mode_t> ttl,
      std::optional<sai_tunnel_dscp_mode_t> dscp,
      std::optional<sai_tunnel_decap_ecn_mode_t> ecn)
      : type(type),
        underlay(underlay),
        overlay(overlay),
        ttlMode(ttl),
        dscpMode(dscp),
        ecnMode(ecn) {}
  sai_object_id_t id;
  sai_tunnel_type_t type;
  sai_object_id_t underlay;
  sai_object_id_t overlay;
  std::optional<sai_tunnel_ttl_mode_t> ttlMode;
  std::optional<sai_tunnel_dscp_mode_t> dscpMode;
  std::optional<sai_tunnel_decap_ecn_mode_t> ecnMode;
};

class FakeSaiTunnelTerm {
 public:
  FakeSaiTunnelTerm(
      sai_tunnel_term_table_entry_type_t type,
      sai_object_id_t vrId,
      sai_ip_address_t dstIp,
      sai_tunnel_type_t tunnelType,
      sai_object_id_t tunnelId,
      std::optional<sai_ip_address_t> srcIp,
      std::optional<sai_ip_address_t> dstIpMask,
      std::optional<sai_ip_address_t> srcIpMask)
      : type(type),
        vrId(vrId),
        dstIp(dstIp),
        tunnelType(tunnelType),
        tunnelId(tunnelId),
        srcIp(srcIp),
        dstIpMask(dstIpMask),
        srcIpMask(srcIpMask) {}
  sai_object_id_t id;
  sai_tunnel_term_table_entry_type_t type;
  sai_object_id_t vrId;
  sai_ip_address_t dstIp;
  sai_tunnel_type_t tunnelType;
  sai_object_id_t tunnelId;
  std::optional<sai_ip_address_t> srcIp;
  std::optional<sai_ip_address_t> dstIpMask;
  std::optional<sai_ip_address_t> srcIpMask;
};

using FakeTunnelManager = FakeManager<sai_object_id_t, FakeSaiTunnel>;
using FakeTunnelTermManager = FakeManager<sai_object_id_t, FakeSaiTunnelTerm>;

void populate_tunnel_api(sai_tunnel_api_t** tunnel_api);

} // namespace facebook::fboss
