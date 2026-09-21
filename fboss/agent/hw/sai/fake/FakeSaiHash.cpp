/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/fake/FakeSaiHash.h"

#include "fboss/agent/hw/sai/fake/FakeSai.h"

using facebook::fboss::FakeHash;
using facebook::fboss::FakeSai;

sai_status_t set_hash_attribute_fn(
    sai_object_id_t id,
    const sai_attribute_t* attr) {
  if (!attr) {
    return SAI_STATUS_INVALID_PARAMETER;
  }
  auto& hashManager = FakeSai::getInstance()->hashManager;
  auto& hash = hashManager.get(id);
  switch (attr->id) {
    case SAI_HASH_ATTR_NATIVE_HASH_FIELD_LIST: {
      FakeHash::NativeFields nativeHashFields(attr->value.s32list.count);
      for (auto i = 0; i < attr->value.s32list.count; ++i) {
        nativeHashFields[i] = attr->value.s32list.list[i];
      }
      hash.setNativeHashFields(nativeHashFields);
      return SAI_STATUS_SUCCESS;
    }
    case SAI_HASH_ATTR_UDF_GROUP_LIST: {
      FakeHash::UDFGroups udfGroups(attr->value.objlist.count);
      for (auto i = 0; i < attr->value.objlist.count; ++i) {
        udfGroups[i] = attr->value.objlist.list[i];
      }
      hash.setUdfGroups(udfGroups);
      return SAI_STATUS_SUCCESS;
    }
    default:
      return SAI_STATUS_INVALID_PARAMETER;
  }
  return SAI_STATUS_INVALID_PARAMETER;
}

sai_status_t create_hash_fn(
    sai_object_id_t* id,
    sai_object_id_t /*switch_id*/,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  auto fs = FakeSai::getInstance();
  *id = fs->hashManager.create();
  for (int i = 0; i < attr_count; ++i) {
    set_hash_attribute_fn(*id, &attr_list[i]);
  }
  return SAI_STATUS_SUCCESS;
}

sai_status_t remove_hash_fn(sai_object_id_t hash_id) {
  auto fs = FakeSai::getInstance();
  fs->hashManager.remove(hash_id);
  return SAI_STATUS_SUCCESS;
}

sai_status_t get_hash_attribute_fn(
    sai_object_id_t hash_id,
    uint32_t attr_count,
    sai_attribute_t* attr) {
  auto fs = FakeSai::getInstance();
  const auto& hash = fs->hashManager.get(hash_id);
  for (int i = 0; i < attr_count; ++i) {
    switch (attr[i].id) {
      case SAI_HASH_ATTR_NATIVE_HASH_FIELD_LIST: {
        const auto& nativeHashFields = hash.getNativeHashFields();
        if (nativeHashFields.size() > attr[i].value.s32list.count) {
          attr[i].value.s32list.count = nativeHashFields.size();
          return SAI_STATUS_BUFFER_OVERFLOW;
        }
        attr[i].value.s32list.count = nativeHashFields.size();
        int j = 0;
        for (const auto& f : nativeHashFields) {
          attr[i].value.s32list.list[j++] = f;
        }
      } break;
      case SAI_HASH_ATTR_UDF_GROUP_LIST: {
        const auto& udfFields = hash.getUdfGroups();
        if (udfFields.size() > attr[i].value.objlist.count) {
          attr[i].value.objlist.count = udfFields.size();
          return SAI_STATUS_BUFFER_OVERFLOW;
        }
        attr[i].value.objlist.count = udfFields.size();
        int j = 0;
        for (const auto& f : udfFields) {
          attr[i].value.objlist.list[j++] = f;
        }
      } break;
      default:
        return SAI_STATUS_INVALID_PARAMETER;
    }
  }
  return SAI_STATUS_SUCCESS;
}

namespace facebook::fboss {

static sai_hash_api_t _hash_api;

void populate_hash_api(sai_hash_api_t** hash_api) {
  _hash_api.create_hash = &create_hash_fn;
  _hash_api.remove_hash = &remove_hash_fn;
  _hash_api.set_hash_attribute = &set_hash_attribute_fn;
  _hash_api.get_hash_attribute = &get_hash_attribute_fn;
  *hash_api = &_hash_api;
}

} // namespace facebook::fboss
