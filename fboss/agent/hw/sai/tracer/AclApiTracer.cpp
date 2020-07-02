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

namespace facebook::fboss {

sai_status_t wrap_create_acl_table(
    sai_object_id_t* acl_table_id,
    sai_object_id_t switch_id,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  return SaiTracer::getInstance()->aclApi_->create_acl_table(
      acl_table_id, switch_id, attr_count, attr_list);
}

sai_status_t wrap_remove_acl_table(sai_object_id_t acl_table_id) {
  return SaiTracer::getInstance()->aclApi_->remove_acl_table(acl_table_id);
}

sai_status_t wrap_set_acl_table_attribute(
    sai_object_id_t acl_table_id,
    const sai_attribute_t* attr) {
  return SaiTracer::getInstance()->aclApi_->set_acl_table_attribute(
      acl_table_id, attr);
}

sai_status_t wrap_get_acl_table_attribute(
    sai_object_id_t acl_table_id,
    uint32_t attr_count,
    sai_attribute_t* attr_list) {
  return SaiTracer::getInstance()->aclApi_->get_acl_table_attribute(
      acl_table_id, attr_count, attr_list);
}

sai_status_t wrap_create_acl_entry(
    sai_object_id_t* acl_entry_id,
    sai_object_id_t switch_id,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  return SaiTracer::getInstance()->aclApi_->create_acl_entry(
      acl_entry_id, switch_id, attr_count, attr_list);
}

sai_status_t wrap_remove_acl_entry(sai_object_id_t acl_entry_id) {
  return SaiTracer::getInstance()->aclApi_->remove_acl_entry(acl_entry_id);
}

sai_status_t wrap_set_acl_entry_attribute(
    sai_object_id_t acl_entry_id,
    const sai_attribute_t* attr) {
  return SaiTracer::getInstance()->aclApi_->set_acl_entry_attribute(
      acl_entry_id, attr);
}

sai_status_t wrap_get_acl_entry_attribute(
    sai_object_id_t acl_entry_id,
    uint32_t attr_count,
    sai_attribute_t* attr_list) {
  return SaiTracer::getInstance()->aclApi_->get_acl_entry_attribute(
      acl_entry_id, attr_count, attr_list);
}

sai_status_t wrap_create_acl_counter(
    sai_object_id_t* acl_counter_id,
    sai_object_id_t switch_id,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  return SaiTracer::getInstance()->aclApi_->create_acl_counter(
      acl_counter_id, switch_id, attr_count, attr_list);
}

sai_status_t wrap_remove_acl_counter(sai_object_id_t acl_counter_id) {
  return SaiTracer::getInstance()->aclApi_->remove_acl_counter(acl_counter_id);
}

sai_status_t wrap_set_acl_counter_attribute(
    sai_object_id_t acl_counter_id,
    const sai_attribute_t* attr) {
  return SaiTracer::getInstance()->aclApi_->set_acl_counter_attribute(
      acl_counter_id, attr);
}

sai_status_t wrap_get_acl_counter_attribute(
    sai_object_id_t acl_counter_id,
    uint32_t attr_count,
    sai_attribute_t* attr_list) {
  return SaiTracer::getInstance()->aclApi_->get_acl_counter_attribute(
      acl_counter_id, attr_count, attr_list);
}

sai_status_t wrap_create_acl_range(
    sai_object_id_t* acl_range_id,
    sai_object_id_t switch_id,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  return SaiTracer::getInstance()->aclApi_->create_acl_range(
      acl_range_id, switch_id, attr_count, attr_list);
}

sai_status_t wrap_remove_acl_range(sai_object_id_t acl_range_id) {
  return SaiTracer::getInstance()->aclApi_->remove_acl_range(acl_range_id);
}

sai_status_t wrap_set_acl_range_attribute(
    sai_object_id_t acl_range_id,
    const sai_attribute_t* attr) {
  return SaiTracer::getInstance()->aclApi_->set_acl_range_attribute(
      acl_range_id, attr);
}

sai_status_t wrap_get_acl_range_attribute(
    sai_object_id_t acl_range_id,
    uint32_t attr_count,
    sai_attribute_t* attr_list) {
  return SaiTracer::getInstance()->aclApi_->get_acl_range_attribute(
      acl_range_id, attr_count, attr_list);
}

sai_status_t wrap_create_acl_table_group(
    sai_object_id_t* acl_table_group_id,
    sai_object_id_t switch_id,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  return SaiTracer::getInstance()->aclApi_->create_acl_table_group(
      acl_table_group_id, switch_id, attr_count, attr_list);
}

sai_status_t wrap_remove_acl_table_group(sai_object_id_t acl_table_group_id) {
  return SaiTracer::getInstance()->aclApi_->remove_acl_table_group(
      acl_table_group_id);
}

sai_status_t wrap_set_acl_table_group_attribute(
    sai_object_id_t acl_table_group_id,
    const sai_attribute_t* attr) {
  return SaiTracer::getInstance()->aclApi_->set_acl_table_group_attribute(
      acl_table_group_id, attr);
}

sai_status_t wrap_get_acl_table_group_attribute(
    sai_object_id_t acl_table_group_id,
    uint32_t attr_count,
    sai_attribute_t* attr_list) {
  return SaiTracer::getInstance()->aclApi_->get_acl_table_group_attribute(
      acl_table_group_id, attr_count, attr_list);
}

sai_status_t wrap_create_acl_table_group_member(
    sai_object_id_t* acl_table_group_member_id,
    sai_object_id_t switch_id,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  return SaiTracer::getInstance()->aclApi_->create_acl_table_group_member(
      acl_table_group_member_id, switch_id, attr_count, attr_list);
}

sai_status_t wrap_remove_acl_table_group_member(
    sai_object_id_t acl_table_group_member_id) {
  return SaiTracer::getInstance()->aclApi_->remove_acl_table_group_member(
      acl_table_group_member_id);
}

sai_status_t wrap_set_acl_table_group_member_attribute(
    sai_object_id_t acl_table_group_member_id,
    const sai_attribute_t* attr) {
  return SaiTracer::getInstance()
      ->aclApi_->set_acl_table_group_member_attribute(
          acl_table_group_member_id, attr);
}

sai_status_t wrap_get_acl_table_group_member_attribute(
    sai_object_id_t acl_table_group_member_id,
    uint32_t attr_count,
    sai_attribute_t* attr_list) {
  return SaiTracer::getInstance()
      ->aclApi_->get_acl_table_group_member_attribute(
          acl_table_group_member_id, attr_count, attr_list);
}

sai_acl_api_t* wrapAclApi() {
  static sai_acl_api_t aclWrappers = {
      &wrap_create_acl_table,
      &wrap_remove_acl_table,
      &wrap_set_acl_table_attribute,
      &wrap_get_acl_table_attribute,
      &wrap_create_acl_entry,
      &wrap_remove_acl_entry,
      &wrap_set_acl_entry_attribute,
      &wrap_get_acl_entry_attribute,
      &wrap_create_acl_counter,
      &wrap_remove_acl_counter,
      &wrap_set_acl_counter_attribute,
      &wrap_get_acl_counter_attribute,
      &wrap_create_acl_range,
      &wrap_remove_acl_range,
      &wrap_set_acl_range_attribute,
      &wrap_get_acl_range_attribute,
      &wrap_create_acl_table_group,
      &wrap_remove_acl_table_group,
      &wrap_set_acl_table_group_attribute,
      &wrap_get_acl_table_group_attribute,
      &wrap_create_acl_table_group_member,
      &wrap_remove_acl_table_group_member,
      &wrap_set_acl_table_group_member_attribute,
      &wrap_get_acl_table_group_member_attribute};

  return &aclWrappers;
}

} // namespace facebook::fboss
