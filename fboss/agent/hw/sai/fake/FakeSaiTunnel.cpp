// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/hw/sai/fake/FakeSaiTunnel.h"
#include "fboss/agent/hw/sai/fake/FakeSai.h"

namespace {

using facebook::fboss::FakeSai;

sai_status_t create_tunnel_fn(
    sai_object_id_t* tunnel_id,
    sai_object_id_t /* switch_id */,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  auto fs = FakeSai::getInstance();
  sai_tunnel_type_t type;
  sai_object_id_t underlay;
  sai_object_id_t overlay;
  std::optional<sai_tunnel_ttl_mode_t> ttlMode{
      SAI_TUNNEL_TTL_MODE_UNIFORM_MODEL};
  std::optional<sai_tunnel_dscp_mode_t> dscpMode{
      SAI_TUNNEL_DSCP_MODE_UNIFORM_MODEL};
  std::optional<sai_tunnel_decap_ecn_mode_t> ecnMode{
      SAI_TUNNEL_DECAP_ECN_MODE_STANDARD};
  for (int i = 0; i < attr_count; i++) {
    switch (attr_list[i].id) {
      case SAI_TUNNEL_ATTR_TYPE:
        type = static_cast<sai_tunnel_type_t>(attr_list[i].value.s32);
        break;
      case SAI_TUNNEL_ATTR_UNDERLAY_INTERFACE:
        if (type == SAI_TUNNEL_TYPE_IPINIP) {
          // There are a few conditions that can work with this attr
          // We only support IPinIP for now
          underlay = attr_list[i].value.oid;
        } else {
          return SAI_STATUS_INVALID_PARAMETER;
        }
        break;
      case SAI_TUNNEL_ATTR_OVERLAY_INTERFACE:
        if (type == SAI_TUNNEL_TYPE_IPINIP) {
          // only support IPinIP for now
          overlay = attr_list[i].value.oid;
        } else {
          return SAI_STATUS_INVALID_PARAMETER;
        }
        break;
      case SAI_TUNNEL_ATTR_DECAP_TTL_MODE:
        ttlMode = static_cast<sai_tunnel_ttl_mode_t>(attr_list[i].value.s32);
        break;
      case SAI_TUNNEL_ATTR_DECAP_DSCP_MODE:
        dscpMode = static_cast<sai_tunnel_dscp_mode_t>(attr_list[i].value.s32);
        break;
      case SAI_TUNNEL_ATTR_DECAP_ECN_MODE:
        ecnMode =
            static_cast<sai_tunnel_decap_ecn_mode_t>(attr_list[i].value.s32);
        break;
      default:
        return SAI_STATUS_INVALID_PARAMETER;
    }
  }
  if (!type || !underlay || !overlay) {
    return SAI_STATUS_INVALID_PARAMETER;
  }
  *tunnel_id = fs->tunnelManager.create(
      type,
      underlay,
      overlay,
      ttlMode.value(),
      dscpMode.value(),
      ecnMode.value());
  return SAI_STATUS_SUCCESS;
}

sai_status_t remove_tunnel_fn(sai_object_id_t tunnel_id) {
  auto fs = FakeSai::getInstance();
  fs->tunnelManager.remove(tunnel_id);
  return SAI_STATUS_SUCCESS;
}

sai_status_t get_tunnel_attribute_fn(
    sai_object_id_t tunnel_id,
    uint32_t attr_count,
    sai_attribute_t* attr) {
  auto fs = FakeSai::getInstance();
  auto tunnel = fs->tunnelManager.get(tunnel_id);
  for (int i = 0; i < attr_count; i++) {
    switch (attr[i].id) {
      case SAI_TUNNEL_ATTR_TYPE:
        attr[i].value.s32 = tunnel.type;
        break;
      case SAI_TUNNEL_ATTR_UNDERLAY_INTERFACE:
        attr[i].value.oid = tunnel.underlay;
        break;
      case SAI_TUNNEL_ATTR_OVERLAY_INTERFACE:
        attr[i].value.oid = tunnel.overlay;
        break;
      case SAI_TUNNEL_ATTR_DECAP_TTL_MODE:
        attr[i].value.s32 = tunnel.ttlMode.value();
        break;
      case SAI_TUNNEL_ATTR_DECAP_DSCP_MODE:
        attr[i].value.s32 = tunnel.dscpMode.value();
        break;
      case SAI_TUNNEL_ATTR_DECAP_ECN_MODE:
        attr[i].value.s32 = tunnel.ecnMode.value();
        break;
      default:
        return SAI_STATUS_INVALID_PARAMETER;
    }
  }
  return SAI_STATUS_SUCCESS;
}

sai_status_t set_tunnel_attribute_fn(
    sai_object_id_t tunnel_id,
    const sai_attribute_t* attr) {
  // #TODO: attributes marked CREATE_ONLY should not be set in
  // this function, for now every attribute can be set
  auto fs = FakeSai::getInstance();
  auto tunnel = fs->tunnelManager.get(tunnel_id);
  switch (attr->id) {
    case SAI_TUNNEL_ATTR_TYPE:
      // CREATE_ONLY
      tunnel.type = static_cast<sai_tunnel_type_t>(attr->value.s32);
      break;
    case SAI_TUNNEL_ATTR_UNDERLAY_INTERFACE:
      // CREATE_ONLY
      tunnel.underlay = attr->value.oid;
      break;
    case SAI_TUNNEL_ATTR_OVERLAY_INTERFACE:
      // CREATE_ONLY
      tunnel.overlay = attr->value.oid;
      break;
    case SAI_TUNNEL_ATTR_DECAP_TTL_MODE:
      tunnel.ttlMode = static_cast<sai_tunnel_ttl_mode_t>(attr->value.s32);
      break;
    case SAI_TUNNEL_ATTR_DECAP_DSCP_MODE:
      tunnel.dscpMode = static_cast<sai_tunnel_dscp_mode_t>(attr->value.s32);
      break;
    case SAI_TUNNEL_ATTR_DECAP_ECN_MODE:
      // CREATE_ONLY
      tunnel.ecnMode =
          static_cast<sai_tunnel_decap_ecn_mode_t>(attr->value.s32);
      break;
    default:
      return SAI_STATUS_INVALID_PARAMETER;
  }

  return SAI_STATUS_SUCCESS;
}

sai_status_t create_tunnel_term_table_entry_fn(
    sai_object_id_t* tunnel_term_table_entry_id,
    sai_object_id_t /* switch_id */,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  auto fs = FakeSai::getInstance();
  sai_tunnel_term_table_entry_type_t type;
  sai_object_id_t vrId;
  sai_ip_address_t dstIp;
  sai_ip_address_t srcIp;
  sai_tunnel_type_t tunnelType;
  sai_object_id_t tunnelId;
  std::optional<sai_ip_address_t> dstIpMask;
  std::optional<sai_ip_address_t> srcIpMask;
  for (int i = 0; i < attr_count; i++) {
    switch (attr_list[i].id) {
      case SAI_TUNNEL_TERM_TABLE_ENTRY_ATTR_TYPE:
        type = static_cast<sai_tunnel_term_table_entry_type_t>(
            attr_list[i].value.s32);
        break;
      case SAI_TUNNEL_TERM_TABLE_ENTRY_ATTR_VR_ID:
        vrId = attr_list[i].value.oid;
        break;
      case SAI_TUNNEL_TERM_TABLE_ENTRY_ATTR_DST_IP:
        dstIp = attr_list[i].value.ipaddr;
        break;
      case SAI_TUNNEL_TERM_TABLE_ENTRY_ATTR_DST_IP_MASK:
        dstIpMask = attr_list[i].value.ipaddr;
        break;
      case SAI_TUNNEL_TERM_TABLE_ENTRY_ATTR_SRC_IP:
        srcIp = attr_list[i].value.ipaddr;
        break;
      case SAI_TUNNEL_TERM_TABLE_ENTRY_ATTR_SRC_IP_MASK:
        srcIpMask = attr_list[i].value.ipaddr;
        break;
      case SAI_TUNNEL_TERM_TABLE_ENTRY_ATTR_TUNNEL_TYPE:
        tunnelType = static_cast<sai_tunnel_type_t>(attr_list[i].value.s32);
        break;
      case SAI_TUNNEL_TERM_TABLE_ENTRY_ATTR_ACTION_TUNNEL_ID:
        tunnelId = attr_list[i].value.oid;
        break;
      default:
        return SAI_STATUS_INVALID_PARAMETER;
    }
  }
  if (!type || !vrId || !(dstIp.addr.ip4 || dstIp.addr.ip6) ||
      !(srcIp.addr.ip4 || srcIp.addr.ip6) || !tunnelType || !tunnelId) {
    return SAI_STATUS_INVALID_PARAMETER;
  }
  *tunnel_term_table_entry_id = fs->tunnelTermManager.create(
      type,
      vrId,
      dstIp,
      srcIp,
      tunnelType,
      tunnelId,
      dstIpMask.value(),
      srcIpMask.value());
  return SAI_STATUS_SUCCESS;
}

sai_status_t remove_tunnel_term_table_entry_fn(
    sai_object_id_t tunnel_term_table_entry_id) {
  auto fs = FakeSai::getInstance();
  fs->tunnelTermManager.remove(tunnel_term_table_entry_id);
  return SAI_STATUS_SUCCESS;
}

sai_status_t get_tunnel_term_table_entry_attribute_fn(
    sai_object_id_t tunnel_term_table_entry_id,
    uint32_t attr_count,
    sai_attribute_t* attr) {
  auto fs = FakeSai::getInstance();
  auto term = fs->tunnelTermManager.get(tunnel_term_table_entry_id);
  for (int i = 0; i < attr_count; i++) {
    switch (attr[i].id) {
      case SAI_TUNNEL_TERM_TABLE_ENTRY_ATTR_TYPE:
        attr[i].value.s32 = term.type;
        break;
      case SAI_TUNNEL_TERM_TABLE_ENTRY_ATTR_VR_ID:
        attr[i].value.oid = term.vrId;
        break;
      case SAI_TUNNEL_TERM_TABLE_ENTRY_ATTR_DST_IP:
        attr[i].value.ipaddr = term.dstIp;
        break;
      case SAI_TUNNEL_TERM_TABLE_ENTRY_ATTR_DST_IP_MASK:
        attr[i].value.ipaddr = term.dstIpMask.value();
        break;
      case SAI_TUNNEL_TERM_TABLE_ENTRY_ATTR_SRC_IP:
        attr[i].value.ipaddr = term.srcIp;
        break;
      case SAI_TUNNEL_TERM_TABLE_ENTRY_ATTR_SRC_IP_MASK:
        attr[i].value.ipaddr = term.srcIpMask.value();
        break;
      case SAI_TUNNEL_TERM_TABLE_ENTRY_ATTR_TUNNEL_TYPE:
        attr[i].value.s32 = term.tunnelType;
        break;
      case SAI_TUNNEL_TERM_TABLE_ENTRY_ATTR_ACTION_TUNNEL_ID:
        attr[i].value.oid = term.tunnelId;
        break;
      default:
        return SAI_STATUS_INVALID_PARAMETER;
    }
  }
  return SAI_STATUS_SUCCESS;
}

sai_status_t set_tunnel_term_table_entry_attribute_fn(
    sai_object_id_t tunnel_term_table_entry_id,
    const sai_attribute_t* attr) {
  auto fs = FakeSai::getInstance();
  auto term = fs->tunnelTermManager.get(tunnel_term_table_entry_id);
  switch (attr->id) {
    case SAI_TUNNEL_TERM_TABLE_ENTRY_ATTR_TYPE:
      term.type =
          static_cast<sai_tunnel_term_table_entry_type_t>(attr->value.s32);
      break;
    case SAI_TUNNEL_TERM_TABLE_ENTRY_ATTR_VR_ID:
      // CREATE_ONLY
      term.vrId = attr->value.oid;
      break;
    case SAI_TUNNEL_TERM_TABLE_ENTRY_ATTR_DST_IP:
      // CREATE_ONLY
      term.dstIp = attr->value.ipaddr;
      break;
    case SAI_TUNNEL_TERM_TABLE_ENTRY_ATTR_DST_IP_MASK:
      // CREATE_ONLY
      term.dstIpMask.value() = attr->value.ipaddr;
      break;
    case SAI_TUNNEL_TERM_TABLE_ENTRY_ATTR_SRC_IP:
      // CREATE_ONLY
      term.srcIp = attr->value.ipaddr;
      break;
    case SAI_TUNNEL_TERM_TABLE_ENTRY_ATTR_SRC_IP_MASK:
      // CREATE_ONLY
      term.srcIpMask.value() = attr->value.ipaddr;
      break;
    case SAI_TUNNEL_TERM_TABLE_ENTRY_ATTR_TUNNEL_TYPE:
      // CREATE_ONLY
      term.tunnelType = static_cast<sai_tunnel_type_t>(attr->value.s32);
      break;
    case SAI_TUNNEL_TERM_TABLE_ENTRY_ATTR_ACTION_TUNNEL_ID:
      // CREATE_ONLY
      term.tunnelId = attr->value.oid;
      break;
    default:
      return SAI_STATUS_INVALID_PARAMETER;
  }
  return SAI_STATUS_SUCCESS;
}

} // namespace

namespace facebook::fboss {
static sai_tunnel_api_t _tunnel_api;

void populate_tunnel_api(sai_tunnel_api_t** tunnel_api) {
  _tunnel_api.create_tunnel = &create_tunnel_fn;
  _tunnel_api.remove_tunnel = &remove_tunnel_fn;
  _tunnel_api.get_tunnel_attribute = &get_tunnel_attribute_fn;
  _tunnel_api.set_tunnel_attribute = &set_tunnel_attribute_fn;

  _tunnel_api.create_tunnel_term_table_entry =
      &create_tunnel_term_table_entry_fn;
  _tunnel_api.remove_tunnel_term_table_entry =
      &remove_tunnel_term_table_entry_fn;
  _tunnel_api.get_tunnel_term_table_entry_attribute =
      &get_tunnel_term_table_entry_attribute_fn;
  _tunnel_api.set_tunnel_term_table_entry_attribute =
      &set_tunnel_term_table_entry_attribute_fn;

  *tunnel_api = &_tunnel_api;
}

} // namespace facebook::fboss
