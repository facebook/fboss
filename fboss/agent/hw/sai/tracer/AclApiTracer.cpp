/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <typeindex>
#include <utility>

#include "fboss/agent/hw/sai/api/AclApi.h"
#include "fboss/agent/hw/sai/tracer/AclApiTracer.h"
#include "fboss/agent/hw/sai/tracer/Utils.h"

using folly::to;

namespace {
std::map<int32_t, std::pair<std::string, std::size_t>> _AclTableMap{
    SAI_ATTR_MAP(AclTable, Stage),
    SAI_ATTR_MAP(AclTable, Stage),
    SAI_ATTR_MAP(AclTable, BindPointTypeList),
    SAI_ATTR_MAP(AclTable, ActionTypeList),
    SAI_ATTR_MAP(AclTable, EntryList),
    SAI_ATTR_MAP(AclTable, FieldSrcIpV6),
    SAI_ATTR_MAP(AclTable, FieldDstIpV6),
    SAI_ATTR_MAP(AclTable, FieldSrcIpV4),
    SAI_ATTR_MAP(AclTable, FieldDstIpV4),
    SAI_ATTR_MAP(AclTable, FieldL4SrcPort),
    SAI_ATTR_MAP(AclTable, FieldL4DstPort),
    SAI_ATTR_MAP(AclTable, FieldIpProtocol),
    SAI_ATTR_MAP(AclTable, FieldTcpFlags),
    SAI_ATTR_MAP(AclTable, FieldSrcPort),
    SAI_ATTR_MAP(AclTable, FieldOutPort),
    SAI_ATTR_MAP(AclTable, FieldIpFrag),
    SAI_ATTR_MAP(AclTable, FieldIcmpV4Type),
    SAI_ATTR_MAP(AclTable, FieldIcmpV4Code),
    SAI_ATTR_MAP(AclTable, FieldIcmpV6Type),
    SAI_ATTR_MAP(AclTable, FieldIcmpV6Code),
    SAI_ATTR_MAP(AclTable, FieldDscp),
    SAI_ATTR_MAP(AclTable, FieldDstMac),
    SAI_ATTR_MAP(AclTable, FieldIpType),
    SAI_ATTR_MAP(AclTable, FieldTtl),
    SAI_ATTR_MAP(AclTable, FieldFdbDstUserMeta),
    SAI_ATTR_MAP(AclTable, FieldRouteDstUserMeta),
    SAI_ATTR_MAP(AclTable, FieldNeighborDstUserMeta),
    SAI_ATTR_MAP(AclTable, AvailableEntry),
    SAI_ATTR_MAP(AclTable, AvailableCounter),
    SAI_ATTR_MAP(AclTable, FieldEthertype),
};

std::map<int32_t, std::pair<std::string, std::size_t>> _AclCounterMap{
    SAI_ATTR_MAP(AclCounter, TableId),
    SAI_ATTR_MAP(AclCounter, EnablePacketCount),
    SAI_ATTR_MAP(AclCounter, EnableByteCount),
    SAI_ATTR_MAP(AclCounter, CounterPackets),
    SAI_ATTR_MAP(AclCounter, CounterBytes),
};

} // namespace

namespace facebook::fboss {

WRAP_CREATE_FUNC(acl_counter, SAI_OBJECT_TYPE_ACL_COUNTER, acl);
WRAP_REMOVE_FUNC(acl_counter, SAI_OBJECT_TYPE_ACL_COUNTER, acl);
WRAP_SET_ATTR_FUNC(acl_counter, SAI_OBJECT_TYPE_ACL_COUNTER, acl);
WRAP_GET_ATTR_FUNC(acl_counter, SAI_OBJECT_TYPE_ACL_COUNTER, acl);

WRAP_CREATE_FUNC(acl_entry, SAI_OBJECT_TYPE_ACL_ENTRY, acl);
WRAP_REMOVE_FUNC(acl_entry, SAI_OBJECT_TYPE_ACL_ENTRY, acl);
WRAP_SET_ATTR_FUNC(acl_entry, SAI_OBJECT_TYPE_ACL_ENTRY, acl);
WRAP_GET_ATTR_FUNC(acl_entry, SAI_OBJECT_TYPE_ACL_ENTRY, acl);

WRAP_CREATE_FUNC(acl_table, SAI_OBJECT_TYPE_ACL_TABLE, acl);
WRAP_REMOVE_FUNC(acl_table, SAI_OBJECT_TYPE_ACL_TABLE, acl);
WRAP_SET_ATTR_FUNC(acl_table, SAI_OBJECT_TYPE_ACL_TABLE, acl);
WRAP_GET_ATTR_FUNC(acl_table, SAI_OBJECT_TYPE_ACL_TABLE, acl);

WRAP_CREATE_FUNC(acl_table_group, SAI_OBJECT_TYPE_ACL_TABLE_GROUP, acl);
WRAP_REMOVE_FUNC(acl_table_group, SAI_OBJECT_TYPE_ACL_TABLE_GROUP, acl);
WRAP_SET_ATTR_FUNC(acl_table_group, SAI_OBJECT_TYPE_ACL_TABLE_GROUP, acl);
WRAP_GET_ATTR_FUNC(acl_table_group, SAI_OBJECT_TYPE_ACL_TABLE_GROUP, acl);

WRAP_CREATE_FUNC(
    acl_table_group_member,
    SAI_OBJECT_TYPE_ACL_TABLE_GROUP_MEMBER,
    acl);
WRAP_REMOVE_FUNC(
    acl_table_group_member,
    SAI_OBJECT_TYPE_ACL_TABLE_GROUP_MEMBER,
    acl);
WRAP_SET_ATTR_FUNC(
    acl_table_group_member,
    SAI_OBJECT_TYPE_ACL_TABLE_GROUP_MEMBER,
    acl);
WRAP_GET_ATTR_FUNC(
    acl_table_group_member,
    SAI_OBJECT_TYPE_ACL_TABLE_GROUP_MEMBER,
    acl);

sai_status_t wrap_create_acl_range(
    sai_object_id_t* acl_range_id,
    sai_object_id_t switch_id,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  // TODO: To be implemented if the function is used by fboss
  return SaiTracer::getInstance()->aclApi_->create_acl_range(
      acl_range_id, switch_id, attr_count, attr_list);
}

sai_status_t wrap_remove_acl_range(sai_object_id_t acl_range_id) {
  // TODO: To be implemented if the function is used by fboss
  return SaiTracer::getInstance()->aclApi_->remove_acl_range(acl_range_id);
}

sai_status_t wrap_set_acl_range_attribute(
    sai_object_id_t acl_range_id,
    const sai_attribute_t* attr) {
  // TODO: To be implemented if the function is used by fboss
  return SaiTracer::getInstance()->aclApi_->set_acl_range_attribute(
      acl_range_id, attr);
}

sai_status_t wrap_get_acl_range_attribute(
    sai_object_id_t acl_range_id,
    uint32_t attr_count,
    sai_attribute_t* attr_list) {
  // TODO: To be implemented if the function is used by fboss
  return SaiTracer::getInstance()->aclApi_->get_acl_range_attribute(
      acl_range_id, attr_count, attr_list);
}

sai_acl_api_t* wrappedAclApi() {
  static sai_acl_api_t aclWrappers;

  aclWrappers.create_acl_table = &wrap_create_acl_table;
  aclWrappers.remove_acl_table = &wrap_remove_acl_table;
  aclWrappers.set_acl_table_attribute = &wrap_set_acl_table_attribute;
  aclWrappers.get_acl_table_attribute = &wrap_get_acl_table_attribute;
  aclWrappers.create_acl_entry = &wrap_create_acl_entry;
  aclWrappers.remove_acl_entry = &wrap_remove_acl_entry;
  aclWrappers.set_acl_entry_attribute = &wrap_set_acl_entry_attribute;
  aclWrappers.get_acl_entry_attribute = &wrap_get_acl_entry_attribute;
  aclWrappers.create_acl_counter = &wrap_create_acl_counter;
  aclWrappers.remove_acl_counter = &wrap_remove_acl_counter;
  aclWrappers.set_acl_counter_attribute = &wrap_set_acl_counter_attribute;
  aclWrappers.get_acl_counter_attribute = &wrap_get_acl_counter_attribute;
  aclWrappers.create_acl_range = &wrap_create_acl_range;
  aclWrappers.remove_acl_range = &wrap_remove_acl_range;
  aclWrappers.set_acl_range_attribute = &wrap_set_acl_range_attribute;
  aclWrappers.get_acl_range_attribute = &wrap_get_acl_range_attribute;
  aclWrappers.create_acl_table_group = &wrap_create_acl_table_group;
  aclWrappers.remove_acl_table_group = &wrap_remove_acl_table_group;
  aclWrappers.set_acl_table_group_attribute =
      &wrap_set_acl_table_group_attribute;
  aclWrappers.get_acl_table_group_attribute =
      &wrap_get_acl_table_group_attribute;
  aclWrappers.create_acl_table_group_member =
      &wrap_create_acl_table_group_member;
  aclWrappers.remove_acl_table_group_member =
      &wrap_remove_acl_table_group_member;
  aclWrappers.set_acl_table_group_member_attribute =
      &wrap_set_acl_table_group_member_attribute;
  aclWrappers.get_acl_table_group_member_attribute =
      &wrap_get_acl_table_group_member_attribute;

  return &aclWrappers;
}

SET_SAI_ATTRIBUTES(AclCounter)
SET_SAI_ATTRIBUTES(AclTable)

void setAclEntryAttributes(
    const sai_attribute_t* attr_list,
    uint32_t attr_count,
    std::vector<std::string>& attrLines) {
  uint32_t listCount = 0;

  for (int i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      case SAI_ACL_ENTRY_ATTR_TABLE_ID:
        attrLines.push_back(oidAttr(attr_list, i));
        break;
      case SAI_ACL_ENTRY_ATTR_PRIORITY:
        attrLines.push_back(u32Attr(attr_list, i));
        break;
      case SAI_ACL_ENTRY_ATTR_FIELD_SRC_IPV6:
      case SAI_ACL_ENTRY_ATTR_FIELD_DST_IPV6:
        aclEntryFieldIpV6Attr(attr_list, i, attrLines);
        break;
      case SAI_ACL_ENTRY_ATTR_FIELD_SRC_IP:
      case SAI_ACL_ENTRY_ATTR_FIELD_DST_IP:
        aclEntryFieldIpV4Attr(attr_list, i, attrLines);
        break;
      case SAI_ACL_ENTRY_ATTR_FIELD_SRC_PORT:
      case SAI_ACL_ENTRY_ATTR_FIELD_OUT_PORT:
        aclEntryFieldSaiObjectIdAttr(attr_list, i, attrLines);
        break;
      case SAI_ACL_ENTRY_ATTR_ACTION_COUNTER:
        aclEntryActionSaiObjectIdAttr(attr_list, i, attrLines);
        break;
      case SAI_ACL_ENTRY_ATTR_ACTION_MIRROR_INGRESS:
      case SAI_ACL_ENTRY_ATTR_ACTION_MIRROR_EGRESS:
        aclEntryActionSaiObjectIdListAttr(attr_list, i, listCount++, attrLines);
        break;
      case SAI_ACL_ENTRY_ATTR_FIELD_ACL_IP_TYPE:
      case SAI_ACL_ENTRY_ATTR_FIELD_ACL_IP_FRAG:
      case SAI_ACL_ENTRY_ATTR_FIELD_FDB_DST_USER_META:
      case SAI_ACL_ENTRY_ATTR_FIELD_ROUTE_DST_USER_META:
      case SAI_ACL_ENTRY_ATTR_FIELD_NEIGHBOR_DST_USER_META:
        aclEntryFieldU32Attr(attr_list, i, attrLines);
        break;
      case SAI_ACL_ENTRY_ATTR_ACTION_PACKET_ACTION:
        aclEntryActionU32Attr(attr_list, i, attrLines);
        break;
      case SAI_ACL_ENTRY_ATTR_FIELD_L4_SRC_PORT:
      case SAI_ACL_ENTRY_ATTR_FIELD_L4_DST_PORT:
        aclEntryFieldU16Attr(attr_list, i, attrLines);
        break;
      case SAI_ACL_ENTRY_ATTR_FIELD_IP_PROTOCOL:
      case SAI_ACL_ENTRY_ATTR_FIELD_TCP_FLAGS:
      case SAI_ACL_ENTRY_ATTR_FIELD_ICMP_TYPE:
      case SAI_ACL_ENTRY_ATTR_FIELD_ICMP_CODE:
      case SAI_ACL_ENTRY_ATTR_FIELD_ICMPV6_TYPE:
      case SAI_ACL_ENTRY_ATTR_FIELD_ICMPV6_CODE:
      case SAI_ACL_ENTRY_ATTR_FIELD_DSCP:
      case SAI_ACL_ENTRY_ATTR_FIELD_TTL:
        aclEntryFieldU8Attr(attr_list, i, attrLines);
        break;
      case SAI_ACL_ENTRY_ATTR_ACTION_SET_TC:
      case SAI_ACL_ENTRY_ATTR_ACTION_SET_DSCP:
        aclEntryActionU8Attr(attr_list, i, attrLines);
        break;
      case SAI_ACL_ENTRY_ATTR_FIELD_DST_MAC:
        aclEntryFieldMacAttr(attr_list, i, attrLines);
        break;
      default:
        break;
    }
  }
}

void setAclTableGroupAttributes(
    const sai_attribute_t* attr_list,
    uint32_t attr_count,
    std::vector<std::string>& attrLines) {
  uint32_t listCount = 0;

  for (int i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      case SAI_ACL_TABLE_GROUP_ATTR_ACL_STAGE:
      case SAI_ACL_TABLE_GROUP_ATTR_TYPE:
        attrLines.push_back(s32Attr(attr_list, i));
        break;
      case SAI_ACL_TABLE_GROUP_ATTR_ACL_BIND_POINT_TYPE_LIST:
        s32ListAttr(attr_list, i, listCount++, attrLines);
        break;
      case SAI_ACL_TABLE_GROUP_ATTR_MEMBER_LIST:
        oidListAttr(attr_list, i, listCount++, attrLines);
        break;
      default:
        break;
    }
  }
}

void setAclTableGroupMemberAttributes(
    const sai_attribute_t* attr_list,
    uint32_t attr_count,
    std::vector<std::string>& attrLines) {
  for (int i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      case SAI_ACL_TABLE_GROUP_MEMBER_ATTR_ACL_TABLE_ID:
      case SAI_ACL_TABLE_GROUP_MEMBER_ATTR_ACL_TABLE_GROUP_ID:
        attrLines.push_back(oidAttr(attr_list, i));
        break;
      case SAI_ACL_TABLE_GROUP_MEMBER_ATTR_PRIORITY:
        attrLines.push_back(u32Attr(attr_list, i));
        break;
      default:
        break;
    }
  }
}

} // namespace facebook::fboss
