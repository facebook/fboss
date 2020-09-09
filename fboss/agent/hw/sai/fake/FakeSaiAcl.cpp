/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/fake/FakeSaiAcl.h"

#include "fboss/agent/hw/sai/api/AddressUtil.h"
#include "fboss/agent/hw/sai/fake/FakeSai.h"

using facebook::fboss::FakeSai;

sai_status_t create_acl_table_fn(
    sai_object_id_t* acl_table_id,
    sai_object_id_t /*switch_id */,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  auto fs = FakeSai::getInstance();

  std::optional<sai_int32_t> stage;
  std::vector<int32_t> bindPointTypeList;
  std::vector<int32_t> actionTypeList;
  bool fieldSrcIpV6 = 0;
  bool fieldDstIpV6 = 0;
  bool fieldSrcIpV4 = 0;
  bool fieldDstIpV4 = 0;
  bool fieldL4SrcPort = 0;
  bool fieldL4DstPort = 0;
  bool fieldIpProtocol = 0;
  bool fieldTcpFlags = 0;
  bool fieldSrcPort = 0;
  bool fieldOutPort = 0;
  bool fieldIpFrag = 0;
  bool fieldIcmpV4Type = 0;
  bool fieldIcmpV4Code = 0;
  bool fieldIcmpV6Type = 0;
  bool fieldIcmpV6Code = 0;
  bool fieldDscp = 0;
  bool fieldDstMac = 0;
  bool fieldIpType = 0;
  bool fieldTtl = 0;
  bool fieldFdbDstUserMeta = 0;
  bool fieldRouteDstUserMeta = 0;
  bool fieldNeighborDstUserMeta = 0;

  for (int i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      case SAI_ACL_TABLE_ATTR_ACL_STAGE:
        stage = attr_list[i].value.s32;
        break;
      case SAI_ACL_TABLE_ATTR_ACL_BIND_POINT_TYPE_LIST:
        for (int j = 0; j < attr_list[i].value.s32list.count; ++j) {
          bindPointTypeList.push_back(attr_list[i].value.s32list.list[j]);
        }
        break;
      case SAI_ACL_TABLE_ATTR_ACL_ACTION_TYPE_LIST:
        for (int j = 0; j < attr_list[i].value.s32list.count; ++j) {
          actionTypeList.push_back(attr_list[i].value.s32list.list[j]);
        }
        break;

      case SAI_ACL_TABLE_ATTR_FIELD_SRC_IPV6:
        fieldSrcIpV6 = attr_list[i].value.booldata;
        break;
      case SAI_ACL_TABLE_ATTR_FIELD_DST_IPV6:
        fieldDstIpV6 = attr_list[i].value.booldata;
        break;
      case SAI_ACL_TABLE_ATTR_FIELD_SRC_IP:
        fieldSrcIpV4 = attr_list[i].value.booldata;
        break;
      case SAI_ACL_TABLE_ATTR_FIELD_DST_IP:
        fieldDstIpV4 = attr_list[i].value.booldata;
        break;
      case SAI_ACL_TABLE_ATTR_FIELD_L4_SRC_PORT:
        fieldL4SrcPort = attr_list[i].value.booldata;
        break;
      case SAI_ACL_TABLE_ATTR_FIELD_L4_DST_PORT:
        fieldL4DstPort = attr_list[i].value.booldata;
        break;
      case SAI_ACL_TABLE_ATTR_FIELD_IP_PROTOCOL:
        fieldIpProtocol = attr_list[i].value.booldata;
        break;
      case SAI_ACL_TABLE_ATTR_FIELD_TCP_FLAGS:
        fieldTcpFlags = attr_list[i].value.booldata;
        break;
      case SAI_ACL_TABLE_ATTR_FIELD_SRC_PORT:
        fieldSrcPort = attr_list[i].value.booldata;
        break;
      case SAI_ACL_TABLE_ATTR_FIELD_OUT_PORT:
        fieldOutPort = attr_list[i].value.booldata;
        break;
      case SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_FRAG:
        fieldIpFrag = attr_list[i].value.booldata;
        break;
      case SAI_ACL_TABLE_ATTR_FIELD_ICMP_TYPE:
        fieldIcmpV4Type = attr_list[i].value.booldata;
        break;
      case SAI_ACL_TABLE_ATTR_FIELD_ICMP_CODE:
        fieldIcmpV4Code = attr_list[i].value.booldata;
        break;
      case SAI_ACL_TABLE_ATTR_FIELD_ICMPV6_TYPE:
        fieldIcmpV6Type = attr_list[i].value.booldata;
        break;
      case SAI_ACL_TABLE_ATTR_FIELD_ICMPV6_CODE:
        fieldIcmpV6Code = attr_list[i].value.booldata;
        break;
      case SAI_ACL_TABLE_ATTR_FIELD_DSCP:
        fieldDscp = attr_list[i].value.booldata;
        break;
      case SAI_ACL_TABLE_ATTR_FIELD_DST_MAC:
        fieldDstMac = attr_list[i].value.booldata;
        break;
      case SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_TYPE:
        fieldIpType = attr_list[i].value.booldata;
        break;
      case SAI_ACL_TABLE_ATTR_FIELD_TTL:
        fieldTtl = attr_list[i].value.booldata;
        break;
      case SAI_ACL_TABLE_ATTR_FIELD_FDB_DST_USER_META:
        fieldFdbDstUserMeta = attr_list[i].value.booldata;
        break;
      case SAI_ACL_TABLE_ATTR_FIELD_ROUTE_DST_USER_META:
        fieldRouteDstUserMeta = attr_list[i].value.booldata;
        break;
      case SAI_ACL_TABLE_ATTR_FIELD_NEIGHBOR_DST_USER_META:
        fieldNeighborDstUserMeta = attr_list[i].value.booldata;
        break;
      default:
        return SAI_STATUS_INVALID_PARAMETER;
        break;
    }
  }

  if (!stage) {
    return SAI_STATUS_INVALID_PARAMETER;
  }

  *acl_table_id = fs->aclTableManager.create(
      stage.value(),
      bindPointTypeList,
      actionTypeList,
      fieldSrcIpV6,
      fieldDstIpV6,
      fieldSrcIpV4,
      fieldDstIpV4,
      fieldL4SrcPort,
      fieldL4DstPort,
      fieldIpProtocol,
      fieldTcpFlags,
      fieldSrcPort,
      fieldOutPort,
      fieldIpFrag,
      fieldIcmpV4Type,
      fieldIcmpV4Code,
      fieldIcmpV6Type,
      fieldIcmpV6Code,
      fieldDscp,
      fieldDstMac,
      fieldIpType,
      fieldTtl,
      fieldFdbDstUserMeta,
      fieldRouteDstUserMeta,
      fieldNeighborDstUserMeta);

  return SAI_STATUS_SUCCESS;
}

sai_status_t remove_acl_table_fn(sai_object_id_t acl_table_id) {
  auto fs = FakeSai::getInstance();
  fs->aclTableManager.remove(acl_table_id);
  return SAI_STATUS_SUCCESS;
}

sai_status_t set_acl_table_attribute_fn(
    sai_object_id_t /*acl_table_id*/,
    const sai_attribute_t* attr) {
  switch (attr->id) {
    default:
      // SAI spec does not support setting any attribute for ACL table post
      // creation.
      return SAI_STATUS_NOT_SUPPORTED;
  }

  return SAI_STATUS_SUCCESS;
}

sai_status_t get_acl_table_attribute_fn(
    sai_object_id_t acl_table_id,
    uint32_t attr_count,
    sai_attribute_t* attr) {
  auto fs = FakeSai::getInstance();
  for (int i = 0; i < attr_count; ++i) {
    switch (attr[i].id) {
      case SAI_ACL_TABLE_ATTR_ACL_STAGE: {
        const auto& aclTable = fs->aclTableManager.get(acl_table_id);
        attr[i].value.s32 = aclTable.stage;
      } break;
      case SAI_ACL_TABLE_ATTR_ACL_BIND_POINT_TYPE_LIST: {
        const auto& aclTable = fs->aclTableManager.get(acl_table_id);
        if (aclTable.bindPointTypeList.size() > attr[i].value.s32list.count) {
          attr[i].value.s32list.count = aclTable.bindPointTypeList.size();
          return SAI_STATUS_BUFFER_OVERFLOW;
        }

        attr[i].value.s32list.count = aclTable.bindPointTypeList.size();
        int j = 0;
        for (const auto& bindPointType : aclTable.bindPointTypeList) {
          attr[i].value.s32list.list[j++] = bindPointType;
        }
      } break;
      case SAI_ACL_TABLE_ATTR_ACL_ACTION_TYPE_LIST: {
        const auto& aclTable = fs->aclTableManager.get(acl_table_id);
        if (aclTable.actionTypeList.size() > attr[i].value.s32list.count) {
          attr[i].value.s32list.count = aclTable.actionTypeList.size();
          return SAI_STATUS_BUFFER_OVERFLOW;
        }
        attr[i].value.s32list.count = aclTable.actionTypeList.size();
        int j = 0;
        for (const auto& actionType : aclTable.actionTypeList) {
          attr[i].value.s32list.list[j++] = actionType;
        }
      } break;
      case SAI_ACL_TABLE_ATTR_ENTRY_LIST: {
        int cnt = 0;
        for (const auto& [oid, aclEntry] : fs->aclEntryManager.map()) {
          std::ignore = oid;
          if (aclEntry.tableId == acl_table_id) {
            cnt++;
          }
        }

        if (cnt > attr[i].value.objlist.count) {
          attr[i].value.objlist.count = cnt;
          return SAI_STATUS_BUFFER_OVERFLOW;
        }

        int j = 0;
        for (const auto& [oid, aclEntry] : fs->aclEntryManager.map()) {
          if (aclEntry.tableId == acl_table_id) {
            attr[i].value.objlist.list[j++] = oid;
          }
        }
        attr[i].value.objlist.count = j;
      } break;
      case SAI_ACL_TABLE_ATTR_FIELD_SRC_IPV6: {
        const auto& aclTable = fs->aclTableManager.get(acl_table_id);
        attr[i].value.booldata = aclTable.fieldSrcIpV6;
      } break;
      case SAI_ACL_TABLE_ATTR_FIELD_DST_IPV6: {
        const auto& aclTable = fs->aclTableManager.get(acl_table_id);
        attr[i].value.booldata = aclTable.fieldDstIpV6;
      } break;
      case SAI_ACL_TABLE_ATTR_FIELD_SRC_IP: {
        const auto& aclTable = fs->aclTableManager.get(acl_table_id);
        attr[i].value.booldata = aclTable.fieldSrcIpV4;
      } break;
      case SAI_ACL_TABLE_ATTR_FIELD_DST_IP: {
        const auto& aclTable = fs->aclTableManager.get(acl_table_id);
        attr[i].value.booldata = aclTable.fieldDstIpV4;
      } break;
      case SAI_ACL_TABLE_ATTR_FIELD_L4_SRC_PORT: {
        const auto& aclTable = fs->aclTableManager.get(acl_table_id);
        attr[i].value.booldata = aclTable.fieldL4SrcPort;
      } break;
      case SAI_ACL_TABLE_ATTR_FIELD_L4_DST_PORT: {
        const auto& aclTable = fs->aclTableManager.get(acl_table_id);
        attr[i].value.booldata = aclTable.fieldL4DstPort;
      } break;
      case SAI_ACL_TABLE_ATTR_FIELD_IP_PROTOCOL: {
        const auto& aclTable = fs->aclTableManager.get(acl_table_id);
        attr[i].value.booldata = aclTable.fieldIpProtocol;
      } break;
      case SAI_ACL_TABLE_ATTR_FIELD_TCP_FLAGS: {
        const auto& aclTable = fs->aclTableManager.get(acl_table_id);
        attr[i].value.booldata = aclTable.fieldTcpFlags;
      } break;
      case SAI_ACL_TABLE_ATTR_FIELD_SRC_PORT: {
        const auto& aclTable = fs->aclTableManager.get(acl_table_id);
        attr[i].value.booldata = aclTable.fieldSrcPort;
      } break;
      case SAI_ACL_TABLE_ATTR_FIELD_OUT_PORT: {
        const auto& aclTable = fs->aclTableManager.get(acl_table_id);
        attr[i].value.booldata = aclTable.fieldOutPort;
      } break;
      case SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_FRAG: {
        const auto& aclTable = fs->aclTableManager.get(acl_table_id);
        attr[i].value.booldata = aclTable.fieldIpFrag;
      } break;
      case SAI_ACL_TABLE_ATTR_FIELD_ICMP_TYPE: {
        const auto& aclTable = fs->aclTableManager.get(acl_table_id);
        attr[i].value.booldata = aclTable.fieldIcmpV4Type;
      } break;
      case SAI_ACL_TABLE_ATTR_FIELD_ICMP_CODE: {
        const auto& aclTable = fs->aclTableManager.get(acl_table_id);
        attr[i].value.booldata = aclTable.fieldIcmpV4Code;
      } break;
      case SAI_ACL_TABLE_ATTR_FIELD_ICMPV6_TYPE: {
        const auto& aclTable = fs->aclTableManager.get(acl_table_id);
        attr[i].value.booldata = aclTable.fieldIcmpV6Type;
      } break;
      case SAI_ACL_TABLE_ATTR_FIELD_ICMPV6_CODE: {
        const auto& aclTable = fs->aclTableManager.get(acl_table_id);
        attr[i].value.booldata = aclTable.fieldIcmpV6Code;
      } break;
      case SAI_ACL_TABLE_ATTR_FIELD_DSCP: {
        const auto& aclTable = fs->aclTableManager.get(acl_table_id);
        attr[i].value.booldata = aclTable.fieldDscp;
      } break;
      case SAI_ACL_TABLE_ATTR_FIELD_DST_MAC: {
        const auto& aclTable = fs->aclTableManager.get(acl_table_id);
        attr[i].value.booldata = aclTable.fieldDstMac;
      } break;
      case SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_TYPE: {
        const auto& aclTable = fs->aclTableManager.get(acl_table_id);
        attr[i].value.booldata = aclTable.fieldIpType;
      } break;
      case SAI_ACL_TABLE_ATTR_FIELD_TTL: {
        const auto& aclTable = fs->aclTableManager.get(acl_table_id);
        attr[i].value.booldata = aclTable.fieldTtl;
      } break;
      case SAI_ACL_TABLE_ATTR_FIELD_FDB_DST_USER_META: {
        const auto& aclTable = fs->aclTableManager.get(acl_table_id);
        attr[i].value.booldata = aclTable.fieldFdbDstUserMeta;
      } break;
      case SAI_ACL_TABLE_ATTR_FIELD_ROUTE_DST_USER_META: {
        const auto& aclTable = fs->aclTableManager.get(acl_table_id);
        attr[i].value.booldata = aclTable.fieldRouteDstUserMeta;
      } break;
      case SAI_ACL_TABLE_ATTR_FIELD_NEIGHBOR_DST_USER_META: {
        const auto& aclTable = fs->aclTableManager.get(acl_table_id);
        attr[i].value.booldata = aclTable.fieldNeighborDstUserMeta;
      } break;
      case SAI_ACL_TABLE_ATTR_AVAILABLE_ACL_ENTRY:
      case SAI_ACL_TABLE_ATTR_AVAILABLE_ACL_COUNTER:
        attr[i].value.u32 = 1000;
        break;
      default:
        return SAI_STATUS_NOT_SUPPORTED;
    }
  }

  return SAI_STATUS_SUCCESS;
}

sai_status_t set_acl_entry_attribute_fn(
    sai_object_id_t acl_entry_id,
    const sai_attribute_t* attr) {
  auto fs = FakeSai::getInstance();
  auto& aclEntry = fs->aclEntryManager.get(acl_entry_id);
  sai_status_t res;
  if (!attr) {
    return SAI_STATUS_INVALID_PARAMETER;
  }

  switch (attr->id) {
    case SAI_ACL_ENTRY_ATTR_PRIORITY:
      aclEntry.priority = attr->value.u32;
      res = SAI_STATUS_SUCCESS;
      break;
    case SAI_ACL_ENTRY_ATTR_FIELD_SRC_IPV6:
      aclEntry.fieldSrcIpV6Enable = attr->value.aclfield.enable;
      aclEntry.fieldSrcIpV6Data =
          facebook::fboss::fromSaiIpAddress(attr->value.aclfield.data.ip6);
      aclEntry.fieldSrcIpV6Mask =
          facebook::fboss::fromSaiIpAddress(attr->value.aclfield.mask.ip6);
      res = SAI_STATUS_SUCCESS;
      break;
    case SAI_ACL_ENTRY_ATTR_FIELD_DST_IPV6:
      aclEntry.fieldDstIpV6Enable = attr->value.aclfield.enable;
      aclEntry.fieldDstIpV6Data =
          facebook::fboss::fromSaiIpAddress(attr->value.aclfield.data.ip6);
      aclEntry.fieldDstIpV6Mask =
          facebook::fboss::fromSaiIpAddress(attr->value.aclfield.mask.ip6);
      res = SAI_STATUS_SUCCESS;
      break;
    case SAI_ACL_ENTRY_ATTR_FIELD_SRC_IP:
      aclEntry.fieldSrcIpV4Enable = attr->value.aclfield.enable;
      aclEntry.fieldSrcIpV4Data =
          facebook::fboss::fromSaiIpAddress(attr->value.aclfield.data.ip4);
      aclEntry.fieldSrcIpV4Mask =
          facebook::fboss::fromSaiIpAddress(attr->value.aclfield.mask.ip4);
      res = SAI_STATUS_SUCCESS;
      break;
    case SAI_ACL_ENTRY_ATTR_FIELD_DST_IP:
      aclEntry.fieldDstIpV4Enable = attr->value.aclfield.enable;
      aclEntry.fieldDstIpV4Data =
          facebook::fboss::fromSaiIpAddress(attr->value.aclfield.data.ip4);
      aclEntry.fieldDstIpV4Mask =
          facebook::fboss::fromSaiIpAddress(attr->value.aclfield.mask.ip4);
      res = SAI_STATUS_SUCCESS;
      break;

    /*
     * Mask is not needed for SAI_ACL_ENTRY_ATTR_FIELD_SRC_PORT
     * or SAI_ACL_ENTRY_ATTR_FIELD_OUT_PORT.
     * Thus, there is no oid field in sai_acl_field_data_mask_t.
     * oid is u64, but unfortunately, u64 was not added to
     * sai_acl_field_data_mask_t till SAI 1.6, so use u32.
     * This will be ignored by the implementation anyway.
     */
    case SAI_ACL_ENTRY_ATTR_FIELD_SRC_PORT:
      aclEntry.fieldSrcPortEnable = attr->value.aclfield.enable;
      aclEntry.fieldSrcPortData = attr->value.aclfield.data.oid;
      aclEntry.fieldSrcPortMask = attr->value.aclfield.mask.u32;
      res = SAI_STATUS_SUCCESS;
      break;
    case SAI_ACL_ENTRY_ATTR_FIELD_OUT_PORT:
      aclEntry.fieldOutPortEnable = attr->value.aclfield.enable;
      aclEntry.fieldOutPortData = attr->value.aclfield.data.oid;
      aclEntry.fieldOutPortMask = attr->value.aclfield.mask.u32;
      res = SAI_STATUS_SUCCESS;
      break;
    case SAI_ACL_ENTRY_ATTR_FIELD_L4_SRC_PORT:
      aclEntry.fieldL4SrcPortEnable = attr->value.aclfield.enable;
      aclEntry.fieldL4SrcPortData = attr->value.aclfield.data.u16;
      aclEntry.fieldL4SrcPortMask = attr->value.aclfield.mask.u16;
      res = SAI_STATUS_SUCCESS;
      break;
    case SAI_ACL_ENTRY_ATTR_FIELD_L4_DST_PORT:
      aclEntry.fieldL4DstPortEnable = attr->value.aclfield.enable;
      aclEntry.fieldL4DstPortData = attr->value.aclfield.data.u16;
      aclEntry.fieldL4DstPortMask = attr->value.aclfield.mask.u16;
      res = SAI_STATUS_SUCCESS;
      break;
    case SAI_ACL_ENTRY_ATTR_FIELD_IP_PROTOCOL:
      aclEntry.fieldIpProtocolEnable = attr->value.aclfield.enable;
      aclEntry.fieldIpProtocolData = attr->value.aclfield.data.u8;
      aclEntry.fieldIpProtocolMask = attr->value.aclfield.mask.u8;
      res = SAI_STATUS_SUCCESS;
      break;
    case SAI_ACL_ENTRY_ATTR_FIELD_TCP_FLAGS:
      aclEntry.fieldTcpFlagsEnable = attr->value.aclfield.enable;
      aclEntry.fieldTcpFlagsData = attr->value.aclfield.data.u8;
      aclEntry.fieldTcpFlagsMask = attr->value.aclfield.mask.u8;
      res = SAI_STATUS_SUCCESS;
      break;
    case SAI_ACL_ENTRY_ATTR_FIELD_ACL_IP_FRAG:
      aclEntry.fieldIpFragEnable = attr->value.aclfield.enable;
      aclEntry.fieldIpFragData = attr->value.aclfield.data.u32;
      aclEntry.fieldIpFragMask = attr->value.aclfield.mask.u32;
      res = SAI_STATUS_SUCCESS;
      break;
    case SAI_ACL_ENTRY_ATTR_FIELD_ICMP_TYPE:
      aclEntry.fieldIcmpV4TypeEnable = attr->value.aclfield.enable;
      aclEntry.fieldIcmpV4TypeData = attr->value.aclfield.data.u8;
      aclEntry.fieldIcmpV4TypeMask = attr->value.aclfield.mask.u8;
      res = SAI_STATUS_SUCCESS;
      break;
    case SAI_ACL_ENTRY_ATTR_FIELD_ICMP_CODE:
      aclEntry.fieldIcmpV4CodeEnable = attr->value.aclfield.enable;
      aclEntry.fieldIcmpV4CodeData = attr->value.aclfield.data.u8;
      aclEntry.fieldIcmpV4CodeMask = attr->value.aclfield.mask.u8;
      res = SAI_STATUS_SUCCESS;
      break;
    case SAI_ACL_ENTRY_ATTR_FIELD_ICMPV6_TYPE:
      aclEntry.fieldIcmpV6TypeEnable = attr->value.aclfield.enable;
      aclEntry.fieldIcmpV6TypeData = attr->value.aclfield.data.u8;
      aclEntry.fieldIcmpV6TypeMask = attr->value.aclfield.mask.u8;
      res = SAI_STATUS_SUCCESS;
      break;
    case SAI_ACL_ENTRY_ATTR_FIELD_ICMPV6_CODE:
      aclEntry.fieldIcmpV6CodeEnable = attr->value.aclfield.enable;
      aclEntry.fieldIcmpV6CodeData = attr->value.aclfield.data.u8;
      aclEntry.fieldIcmpV6CodeMask = attr->value.aclfield.mask.u8;
      res = SAI_STATUS_SUCCESS;
      break;
    case SAI_ACL_ENTRY_ATTR_FIELD_DSCP:
      aclEntry.fieldDscpEnable = attr->value.aclfield.enable;
      aclEntry.fieldDscpData = attr->value.aclfield.data.u8;
      aclEntry.fieldDscpMask = attr->value.aclfield.mask.u8;
      res = SAI_STATUS_SUCCESS;
      break;
    case SAI_ACL_ENTRY_ATTR_FIELD_DST_MAC:
      aclEntry.fieldDstMacEnable = attr->value.aclfield.enable;
      aclEntry.fieldDstMacData =
          facebook::fboss::fromSaiMacAddress(attr->value.aclfield.data.mac);
      aclEntry.fieldDstMacMask =
          facebook::fboss::fromSaiMacAddress(attr->value.aclfield.mask.mac);
      res = SAI_STATUS_SUCCESS;
      break;
    case SAI_ACL_ENTRY_ATTR_FIELD_ACL_IP_TYPE:
      aclEntry.fieldIpTypeEnable = attr->value.aclfield.enable;
      aclEntry.fieldIpTypeData = attr->value.aclfield.data.u32;
      aclEntry.fieldIpTypeMask = attr->value.aclfield.mask.u32;
      res = SAI_STATUS_SUCCESS;
      break;
    case SAI_ACL_ENTRY_ATTR_FIELD_TTL:
      aclEntry.fieldTtlEnable = attr->value.aclfield.enable;
      aclEntry.fieldTtlData = attr->value.aclfield.data.u8;
      aclEntry.fieldTtlMask = attr->value.aclfield.mask.u8;
      res = SAI_STATUS_SUCCESS;
      break;
    case SAI_ACL_ENTRY_ATTR_FIELD_FDB_DST_USER_META:
      aclEntry.fieldFdbDstUserMetaEnable = attr->value.aclfield.enable;
      aclEntry.fieldFdbDstUserMetaData = attr->value.aclfield.data.u32;
      aclEntry.fieldFdbDstUserMetaMask = attr->value.aclfield.mask.u32;
      res = SAI_STATUS_SUCCESS;
      break;
    case SAI_ACL_ENTRY_ATTR_FIELD_ROUTE_DST_USER_META:
      aclEntry.fieldRouteDstUserMetaEnable = attr->value.aclfield.enable;
      aclEntry.fieldRouteDstUserMetaData = attr->value.aclfield.data.u32;
      aclEntry.fieldRouteDstUserMetaMask = attr->value.aclfield.mask.u32;
      res = SAI_STATUS_SUCCESS;
      break;
    case SAI_ACL_ENTRY_ATTR_FIELD_NEIGHBOR_DST_USER_META:
      aclEntry.fieldNeighborDstUserMetaEnable = attr->value.aclfield.enable;
      aclEntry.fieldNeighborDstUserMetaData = attr->value.aclfield.data.u32;
      aclEntry.fieldNeighborDstUserMetaMask = attr->value.aclfield.mask.u32;
      res = SAI_STATUS_SUCCESS;
      break;
    case SAI_ACL_ENTRY_ATTR_ACTION_PACKET_ACTION:
      aclEntry.actionPacketActionEnable = attr->value.aclaction.enable;
      aclEntry.actionPacketActionData = attr->value.aclaction.parameter.u32;
      res = SAI_STATUS_SUCCESS;
      break;
    case SAI_ACL_ENTRY_ATTR_ACTION_COUNTER:
      aclEntry.actionCounterEnable = attr->value.aclaction.enable;
      aclEntry.actionCounterData = attr->value.aclaction.parameter.oid;
      res = SAI_STATUS_SUCCESS;
      break;
    case SAI_ACL_ENTRY_ATTR_ACTION_SET_TC:
      aclEntry.actionSetTCEnable = attr->value.aclaction.enable;
      aclEntry.actionSetTCData = attr->value.aclaction.parameter.u8;
      res = SAI_STATUS_SUCCESS;
      break;
    case SAI_ACL_ENTRY_ATTR_ACTION_SET_DSCP:
      aclEntry.actionSetDSCPEnable = attr->value.aclaction.enable;
      aclEntry.actionSetDSCPData = attr->value.aclaction.parameter.u8;
      res = SAI_STATUS_SUCCESS;
      break;
    case SAI_ACL_ENTRY_ATTR_ACTION_MIRROR_INGRESS:
      aclEntry.actionMirrorIngressEnable = attr->value.aclaction.enable;
      aclEntry.actionMirrorIngressData.resize(
          attr->value.aclaction.parameter.objlist.count);
      std::copy(
          attr->value.aclaction.parameter.objlist.list,
          attr->value.aclaction.parameter.objlist.list +
              attr->value.aclaction.parameter.objlist.count,
          std::begin(aclEntry.actionMirrorIngressData));
      res = SAI_STATUS_SUCCESS;
      break;
    case SAI_ACL_ENTRY_ATTR_ACTION_MIRROR_EGRESS:
      aclEntry.actionMirrorEgressEnable = attr->value.aclaction.enable;
      aclEntry.actionMirrorEgressData.resize(
          attr->value.aclaction.parameter.objlist.count);
      std::copy(
          attr->value.aclaction.parameter.objlist.list,
          attr->value.aclaction.parameter.objlist.list +
              attr->value.aclaction.parameter.objlist.count,
          std::begin(aclEntry.actionMirrorEgressData));
      res = SAI_STATUS_SUCCESS;
      break;
    default:
      res = SAI_STATUS_NOT_SUPPORTED;
      break;
  }
  return res;
}

sai_status_t get_acl_entry_attribute_fn(
    sai_object_id_t acl_entry_id,
    uint32_t attr_count,
    sai_attribute_t* attr_list) {
  auto fs = FakeSai::getInstance();
  auto& aclEntry = fs->aclEntryManager.get(acl_entry_id);
  for (int i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      case SAI_ACL_ENTRY_ATTR_TABLE_ID:
        attr_list[i].value.oid = aclEntry.tableId;
        break;
      case SAI_ACL_ENTRY_ATTR_PRIORITY:
        attr_list[i].value.u32 = aclEntry.priority;
        break;
      case SAI_ACL_ENTRY_ATTR_FIELD_SRC_IPV6:
        attr_list[i].value.aclfield.enable = aclEntry.fieldSrcIpV6Enable;
        facebook::fboss::toSaiIpAddressV6(
            aclEntry.fieldSrcIpV6Data, &attr_list[i].value.aclfield.data.ip6);
        facebook::fboss::toSaiIpAddressV6(
            aclEntry.fieldSrcIpV6Mask, &attr_list[i].value.aclfield.mask.ip6);
        break;
      case SAI_ACL_ENTRY_ATTR_FIELD_DST_IPV6:
        attr_list[i].value.aclfield.enable = aclEntry.fieldDstIpV6Enable;
        facebook::fboss::toSaiIpAddressV6(
            aclEntry.fieldDstIpV6Data, &attr_list[i].value.aclfield.data.ip6);
        facebook::fboss::toSaiIpAddressV6(
            aclEntry.fieldDstIpV6Mask, &attr_list[i].value.aclfield.mask.ip6);
        break;
      case SAI_ACL_ENTRY_ATTR_FIELD_SRC_IP:
        attr_list[i].value.aclfield.enable = aclEntry.fieldSrcIpV4Enable;
        attr_list[i].value.aclfield.data.ip4 =
            facebook::fboss::toSaiIpAddress(aclEntry.fieldSrcIpV4Data).addr.ip4;
        attr_list[i].value.aclfield.mask.ip4 =
            facebook::fboss::toSaiIpAddress(aclEntry.fieldSrcIpV4Mask).addr.ip4;
        break;
      case SAI_ACL_ENTRY_ATTR_FIELD_DST_IP:
        attr_list[i].value.aclfield.enable = aclEntry.fieldDstIpV4Enable;
        attr_list[i].value.aclfield.data.ip4 =
            facebook::fboss::toSaiIpAddress(aclEntry.fieldDstIpV4Data).addr.ip4;
        attr_list[i].value.aclfield.mask.ip4 =
            facebook::fboss::toSaiIpAddress(aclEntry.fieldDstIpV4Mask).addr.ip4;
        break;

      /*
       * Mask is not needed for SAI_ACL_ENTRY_ATTR_FIELD_SRC_PORT
       * or SAI_ACL_ENTRY_ATTR_FIELD_OUT_PORT.
       * Thus, there is no oid field in sai_acl_field_data_mask_t.
       * oid is u64, but unfortunately, u64 was not added to
       * sai_acl_field_data_mask_t till SAI 1.6, so use u32.
       * This will be ignored by the implementation anyway.
       */
      case SAI_ACL_ENTRY_ATTR_FIELD_SRC_PORT:
        attr_list[i].value.aclfield.enable = aclEntry.fieldSrcPortEnable;
        attr_list[i].value.aclfield.data.oid = aclEntry.fieldSrcPortData;
        attr_list[i].value.aclfield.mask.u32 = aclEntry.fieldSrcPortMask;
        break;
      case SAI_ACL_ENTRY_ATTR_FIELD_OUT_PORT:
        attr_list[i].value.aclfield.enable = aclEntry.fieldOutPortEnable;
        attr_list[i].value.aclfield.data.oid = aclEntry.fieldOutPortData;
        attr_list[i].value.aclfield.mask.u32 = aclEntry.fieldOutPortMask;
        break;
      case SAI_ACL_ENTRY_ATTR_FIELD_L4_SRC_PORT:
        attr_list[i].value.aclfield.enable = aclEntry.fieldL4SrcPortEnable;
        attr_list[i].value.aclfield.data.u16 = aclEntry.fieldL4SrcPortData;
        attr_list[i].value.aclfield.mask.u16 = aclEntry.fieldL4SrcPortMask;
        break;
      case SAI_ACL_ENTRY_ATTR_FIELD_L4_DST_PORT:
        attr_list[i].value.aclfield.enable = aclEntry.fieldL4DstPortEnable;
        attr_list[i].value.aclfield.data.u16 = aclEntry.fieldL4DstPortData;
        attr_list[i].value.aclfield.mask.u16 = aclEntry.fieldL4DstPortMask;
        break;
      case SAI_ACL_ENTRY_ATTR_FIELD_IP_PROTOCOL:
        attr_list[i].value.aclfield.enable = aclEntry.fieldIpProtocolEnable;
        attr_list[i].value.aclfield.data.u8 = aclEntry.fieldIpProtocolData;
        attr_list[i].value.aclfield.mask.u8 = aclEntry.fieldIpProtocolMask;
        break;
      case SAI_ACL_ENTRY_ATTR_FIELD_TCP_FLAGS:
        attr_list[i].value.aclfield.enable = aclEntry.fieldTcpFlagsEnable;
        attr_list[i].value.aclfield.data.u8 = aclEntry.fieldTcpFlagsData;
        attr_list[i].value.aclfield.mask.u8 = aclEntry.fieldTcpFlagsMask;
        break;
      case SAI_ACL_ENTRY_ATTR_FIELD_ACL_IP_FRAG:
        attr_list[i].value.aclfield.enable = aclEntry.fieldIpFragEnable;
        attr_list[i].value.aclfield.data.u32 = aclEntry.fieldIpFragData;
        attr_list[i].value.aclfield.mask.u32 = aclEntry.fieldIpFragMask;
        break;
      case SAI_ACL_ENTRY_ATTR_FIELD_ICMP_TYPE:
        attr_list[i].value.aclfield.enable = aclEntry.fieldIcmpV4TypeEnable;
        attr_list[i].value.aclfield.data.u8 = aclEntry.fieldIcmpV4TypeData;
        attr_list[i].value.aclfield.mask.u8 = aclEntry.fieldIcmpV4TypeMask;
        break;
      case SAI_ACL_ENTRY_ATTR_FIELD_ICMP_CODE:
        attr_list[i].value.aclfield.enable = aclEntry.fieldIcmpV4CodeEnable;
        attr_list[i].value.aclfield.data.u8 = aclEntry.fieldIcmpV4CodeData;
        attr_list[i].value.aclfield.mask.u8 = aclEntry.fieldIcmpV4CodeMask;
        break;
      case SAI_ACL_ENTRY_ATTR_FIELD_ICMPV6_TYPE:
        attr_list[i].value.aclfield.enable = aclEntry.fieldIcmpV6TypeEnable;
        attr_list[i].value.aclfield.data.u8 = aclEntry.fieldIcmpV6TypeData;
        attr_list[i].value.aclfield.mask.u8 = aclEntry.fieldIcmpV6TypeMask;
        break;
      case SAI_ACL_ENTRY_ATTR_FIELD_ICMPV6_CODE:
        attr_list[i].value.aclfield.enable = aclEntry.fieldIcmpV6CodeEnable;
        attr_list[i].value.aclfield.data.u8 = aclEntry.fieldIcmpV6CodeData;
        attr_list[i].value.aclfield.mask.u8 = aclEntry.fieldIcmpV6CodeMask;
        break;
      case SAI_ACL_ENTRY_ATTR_FIELD_DSCP:
        attr_list[i].value.aclfield.enable = aclEntry.fieldDscpEnable;
        attr_list[i].value.aclfield.data.u8 = aclEntry.fieldDscpData;
        attr_list[i].value.aclfield.mask.u8 = aclEntry.fieldDscpMask;
        break;
      case SAI_ACL_ENTRY_ATTR_FIELD_DST_MAC:
        attr_list[i].value.aclfield.enable = aclEntry.fieldDstMacEnable;
        facebook::fboss::toSaiMacAddress(
            aclEntry.fieldDstMacData, attr_list[i].value.aclfield.data.mac);
        facebook::fboss::toSaiMacAddress(
            aclEntry.fieldDstMacMask, attr_list[i].value.aclfield.mask.mac);
        break;
      case SAI_ACL_ENTRY_ATTR_FIELD_ACL_IP_TYPE:
        attr_list[i].value.aclfield.enable = aclEntry.fieldIpTypeEnable;
        attr_list[i].value.aclfield.data.u32 = aclEntry.fieldIpTypeData;
        attr_list[i].value.aclfield.mask.u32 = aclEntry.fieldIpTypeMask;
        break;
      case SAI_ACL_ENTRY_ATTR_FIELD_TTL:
        attr_list[i].value.aclfield.enable = aclEntry.fieldTtlEnable;
        attr_list[i].value.aclfield.data.u8 = aclEntry.fieldTtlData;
        attr_list[i].value.aclfield.mask.u8 = aclEntry.fieldTtlMask;
        break;
      case SAI_ACL_ENTRY_ATTR_FIELD_FDB_DST_USER_META:
        attr_list[i].value.aclfield.enable = aclEntry.fieldFdbDstUserMetaEnable;
        attr_list[i].value.aclfield.data.u32 = aclEntry.fieldFdbDstUserMetaData;
        attr_list[i].value.aclfield.mask.u32 = aclEntry.fieldFdbDstUserMetaMask;
        break;
      case SAI_ACL_ENTRY_ATTR_FIELD_ROUTE_DST_USER_META:
        attr_list[i].value.aclfield.enable =
            aclEntry.fieldRouteDstUserMetaEnable;
        attr_list[i].value.aclfield.data.u32 =
            aclEntry.fieldRouteDstUserMetaData;
        attr_list[i].value.aclfield.mask.u32 =
            aclEntry.fieldRouteDstUserMetaMask;
        break;
      case SAI_ACL_ENTRY_ATTR_FIELD_NEIGHBOR_DST_USER_META:
        attr_list[i].value.aclfield.enable =
            aclEntry.fieldNeighborDstUserMetaEnable;
        attr_list[i].value.aclfield.data.u32 =
            aclEntry.fieldNeighborDstUserMetaData;
        attr_list[i].value.aclfield.mask.u32 =
            aclEntry.fieldNeighborDstUserMetaMask;
        break;
      case SAI_ACL_ENTRY_ATTR_ACTION_PACKET_ACTION:
        attr_list[i].value.aclaction.enable = aclEntry.actionPacketActionEnable;
        attr_list[i].value.aclaction.parameter.u32 =
            aclEntry.actionPacketActionData;
        break;
      case SAI_ACL_ENTRY_ATTR_ACTION_COUNTER:
        attr_list[i].value.aclaction.enable = aclEntry.actionCounterEnable;
        attr_list[i].value.aclaction.parameter.oid = aclEntry.actionCounterData;
        break;
      case SAI_ACL_ENTRY_ATTR_ACTION_SET_TC:
        attr_list[i].value.aclaction.enable = aclEntry.actionSetTCEnable;
        attr_list[i].value.aclaction.parameter.u8 = aclEntry.actionSetTCData;
        break;
      case SAI_ACL_ENTRY_ATTR_ACTION_SET_DSCP:
        attr_list[i].value.aclaction.enable = aclEntry.actionSetDSCPEnable;
        attr_list[i].value.aclaction.parameter.u8 = aclEntry.actionSetDSCPData;
        break;
      case SAI_ACL_ENTRY_ATTR_ACTION_MIRROR_INGRESS:
        attr_list[i].value.aclaction.enable =
            aclEntry.actionMirrorIngressEnable;
        attr_list[i].value.aclaction.parameter.objlist.count =
            aclEntry.actionMirrorIngressData.size();
        attr_list[i].value.aclaction.parameter.objlist.list =
            aclEntry.actionMirrorIngressData.data();
        break;
      case SAI_ACL_ENTRY_ATTR_ACTION_MIRROR_EGRESS:
        attr_list[i].value.aclaction.enable = aclEntry.actionMirrorEgressEnable;
        attr_list[i].value.aclaction.parameter.objlist.count =
            aclEntry.actionMirrorEgressData.size();
        attr_list[i].value.aclaction.parameter.objlist.list =
            aclEntry.actionMirrorEgressData.data();
        break;
      default:
        return SAI_STATUS_NOT_SUPPORTED;
    }
  }
  return SAI_STATUS_SUCCESS;
}

sai_status_t create_acl_entry_fn(
    sai_object_id_t* acl_entry_id,
    sai_object_id_t switch_id,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  auto fs = FakeSai::getInstance();

  std::optional<sai_object_id_t> tableId;
  for (int i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      case SAI_ACL_ENTRY_ATTR_TABLE_ID:
        tableId = attr_list[i].value.oid;
        break;
    }
  }

  if (!tableId) {
    return SAI_STATUS_INVALID_PARAMETER;
  }

  *acl_entry_id = fs->aclEntryManager.create(tableId.value());
  auto& aclEntry = fs->aclEntryManager.get(*acl_entry_id);

  for (int i = 0; i < attr_count; ++i) {
    if (attr_list[i].id == SAI_ACL_ENTRY_ATTR_TABLE_ID) {
      aclEntry.tableId = attr_list[i].value.oid;
    } else {
      sai_status_t res =
          set_acl_entry_attribute_fn(*acl_entry_id, &attr_list[i]);
      if (res != SAI_STATUS_SUCCESS) {
        fs->aclEntryManager.remove(*acl_entry_id);
        return res;
      }
    }
  }

  return SAI_STATUS_SUCCESS;
}

sai_status_t remove_acl_entry_fn(sai_object_id_t acl_entry_id) {
  auto fs = FakeSai::getInstance();
  fs->aclEntryManager.remove(acl_entry_id);
  return SAI_STATUS_SUCCESS;
}

sai_status_t set_acl_counter_attribute_fn(
    sai_object_id_t acl_counter_id,
    const sai_attribute_t* attr) {
  auto fs = FakeSai::getInstance();
  auto& aclCounter = fs->aclCounterManager.get(acl_counter_id);
  sai_status_t res;
  if (!attr) {
    return SAI_STATUS_INVALID_PARAMETER;
  }

  switch (attr->id) {
    case SAI_ACL_COUNTER_ATTR_PACKETS:
      aclCounter.counterPackets = attr->value.u64;
      res = SAI_STATUS_SUCCESS;
      break;
    case SAI_ACL_COUNTER_ATTR_BYTES:
      aclCounter.counterBytes = attr->value.u64;
      res = SAI_STATUS_SUCCESS;
      break;
    default:
      res = SAI_STATUS_NOT_SUPPORTED;
      break;
  }
  return res;
}

sai_status_t get_acl_counter_attribute_fn(
    sai_object_id_t acl_counter_id,
    uint32_t attr_count,
    sai_attribute_t* attr_list) {
  auto fs = FakeSai::getInstance();
  auto& aclCounter = fs->aclCounterManager.get(acl_counter_id);

  for (int i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      case SAI_ACL_COUNTER_ATTR_TABLE_ID:
        attr_list[i].value.oid = aclCounter.tableId;
        break;
      case SAI_ACL_COUNTER_ATTR_ENABLE_PACKET_COUNT:
        attr_list[i].value.booldata = aclCounter.enablePacketCount;
        break;
      case SAI_ACL_COUNTER_ATTR_ENABLE_BYTE_COUNT:
        attr_list[i].value.booldata = aclCounter.enableByteCount;
        break;
      case SAI_ACL_COUNTER_ATTR_PACKETS:
        attr_list[i].value.u64 = aclCounter.counterPackets;
        break;
      case SAI_ACL_COUNTER_ATTR_BYTES:
        attr_list[i].value.u64 = aclCounter.counterBytes;
        break;
      default:
        return SAI_STATUS_NOT_SUPPORTED;
    }
  }
  return SAI_STATUS_SUCCESS;
}

sai_status_t create_acl_counter_fn(
    sai_object_id_t* acl_counter_id,
    sai_object_id_t /*switch_id */,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  auto fs = FakeSai::getInstance();

  std::optional<sai_object_id_t> tableId;
  for (int i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      case SAI_ACL_COUNTER_ATTR_TABLE_ID:
        tableId = attr_list[i].value.oid;
        break;
    }
  }

  if (!tableId) {
    return SAI_STATUS_INVALID_PARAMETER;
  }

  *acl_counter_id = fs->aclCounterManager.create(tableId.value());
  auto& aclCounter = fs->aclCounterManager.get(*acl_counter_id);

  for (int i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      case SAI_ACL_COUNTER_ATTR_TABLE_ID:
        aclCounter.tableId = attr_list[i].value.oid;
        break;
      case SAI_ACL_COUNTER_ATTR_ENABLE_PACKET_COUNT:
        aclCounter.enablePacketCount = attr_list[i].value.booldata;
        break;
      case SAI_ACL_COUNTER_ATTR_ENABLE_BYTE_COUNT:
        aclCounter.enableByteCount = attr_list[i].value.booldata;
        break;
      default: {
        sai_status_t res =
            set_acl_counter_attribute_fn(*acl_counter_id, &attr_list[i]);
        if (res != SAI_STATUS_SUCCESS) {
          fs->aclCounterManager.remove(*acl_counter_id);
          return res;
        }
      }
    }
  }

  return SAI_STATUS_SUCCESS;
}

sai_status_t remove_acl_counter_fn(sai_object_id_t acl_counter_id) {
  auto fs = FakeSai::getInstance();
  fs->aclCounterManager.remove(acl_counter_id);
  return SAI_STATUS_SUCCESS;
}

sai_status_t create_acl_range_fn(
    sai_object_id_t* /*acl_range_id */,
    sai_object_id_t /*switch_id*/,
    uint32_t /*attr_count*/,
    const sai_attribute_t* /*attr_list*/) {
  return SAI_STATUS_NOT_IMPLEMENTED;
}

sai_status_t remove_acl_range_fn(sai_object_id_t /*acl_range_id*/) {
  return SAI_STATUS_NOT_IMPLEMENTED;
}

sai_status_t set_acl_range_attribute_fn(
    sai_object_id_t /*acl_range_id*/,
    const sai_attribute_t* /*attr*/) {
  return SAI_STATUS_NOT_IMPLEMENTED;
}

sai_status_t get_acl_range_attribute_fn(
    sai_object_id_t acl_range_id,
    uint32_t attr_count,
    sai_attribute_t* attr_list) {
  return SAI_STATUS_NOT_IMPLEMENTED;
}

sai_status_t create_acl_table_group_fn(
    sai_object_id_t* acl_table_group_id,
    sai_object_id_t switch_id,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  auto fs = FakeSai::getInstance();

  std::optional<sai_int32_t> stage;
  std::vector<sai_int32_t> bindPointTypeList;
  sai_int32_t type;

  for (int i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      case SAI_ACL_TABLE_GROUP_ATTR_ACL_STAGE:
        stage = attr_list[i].value.s32;
        break;
      case SAI_ACL_TABLE_GROUP_ATTR_ACL_BIND_POINT_TYPE_LIST:
        for (int j = 0; j < attr_list[i].value.s32list.count; ++j) {
          bindPointTypeList.push_back(attr_list[i].value.s32list.list[j]);
        }
        break;
      case SAI_ACL_TABLE_GROUP_ATTR_TYPE:
        type = attr_list[i].value.s32;
        break;
      default:
        return SAI_STATUS_INVALID_PARAMETER;
        break;
    }
  }

  if (!stage) {
    return SAI_STATUS_INVALID_PARAMETER;
  }

  *acl_table_group_id =
      fs->aclTableGroupManager.create(stage.value(), bindPointTypeList, type);

  return SAI_STATUS_SUCCESS;
}

sai_status_t remove_acl_table_group_fn(sai_object_id_t acl_table_group_id) {
  auto fs = FakeSai::getInstance();
  fs->aclTableGroupManager.remove(acl_table_group_id);
  return SAI_STATUS_SUCCESS;
}

sai_status_t set_acl_table_group_attribute_fn(
    sai_object_id_t /*acl_table_group_id*/,
    const sai_attribute_t* attr) {
  switch (attr->id) {
    default:
      // SAI spec does not support setting any attribute for ACL table group
      // post creation.
      return SAI_STATUS_NOT_SUPPORTED;
  }

  return SAI_STATUS_SUCCESS;
}

sai_status_t get_acl_table_group_attribute_fn(
    sai_object_id_t acl_table_group_id,
    uint32_t attr_count,
    sai_attribute_t* attr_list) {
  auto fs = FakeSai::getInstance();

  for (int i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      case SAI_ACL_TABLE_GROUP_ATTR_ACL_STAGE: {
        const auto& aclTableGroup =
            fs->aclTableGroupManager.get(acl_table_group_id);
        attr_list[i].value.s32 = aclTableGroup.stage;
      } break;
      case SAI_ACL_TABLE_GROUP_ATTR_ACL_BIND_POINT_TYPE_LIST: {
        const auto& aclTableGroup =
            fs->aclTableGroupManager.get(acl_table_group_id);
        if (aclTableGroup.bindPointTypeList.size() >
            attr_list[i].value.s32list.count) {
          attr_list[i].value.s32list.count =
              aclTableGroup.bindPointTypeList.size();
          return SAI_STATUS_BUFFER_OVERFLOW;
        }

        attr_list[i].value.s32list.count =
            aclTableGroup.bindPointTypeList.size();
        int j = 0;
        for (const auto& bindPointType : aclTableGroup.bindPointTypeList) {
          attr_list[i].value.s32list.list[j++] = bindPointType;
        }
      } break;
      case SAI_ACL_TABLE_GROUP_ATTR_TYPE: {
        const auto& aclTableGroup =
            fs->aclTableGroupManager.get(acl_table_group_id);
        attr_list[i].value.s32 = aclTableGroup.type;
      } break;
      case SAI_ACL_TABLE_GROUP_ATTR_MEMBER_LIST: {
        const auto& aclTableGroupMemberMap =
            fs->aclTableGroupManager.get(acl_table_group_id).fm().map();
        if (aclTableGroupMemberMap.size() > attr_list[i].value.objlist.count) {
          attr_list[i].value.objlist.count = aclTableGroupMemberMap.size();
          return SAI_STATUS_BUFFER_OVERFLOW;
        }
        attr_list[i].value.objlist.count = aclTableGroupMemberMap.size();
        int j = 0;
        for (const auto& m : aclTableGroupMemberMap) {
          attr_list[i].value.objlist.list[j++] = m.first;
        }
      } break;
      default:
        return SAI_STATUS_NOT_SUPPORTED;
    }
  }

  return SAI_STATUS_SUCCESS;
}

sai_status_t create_acl_table_group_member_fn(
    sai_object_id_t* acl_table_group_member_id,
    sai_object_id_t /*switch_id*/,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  auto fs = FakeSai::getInstance();

  std::optional<sai_object_id_t> tableGroupId;
  std::optional<sai_object_id_t> tableId;
  std::optional<sai_uint32_t> priority;

  for (int i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      case SAI_ACL_TABLE_GROUP_MEMBER_ATTR_ACL_TABLE_GROUP_ID:
        tableGroupId = attr_list[i].value.oid;
        break;
      case SAI_ACL_TABLE_GROUP_MEMBER_ATTR_ACL_TABLE_ID:
        tableId = attr_list[i].value.oid;
        break;
      case SAI_ACL_TABLE_GROUP_MEMBER_ATTR_PRIORITY:
        priority = attr_list[i].value.u32;
        break;
    }
  }

  if (!tableGroupId || !tableId || !priority) {
    return SAI_STATUS_INVALID_PARAMETER;
  }

  *acl_table_group_member_id = fs->aclTableGroupManager.createMember(
      tableGroupId.value(),
      tableGroupId.value(),
      tableId.value(),
      priority.value());

  return SAI_STATUS_SUCCESS;
}

sai_status_t remove_acl_table_group_member_fn(
    sai_object_id_t acl_table_group_member_id) {
  auto fs = FakeSai::getInstance();
  fs->aclTableGroupManager.removeMember(acl_table_group_member_id);
  return SAI_STATUS_SUCCESS;
}

sai_status_t set_acl_table_group_member_attribute_fn(
    sai_object_id_t /*acl_table_group_member_id*/,
    const sai_attribute_t* attr) {
  return SAI_STATUS_NOT_IMPLEMENTED;

  switch (attr->id) {
    default:
      // SAI spec does not support setting any attribute for ACL table group
      // memeber post creation.
      return SAI_STATUS_NOT_SUPPORTED;
  }

  return SAI_STATUS_SUCCESS;
}

sai_status_t get_acl_table_group_member_attribute_fn(
    sai_object_id_t acl_table_group_member_id,
    uint32_t attr_count,
    sai_attribute_t* attr_list) {
  auto fs = FakeSai::getInstance();
  auto& aclTableGroupMember =
      fs->aclTableGroupManager.getMember(acl_table_group_member_id);

  for (int i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      case SAI_ACL_TABLE_GROUP_MEMBER_ATTR_ACL_TABLE_GROUP_ID:
        attr_list[i].value.oid = aclTableGroupMember.tableGroupId;
        break;
      case SAI_ACL_TABLE_GROUP_MEMBER_ATTR_ACL_TABLE_ID:
        attr_list[i].value.oid = aclTableGroupMember.tableId;
        break;
      case SAI_ACL_TABLE_GROUP_MEMBER_ATTR_PRIORITY:
        attr_list[i].value.u32 = aclTableGroupMember.priority;
        break;
      default:
        return SAI_STATUS_NOT_SUPPORTED;
    }
  }
  return SAI_STATUS_SUCCESS;
}

namespace facebook::fboss {

sai_acl_api_t* FakeAclTable::kApi() {
  static sai_acl_api_t kAclApi = {&create_acl_table_fn,
                                  &remove_acl_table_fn,
                                  &set_acl_table_attribute_fn,
                                  &get_acl_table_attribute_fn,
                                  &create_acl_entry_fn,
                                  &remove_acl_entry_fn,
                                  &set_acl_entry_attribute_fn,
                                  &get_acl_entry_attribute_fn,
                                  &create_acl_counter_fn,
                                  &remove_acl_counter_fn,
                                  &set_acl_counter_attribute_fn,
                                  &get_acl_counter_attribute_fn,
                                  &create_acl_range_fn,
                                  &remove_acl_range_fn,
                                  &set_acl_range_attribute_fn,
                                  &get_acl_range_attribute_fn,
                                  &create_acl_table_group_fn,
                                  &remove_acl_table_group_fn,
                                  &set_acl_table_group_attribute_fn,
                                  &get_acl_table_group_attribute_fn,
                                  &create_acl_table_group_member_fn,
                                  &remove_acl_table_group_member_fn,
                                  &set_acl_table_group_member_attribute_fn,
                                  &get_acl_table_group_member_attribute_fn};

  return &kAclApi;
}

void populate_acl_api(sai_acl_api_t** acl_api) {
  *acl_api = FakeAclTable::kApi();
}

} // namespace facebook::fboss
