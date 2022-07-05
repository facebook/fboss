// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/hw/sai/fake/FakeSaiTunnel.h"
#include "fboss/agent/hw/sai/fake/FakeSai.h"

namespace {

using facebook::fboss::FakeSai;
using facebook::fboss::FakeTunnel;

sai_status_t create_tunnel_fn(
    sai_object_id_t* /* tunnel_id */,
    sai_object_id_t /* switch_id */,
    uint32_t /* attr_count */,
    const sai_attribute_t* /* attr_list */) {
  return SAI_STATUS_SUCCESS;
}

sai_status_t remove_tunnel_fn(sai_object_id_t /* tunnel_id */) {
  return SAI_STATUS_SUCCESS;
}

sai_status_t get_tunnel_attribute_fn(
    sai_object_id_t /* tunnel_id */,
    uint32_t /* attr_count */,
    sai_attribute_t* /* attr */) {
  return SAI_STATUS_SUCCESS;
}

sai_status_t set_tunnel_attribute_fn(
    sai_object_id_t /* tunnel_id */,
    const sai_attribute_t* /* attr */) {
  return SAI_STATUS_SUCCESS;
}

sai_status_t create_tunnel_term_table_entry_fn(
    sai_object_id_t* /* tunnel_term_table_entry_id */,
    sai_object_id_t /* switch_id */,
    uint32_t /* attr_count */,
    const sai_attribute_t* /* attr_list */) {
  return SAI_STATUS_SUCCESS;
}

sai_status_t remove_tunnel_term_table_entry_fn(
    sai_object_id_t /* tunnel_term_table_entry_id */) {
  return SAI_STATUS_SUCCESS;
}

sai_status_t get_tunnel_term_table_entry_attribute_fn(
    sai_object_id_t /* tunnel_term_table_entry_id */,
    uint32_t /* attr_count */,
    sai_attribute_t* /* attr */) {
  return SAI_STATUS_SUCCESS;
}

sai_status_t set_tunnel_term_table_entry_attribute_fn(
    sai_object_id_t /* tunnel_term_table_entry_id */,
    const sai_attribute_t* /* attr */) {
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
