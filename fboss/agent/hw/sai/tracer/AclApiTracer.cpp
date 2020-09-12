/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/tracer/AclApiTracer.h"
#include "fboss/agent/hw/sai/tracer/Utils.h"

namespace facebook::fboss {

sai_status_t wrap_create_acl_table(
    sai_object_id_t* acl_table_id,
    sai_object_id_t switch_id,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  auto rv = SaiTracer::getInstance()->aclApi_->create_acl_table(
      acl_table_id, switch_id, attr_count, attr_list);

  SaiTracer::getInstance()->logCreateFn(
      "create_acl_table",
      acl_table_id,
      switch_id,
      attr_count,
      attr_list,
      SAI_OBJECT_TYPE_ACL_TABLE,
      rv);
  return rv;
}

sai_status_t wrap_remove_acl_table(sai_object_id_t acl_table_id) {
  auto rv = SaiTracer::getInstance()->aclApi_->remove_acl_table(acl_table_id);

  SaiTracer::getInstance()->logRemoveFn(
      "remove_acl_table", acl_table_id, SAI_OBJECT_TYPE_ACL_TABLE, rv);
  return rv;
}

sai_status_t wrap_set_acl_table_attribute(
    sai_object_id_t acl_table_id,
    const sai_attribute_t* attr) {
  auto rv = SaiTracer::getInstance()->aclApi_->set_acl_table_attribute(
      acl_table_id, attr);

  SaiTracer::getInstance()->logSetAttrFn(
      "set_acl_table_attribute",
      acl_table_id,
      attr,
      SAI_OBJECT_TYPE_ACL_TABLE,
      rv);
  return rv;
}

sai_status_t wrap_get_acl_table_attribute(
    sai_object_id_t acl_table_id,
    uint32_t attr_count,
    sai_attribute_t* attr_list) {
  // TODO(zecheng): Log get functions as well

  return SaiTracer::getInstance()->aclApi_->get_acl_table_attribute(
      acl_table_id, attr_count, attr_list);
}

sai_status_t wrap_create_acl_entry(
    sai_object_id_t* acl_entry_id,
    sai_object_id_t switch_id,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  auto rv = SaiTracer::getInstance()->aclApi_->create_acl_entry(
      acl_entry_id, switch_id, attr_count, attr_list);

  SaiTracer::getInstance()->logCreateFn(
      "create_acl_entry",
      acl_entry_id,
      switch_id,
      attr_count,
      attr_list,
      SAI_OBJECT_TYPE_ACL_ENTRY,
      rv);
  return rv;
}

sai_status_t wrap_remove_acl_entry(sai_object_id_t acl_entry_id) {
  auto rv = SaiTracer::getInstance()->aclApi_->remove_acl_entry(acl_entry_id);

  SaiTracer::getInstance()->logRemoveFn(
      "remove_acl_entry", acl_entry_id, SAI_OBJECT_TYPE_ACL_ENTRY, rv);
  return rv;
}

sai_status_t wrap_set_acl_entry_attribute(
    sai_object_id_t acl_entry_id,
    const sai_attribute_t* attr) {
  auto rv = SaiTracer::getInstance()->aclApi_->set_acl_entry_attribute(
      acl_entry_id, attr);

  SaiTracer::getInstance()->logSetAttrFn(
      "set_acl_entry_attribute",
      acl_entry_id,
      attr,
      SAI_OBJECT_TYPE_ACL_ENTRY,
      rv);
  return rv;
}

sai_status_t wrap_get_acl_entry_attribute(
    sai_object_id_t acl_entry_id,
    uint32_t attr_count,
    sai_attribute_t* attr_list) {
  // TODO(zecheng): Log get functions as well
  return SaiTracer::getInstance()->aclApi_->get_acl_entry_attribute(
      acl_entry_id, attr_count, attr_list);
}

sai_status_t wrap_create_acl_counter(
    sai_object_id_t* acl_counter_id,
    sai_object_id_t switch_id,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  // TODO: To be implemented if the function is used by fboss
  return SaiTracer::getInstance()->aclApi_->create_acl_counter(
      acl_counter_id, switch_id, attr_count, attr_list);
}

sai_status_t wrap_remove_acl_counter(sai_object_id_t acl_counter_id) {
  // TODO: To be implemented if the function is used by fboss
  return SaiTracer::getInstance()->aclApi_->remove_acl_counter(acl_counter_id);
}

sai_status_t wrap_set_acl_counter_attribute(
    sai_object_id_t acl_counter_id,
    const sai_attribute_t* attr) {
  // TODO: To be implemented if the function is used by fboss
  return SaiTracer::getInstance()->aclApi_->set_acl_counter_attribute(
      acl_counter_id, attr);
}

sai_status_t wrap_get_acl_counter_attribute(
    sai_object_id_t acl_counter_id,
    uint32_t attr_count,
    sai_attribute_t* attr_list) {
  // TODO: To be implemented if the function is used by fboss
  return SaiTracer::getInstance()->aclApi_->get_acl_counter_attribute(
      acl_counter_id, attr_count, attr_list);
}

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

sai_status_t wrap_create_acl_table_group(
    sai_object_id_t* acl_table_group_id,
    sai_object_id_t switch_id,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  auto rv = SaiTracer::getInstance()->aclApi_->create_acl_table_group(
      acl_table_group_id, switch_id, attr_count, attr_list);

  SaiTracer::getInstance()->logCreateFn(
      "create_acl_table_group",
      acl_table_group_id,
      switch_id,
      attr_count,
      attr_list,
      SAI_OBJECT_TYPE_ACL_TABLE_GROUP,
      rv);
  return rv;
}

sai_status_t wrap_remove_acl_table_group(sai_object_id_t acl_table_group_id) {
  auto rv = SaiTracer::getInstance()->aclApi_->remove_acl_table_group(
      acl_table_group_id);

  SaiTracer::getInstance()->logRemoveFn(
      "remove_acl_table_group",
      acl_table_group_id,
      SAI_OBJECT_TYPE_ACL_TABLE_GROUP,
      rv);
  return rv;
}

sai_status_t wrap_set_acl_table_group_attribute(
    sai_object_id_t acl_table_group_id,
    const sai_attribute_t* attr) {
  auto rv = SaiTracer::getInstance()->aclApi_->set_acl_table_group_attribute(
      acl_table_group_id, attr);

  SaiTracer::getInstance()->logSetAttrFn(
      "set_acl_table_group_attribute",
      acl_table_group_id,
      attr,
      SAI_OBJECT_TYPE_ACL_TABLE_GROUP,
      rv);
  return rv;
}

sai_status_t wrap_get_acl_table_group_attribute(
    sai_object_id_t acl_table_group_id,
    uint32_t attr_count,
    sai_attribute_t* attr_list) {
  // TODO(zecheng): Log get functions as well
  return SaiTracer::getInstance()->aclApi_->get_acl_table_group_attribute(
      acl_table_group_id, attr_count, attr_list);
}

sai_status_t wrap_create_acl_table_group_member(
    sai_object_id_t* acl_table_group_member_id,
    sai_object_id_t switch_id,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  auto rv = SaiTracer::getInstance()->aclApi_->create_acl_table_group_member(
      acl_table_group_member_id, switch_id, attr_count, attr_list);

  SaiTracer::getInstance()->logCreateFn(
      "create_acl_table_group_member",
      acl_table_group_member_id,
      switch_id,
      attr_count,
      attr_list,
      SAI_OBJECT_TYPE_ACL_TABLE_GROUP_MEMBER,
      rv);
  return rv;
}

sai_status_t wrap_remove_acl_table_group_member(
    sai_object_id_t acl_table_group_member_id) {
  auto rv = SaiTracer::getInstance()->aclApi_->remove_acl_table_group_member(
      acl_table_group_member_id);

  SaiTracer::getInstance()->logRemoveFn(
      "remove_acl_table_group_member",
      acl_table_group_member_id,
      SAI_OBJECT_TYPE_ACL_TABLE_GROUP_MEMBER,
      rv);
  return rv;
}

sai_status_t wrap_set_acl_table_group_member_attribute(
    sai_object_id_t acl_table_group_member_id,
    const sai_attribute_t* attr) {
  auto rv =
      SaiTracer::getInstance()->aclApi_->set_acl_table_group_member_attribute(
          acl_table_group_member_id, attr);

  SaiTracer::getInstance()->logSetAttrFn(
      "set_acl_table_group_member_attribute",
      acl_table_group_member_id,
      attr,
      SAI_OBJECT_TYPE_ACL_TABLE_GROUP_MEMBER,
      rv);
  return rv;
}

sai_status_t wrap_get_acl_table_group_member_attribute(
    sai_object_id_t acl_table_group_member_id,
    uint32_t attr_count,
    sai_attribute_t* attr_list) {
  // TODO(zecheng): Log get functions as well
  return SaiTracer::getInstance()
      ->aclApi_->get_acl_table_group_member_attribute(
          acl_table_group_member_id, attr_count, attr_list);
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
        // TODO(zecheng): Better check for newly added attributes (T69350100)
        break;
    }
  }
}

void setAclTableAttributes(
    const sai_attribute_t* attr_list,
    uint32_t attr_count,
    std::vector<std::string>& attrLines) {
  uint32_t listCount = 0;

  for (int i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      case SAI_ACL_TABLE_ATTR_ACL_STAGE:
        attrLines.push_back(s32Attr(attr_list, i));
        break;
      case SAI_ACL_TABLE_ATTR_ACL_BIND_POINT_TYPE_LIST:
      case SAI_ACL_TABLE_ATTR_ACL_ACTION_TYPE_LIST:
        s32ListAttr(attr_list, i, listCount++, attrLines);
        break;
      case SAI_ACL_TABLE_ATTR_ENTRY_LIST:
        oidListAttr(attr_list, i, listCount++, attrLines);
        break;
      case SAI_ACL_TABLE_ATTR_FIELD_SRC_IPV6:
      case SAI_ACL_TABLE_ATTR_FIELD_DST_IPV6:
      case SAI_ACL_TABLE_ATTR_FIELD_SRC_IP:
      case SAI_ACL_TABLE_ATTR_FIELD_DST_IP:
      case SAI_ACL_TABLE_ATTR_FIELD_L4_SRC_PORT:
      case SAI_ACL_TABLE_ATTR_FIELD_L4_DST_PORT:
      case SAI_ACL_TABLE_ATTR_FIELD_IP_PROTOCOL:
      case SAI_ACL_TABLE_ATTR_FIELD_TCP_FLAGS:
      case SAI_ACL_TABLE_ATTR_FIELD_SRC_PORT:
      case SAI_ACL_TABLE_ATTR_FIELD_OUT_PORT:
      case SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_FRAG:
      case SAI_ACL_TABLE_ATTR_FIELD_ICMP_TYPE:
      case SAI_ACL_TABLE_ATTR_FIELD_ICMP_CODE:
      case SAI_ACL_TABLE_ATTR_FIELD_ICMPV6_TYPE:
      case SAI_ACL_TABLE_ATTR_FIELD_ICMPV6_CODE:
      case SAI_ACL_TABLE_ATTR_FIELD_DSCP:
      case SAI_ACL_TABLE_ATTR_FIELD_DST_MAC:
      case SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_TYPE:
      case SAI_ACL_TABLE_ATTR_FIELD_TTL:
      case SAI_ACL_TABLE_ATTR_FIELD_FDB_DST_USER_META:
      case SAI_ACL_TABLE_ATTR_FIELD_ROUTE_DST_USER_META:
      case SAI_ACL_TABLE_ATTR_FIELD_NEIGHBOR_DST_USER_META:
        attrLines.push_back(boolAttr(attr_list, i));
        break;
      default:
        // TODO(zecheng): What to do in this situation?
        // We need a way to remind/flag an error when new attributes are added
        // to Sai API. By default, this function is not going to set the
        // attribute values, which should be caught by Sai Replayer tests.
        // However, it would be better to have a compile time check (T69350100)
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
        // TODO(zecheng): Better check for newly added attributes (T69350100)
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
        // TODO(zecheng): Better check for newly added attributes (T69350100)
        break;
    }
  }
}

} // namespace facebook::fboss
