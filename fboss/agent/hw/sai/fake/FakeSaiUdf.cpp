/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/sai/fake/FakeSaiUdf.h"
#include "fboss/agent/hw/sai/fake/FakeSai.h"

#include <folly/logging/xlog.h>

using facebook::fboss::FakeSai;

sai_status_t set_udf_attribute_fn(
    sai_object_id_t udf_id,
    const sai_attribute_t* attr) {
  auto fs = FakeSai::getInstance();
  auto& udf = fs->udfManager.get(udf_id);
  sai_status_t res;
  if (!attr) {
    return SAI_STATUS_INVALID_PARAMETER;
  }
  res = SAI_STATUS_SUCCESS;
  switch (attr->id) {
    case SAI_UDF_ATTR_BASE:
      udf.base = attr->value.s32;
      break;
    default:
      res = SAI_STATUS_INVALID_PARAMETER;
      break;
  }
  return res;
}

sai_status_t create_udf_fn(
    sai_object_id_t* udf_id,
    sai_object_id_t /*switch_id */,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  auto fs = FakeSai::getInstance();

  std::optional<sai_object_id_t> udfMatchId;
  std::optional<sai_object_id_t> udfGroupId;
  std::optional<sai_int32_t> base;
  std::optional<sai_uint16_t> offset;

  for (int i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      case SAI_UDF_ATTR_MATCH_ID:
        udfMatchId = attr_list[i].value.oid;
        break;
      case SAI_UDF_ATTR_GROUP_ID:
        udfGroupId = attr_list[i].value.oid;
        break;
      case SAI_UDF_ATTR_BASE:
        base = attr_list[i].value.s32;
        break;
      case SAI_UDF_ATTR_OFFSET:
        offset = attr_list[i].value.u16;
        break;
      default:
        return SAI_STATUS_INVALID_PARAMETER;
        break;
    }
  }

  if (!udfMatchId || !udfGroupId || !offset) {
    return SAI_STATUS_INVALID_PARAMETER;
  }

  *udf_id = fs->udfManager.create(
      udfMatchId.value(), udfGroupId.value(), base.value(), offset.value());

  return SAI_STATUS_SUCCESS;
}

sai_status_t remove_udf_fn(sai_object_id_t udf_id) {
  auto fs = FakeSai::getInstance();
  fs->udfManager.remove(udf_id);
  return SAI_STATUS_SUCCESS;
}

sai_status_t get_udf_attribute_fn(
    sai_object_id_t udf_id,
    uint32_t attr_count,
    sai_attribute_t* attr_list) {
  auto fs = FakeSai::getInstance();
  auto& udf = fs->udfManager.get(udf_id);
  for (int i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      case SAI_UDF_ATTR_MATCH_ID:
        attr_list[i].value.oid = udf.udfMatchId;
        break;
      case SAI_UDF_ATTR_GROUP_ID:
        attr_list[i].value.oid = udf.udfGroupId;
        break;
      case SAI_UDF_ATTR_BASE:
        attr_list[i].value.s32 = udf.base;
        break;
      case SAI_UDF_ATTR_OFFSET:
        attr_list[i].value.u16 = udf.offset;
        break;
      default:
        return SAI_STATUS_NOT_SUPPORTED;
    }
  }
  return SAI_STATUS_SUCCESS;
}

sai_status_t set_udf_match_attribute_fn(
    sai_object_id_t /* udf_match_id */,
    const sai_attribute_t* /* attr */) {
  // All attributes are create-only
  return SAI_STATUS_INVALID_PARAMETER;
}

sai_status_t create_udf_match_fn(
    sai_object_id_t* udf_match_id,
    sai_object_id_t /*switch_id */,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  auto fs = FakeSai::getInstance();

  *udf_match_id = fs->udfMatchManager.create();
  auto& udfMatch = fs->udfMatchManager.get(*udf_match_id);
  for (int i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      case SAI_UDF_MATCH_ATTR_L2_TYPE:
        udfMatch.l2TypeEnable = attr_list[i].value.aclfield.enable;
        udfMatch.l2TypeData = attr_list[i].value.aclfield.data.u16;
        udfMatch.l2TypeMask = attr_list[i].value.aclfield.mask.u16;
        break;
      case SAI_UDF_MATCH_ATTR_L3_TYPE:
        udfMatch.l3TypeEnable = attr_list[i].value.aclfield.enable;
        udfMatch.l3TypeData = attr_list[i].value.aclfield.data.u8;
        udfMatch.l3TypeMask = attr_list[i].value.aclfield.mask.u8;
        break;
#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)
      case SAI_UDF_MATCH_ATTR_L4_DST_PORT_TYPE:
        udfMatch.l4DstPortTypeEnable = attr_list[i].value.aclfield.enable;
        udfMatch.l4DstPortTypeData = attr_list[i].value.aclfield.data.u16;
        udfMatch.l4DstPortTypeMask = attr_list[i].value.aclfield.mask.u16;
        break;
#endif
      default:
        fs->udfMatchManager.remove(*udf_match_id);
        *udf_match_id = SAI_NULL_OBJECT_ID;
        return SAI_STATUS_INVALID_PARAMETER;
        break;
    }
  }
  return SAI_STATUS_SUCCESS;
}

sai_status_t remove_udf_match_fn(sai_object_id_t udf_match_id) {
  auto fs = FakeSai::getInstance();
  fs->udfMatchManager.remove(udf_match_id);
  return SAI_STATUS_SUCCESS;
}

sai_status_t get_udf_match_attribute_fn(
    sai_object_id_t udf_match_id,
    uint32_t attr_count,
    sai_attribute_t* attr_list) {
  auto fs = FakeSai::getInstance();
  auto& udfMatch = fs->udfMatchManager.get(udf_match_id);
  for (int i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      case SAI_UDF_MATCH_ATTR_L2_TYPE:
        attr_list[i].value.aclfield.enable = udfMatch.l2TypeEnable;
        attr_list[i].value.aclfield.data.u16 = udfMatch.l2TypeData;
        attr_list[i].value.aclfield.mask.u16 = udfMatch.l2TypeMask;
        break;
      case SAI_UDF_MATCH_ATTR_L3_TYPE:
        attr_list[i].value.aclfield.enable = udfMatch.l3TypeEnable;
        attr_list[i].value.aclfield.data.u8 = udfMatch.l3TypeData;
        attr_list[i].value.aclfield.mask.u8 = udfMatch.l3TypeMask;
        break;
#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)
      case SAI_UDF_MATCH_ATTR_L4_DST_PORT_TYPE:
        attr_list[i].value.aclfield.enable = udfMatch.l4DstPortTypeEnable;
        attr_list[i].value.aclfield.data.u16 = udfMatch.l4DstPortTypeData;
        attr_list[i].value.aclfield.mask.u16 = udfMatch.l4DstPortTypeMask;
        break;
#endif
      default:
        return SAI_STATUS_NOT_SUPPORTED;
    }
  }
  return SAI_STATUS_SUCCESS;
}

namespace facebook::fboss {

static sai_udf_api_t _udf_api;

void populate_udf_api(sai_udf_api_t** udf_api) {
  _udf_api.create_udf = &create_udf_fn;
  _udf_api.remove_udf = &remove_udf_fn;
  _udf_api.set_udf_attribute = &set_udf_attribute_fn;
  _udf_api.get_udf_attribute = &get_udf_attribute_fn;
  _udf_api.create_udf_match = &create_udf_match_fn;
  _udf_api.remove_udf_match = &remove_udf_match_fn;
  _udf_api.set_udf_match_attribute = &set_udf_match_attribute_fn;
  _udf_api.get_udf_match_attribute = &get_udf_match_attribute_fn;
  *udf_api = &_udf_api;
}

} // namespace facebook::fboss
