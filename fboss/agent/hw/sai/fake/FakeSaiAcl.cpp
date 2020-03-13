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

#include "fboss/agent/hw/sai/fake/FakeSai.h"

sai_status_t create_acl_table_fn(
    sai_object_id_t* /*acl_table_id*/,
    sai_object_id_t /*switch_id */,
    uint32_t /*attr_count */,
    const sai_attribute_t* /*attr_list*/) {
  // TODO (skhare) add support
  return SAI_STATUS_SUCCESS;
}

sai_status_t remove_acl_table_fn(sai_object_id_t /*acl_table_id*/) {
  // TODO (skhare) add support
  return SAI_STATUS_SUCCESS;
}

sai_status_t set_acl_table_attribute_fn(
    sai_object_id_t /*acl_table_id*/,
    const sai_attribute_t* /*attr*/) {
  // TODO (skhare) add support
  return SAI_STATUS_SUCCESS;
}

sai_status_t get_acl_table_attribute_fn(
    sai_object_id_t /*acl_table_id*/,
    uint32_t /*attr_count*/,
    sai_attribute_t* /*attr*/) {
  // TODO (skhare) add support
  return SAI_STATUS_SUCCESS;
}

sai_status_t create_acl_entry_fn(
    sai_object_id_t* acl_entry_id,
    sai_object_id_t switch_id,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  return SAI_STATUS_NOT_IMPLEMENTED;
}

sai_status_t remove_acl_entry_fn(sai_object_id_t acl_entry_id) {
  return SAI_STATUS_NOT_IMPLEMENTED;
}

sai_status_t set_acl_entry_attribute_fn(
    sai_object_id_t acl_entry_id,
    const sai_attribute_t* attr) {
  return SAI_STATUS_NOT_IMPLEMENTED;
}

sai_status_t get_acl_entry_attribute_fn(
    sai_object_id_t acl_entry_id,
    uint32_t attr_count,
    sai_attribute_t* attr_list) {
  return SAI_STATUS_NOT_IMPLEMENTED;
}

sai_status_t create_acl_counter_fn(
    sai_object_id_t* acl_counter_id,
    sai_object_id_t switch_id,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  return SAI_STATUS_NOT_IMPLEMENTED;
}

sai_status_t remove_acl_counter_fn(sai_object_id_t acl_counter_id) {
  return SAI_STATUS_NOT_IMPLEMENTED;
}

sai_status_t set_acl_counter_attribute_fn(
    sai_object_id_t acl_counter_id,
    const sai_attribute_t* attr) {
  return SAI_STATUS_NOT_IMPLEMENTED;
}

sai_status_t get_acl_counter_attribute_fn(
    sai_object_id_t acl_counter_id,
    uint32_t attr_count,
    sai_attribute_t* attr_list) {
  return SAI_STATUS_NOT_IMPLEMENTED;
}

sai_status_t create_acl_range_fn(
    sai_object_id_t* acl_range_id,
    sai_object_id_t switch_id,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  return SAI_STATUS_NOT_IMPLEMENTED;
}

sai_status_t remove_acl_range_fn(sai_object_id_t acl_range_id) {
  return SAI_STATUS_NOT_IMPLEMENTED;
}

sai_status_t set_acl_range_attribute_fn(
    sai_object_id_t acl_range_id,
    const sai_attribute_t* attr) {
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
  return SAI_STATUS_NOT_IMPLEMENTED;
}

sai_status_t remove_acl_table_group_fn(sai_object_id_t acl_table_group_id) {
  return SAI_STATUS_NOT_IMPLEMENTED;
}

sai_status_t set_acl_table_group_attribute_fn(
    sai_object_id_t acl_table_group_id,
    const sai_attribute_t* attr) {
  return SAI_STATUS_NOT_IMPLEMENTED;
}

sai_status_t get_acl_table_group_attribute_fn(
    sai_object_id_t acl_table_group_id,
    uint32_t attr_count,
    sai_attribute_t* attr_list) {
  return SAI_STATUS_NOT_IMPLEMENTED;
}

sai_status_t create_acl_table_group_member_fn(
    sai_object_id_t* acl_table_group_member_id,
    sai_object_id_t switch_id,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  return SAI_STATUS_NOT_IMPLEMENTED;
}

sai_status_t remove_acl_table_group_member_fn(
    sai_object_id_t acl_table_group_member_id) {
  return SAI_STATUS_NOT_IMPLEMENTED;
}

sai_status_t set_acl_table_group_member_attribute_fn(
    sai_object_id_t acl_table_group_member_id,
    const sai_attribute_t* attr) {
  return SAI_STATUS_NOT_IMPLEMENTED;
}

sai_status_t get_acl_table_group_member_attribute_fn(
    sai_object_id_t acl_table_group_member_id,
    uint32_t attr_count,
    sai_attribute_t* attr_list) {
  return SAI_STATUS_NOT_IMPLEMENTED;
}

namespace facebook::fboss {
static sai_acl_api_t _acl_api;

sai_acl_api_t* FakeAcl::kApi() {
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
  *acl_api = FakeAcl::kApi();
}

} // namespace facebook::fboss
