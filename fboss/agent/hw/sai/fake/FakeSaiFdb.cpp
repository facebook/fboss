/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/sai/fake/FakeSaiFdb.h"
#include "fboss/agent/hw/sai/fake/FakeSai.h"

#include "fboss/agent/hw/sai/api/AddressUtil.h"

#include <folly/logging/xlog.h>
#include <optional>

using facebook::fboss::FakeFdb;
using facebook::fboss::FakeSai;

sai_status_t create_fdb_entry_fn(
    const sai_fdb_entry_t* fdb_entry,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  auto fs = FakeSai::getInstance();
  auto mac = facebook::fboss::fromSaiMacAddress(fdb_entry->mac_address);
  sai_object_id_t bridgePortId = 0;
  sai_uint32_t metadata{0};
  for (int i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      case SAI_FDB_ENTRY_ATTR_BRIDGE_PORT_ID:
        bridgePortId = attr_list[i].value.oid;
        break;
      case SAI_FDB_ENTRY_ATTR_TYPE:
        break;
      case SAI_FDB_ENTRY_ATTR_META_DATA:
        metadata = attr_list[i].value.u32;
        break;
      default:
        return SAI_STATUS_INVALID_PARAMETER;
    }
  }
  fs->fdbManager.create(
      std::make_tuple(fdb_entry->switch_id, fdb_entry->bv_id, mac),
      bridgePortId,
      metadata);
  return SAI_STATUS_SUCCESS;
}

sai_status_t remove_fdb_entry_fn(const sai_fdb_entry_t* fdb_entry) {
  auto fs = FakeSai::getInstance();
  auto mac = facebook::fboss::fromSaiMacAddress(fdb_entry->mac_address);
  fs->fdbManager.remove(
      std::make_tuple(fdb_entry->switch_id, fdb_entry->bv_id, mac));
  return SAI_STATUS_SUCCESS;
}

sai_status_t set_fdb_entry_attribute_fn(
    const sai_fdb_entry_t* fdb_entry,
    const sai_attribute_t* attr) {
  auto fs = FakeSai::getInstance();
  auto mac = facebook::fboss::fromSaiMacAddress(fdb_entry->mac_address);
  auto fdbKey = std::make_tuple(fdb_entry->switch_id, fdb_entry->bv_id, mac);
  auto& fdbEntry = fs->fdbManager.get(fdbKey);
  switch (attr->id) {
    case SAI_FDB_ENTRY_ATTR_BRIDGE_PORT_ID:
      fdbEntry.bridgePortId = attr->value.oid;
      break;
    case SAI_FDB_ENTRY_ATTR_META_DATA:
      fdbEntry.metadata = attr->value.u32;
      break;
    default:
      return SAI_STATUS_INVALID_PARAMETER;
  }
  return SAI_STATUS_SUCCESS;
}

sai_status_t get_fdb_entry_attribute_fn(
    const sai_fdb_entry_t* fdb_entry,
    uint32_t attr_count,
    sai_attribute_t* attr_list) {
  auto fs = FakeSai::getInstance();
  auto mac = facebook::fboss::fromSaiMacAddress(fdb_entry->mac_address);
  auto fdbKey = std::make_tuple(fdb_entry->switch_id, fdb_entry->bv_id, mac);
  auto& fdbEntry = fs->fdbManager.get(fdbKey);
  for (int i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      case SAI_FDB_ENTRY_ATTR_BRIDGE_PORT_ID:
        attr_list[i].value.oid = fdbEntry.bridgePortId;
        break;
      case SAI_FDB_ENTRY_ATTR_TYPE:
        // Only static fdb entries for now
        attr_list[i].value.s32 = SAI_FDB_ENTRY_TYPE_STATIC;
        break;
      case SAI_FDB_ENTRY_ATTR_META_DATA:
        attr_list[i].value.u32 = fdbEntry.metadata;
        break;
      default:
        return SAI_STATUS_INVALID_PARAMETER;
    }
  }
  return SAI_STATUS_SUCCESS;
}

namespace facebook::fboss {

static sai_fdb_api_t _fdb_api;

void populate_fdb_api(sai_fdb_api_t** fdb_api) {
  _fdb_api.create_fdb_entry = &create_fdb_entry_fn;
  _fdb_api.remove_fdb_entry = &remove_fdb_entry_fn;
  _fdb_api.set_fdb_entry_attribute = &set_fdb_entry_attribute_fn;
  _fdb_api.get_fdb_entry_attribute = &get_fdb_entry_attribute_fn;
  *fdb_api = &_fdb_api;
}

} // namespace facebook::fboss
