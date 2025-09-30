/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/sai/fake/FakeSai.h"

#include <optional>

using facebook::fboss::FakeSai;

sai_status_t create_ars_fn(
    sai_object_id_t* ars_id,
    sai_object_id_t /* switch_id */,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  auto fs = FakeSai::getInstance();
  std::optional<sai_ars_mode_t> mode = SAI_ARS_MODE_FLOWLET_QUALITY;
  std::optional<sai_uint32_t> idle_time = 256;
  std::optional<sai_uint32_t> max_flows = 512;
  std::optional<sai_uint32_t> primary_path_quality_threshold;
  std::optional<sai_uint32_t> alternate_path_cost;
  std::optional<sai_uint32_t> alternate_path_bias;
  for (int i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      case SAI_ARS_ATTR_MODE:
        mode = static_cast<sai_ars_mode_t>(attr_list[i].value.s32);
        break;
      case SAI_ARS_ATTR_IDLE_TIME:
        idle_time = attr_list[i].value.u32;
        break;
      case SAI_ARS_ATTR_MAX_FLOWS:
        max_flows = attr_list[i].value.u32;
        break;
      case SAI_ARS_ATTR_PRIMARY_PATH_QUALITY_THRESHOLD:
        primary_path_quality_threshold = attr_list[i].value.u32;
        break;
      case SAI_ARS_ATTR_ALTERNATE_PATH_COST:
        alternate_path_cost = attr_list[i].value.u32;
        break;
      case SAI_ARS_ATTR_ALTERNATE_PATH_BIAS:
        alternate_path_bias = attr_list[i].value.u32;
        break;
      default:
        return SAI_STATUS_INVALID_PARAMETER;
    }
  }
  *ars_id = fs->arsManager.create(
      mode.value(),
      idle_time.value(),
      max_flows.value(),
      primary_path_quality_threshold,
      alternate_path_cost,
      alternate_path_bias);

  return SAI_STATUS_SUCCESS;
}

sai_status_t remove_ars_fn(sai_object_id_t ars_id) {
  auto fs = FakeSai::getInstance();
  fs->arsManager.remove(ars_id);
  return SAI_STATUS_SUCCESS;
}

sai_status_t set_ars_attribute_fn(
    sai_object_id_t ars_id,
    const sai_attribute_t* attr) {
  auto fs = FakeSai::getInstance();
  auto& ars = fs->arsManager.get(ars_id);
  switch (attr->id) {
    case SAI_ARS_ATTR_MODE:
      ars.mode = static_cast<sai_ars_mode_t>(attr->value.s32);
      break;
    case SAI_ARS_ATTR_IDLE_TIME:
      ars.idle_time = attr->value.u32;
      break;
    case SAI_ARS_ATTR_MAX_FLOWS:
      ars.max_flows = attr->value.u32;
      break;
    case SAI_ARS_ATTR_PRIMARY_PATH_QUALITY_THRESHOLD:
      ars.primary_path_quality_threshold = attr->value.u32;
      break;
    case SAI_ARS_ATTR_ALTERNATE_PATH_COST:
      ars.alternate_path_cost = attr->value.u32;
      break;
    case SAI_ARS_ATTR_ALTERNATE_PATH_BIAS:
      ars.alternate_path_bias = attr->value.u32;
      break;
    default:
      return SAI_STATUS_INVALID_PARAMETER;
  }
  return SAI_STATUS_SUCCESS;
}

sai_status_t get_ars_attribute_fn(
    sai_object_id_t ars_id,
    uint32_t attr_count,
    sai_attribute_t* attr_list) {
  auto fs = FakeSai::getInstance();
  const auto& ars = fs->arsManager.get(ars_id);
  for (int i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      case SAI_ARS_ATTR_MODE:
        attr_list[i].value.s32 = static_cast<sai_int32_t>(ars.mode);
        break;
      case SAI_ARS_ATTR_IDLE_TIME:
        attr_list[i].value.u32 = ars.idle_time;
        break;
      case SAI_ARS_ATTR_MAX_FLOWS:
        attr_list[i].value.u32 = ars.max_flows;
        break;
      case SAI_ARS_ATTR_PRIMARY_PATH_QUALITY_THRESHOLD:
        if (ars.primary_path_quality_threshold) {
          attr_list[i].value.u32 = ars.primary_path_quality_threshold.value();
        } else {
          attr_list[i].value.u32 =
              0; // Default value for unset optional attribute
        }
        break;
      case SAI_ARS_ATTR_ALTERNATE_PATH_COST:
        if (ars.alternate_path_cost) {
          attr_list[i].value.u32 = ars.alternate_path_cost.value();
        } else {
          attr_list[i].value.u32 =
              0; // Default value for unset optional attribute
        }
        break;
      case SAI_ARS_ATTR_ALTERNATE_PATH_BIAS:
        if (ars.alternate_path_bias) {
          attr_list[i].value.u32 = ars.alternate_path_bias.value();
        } else {
          attr_list[i].value.u32 =
              0; // Default value for unset optional attribute
        }
        break;
      default:
        return SAI_STATUS_INVALID_PARAMETER;
    }
  }
  return SAI_STATUS_SUCCESS;
}

namespace facebook::fboss {

static sai_ars_api_t _ars_api;

void populate_ars_api(sai_ars_api_t** ars_api) {
  _ars_api.create_ars = &create_ars_fn;
  _ars_api.remove_ars = &remove_ars_fn;
  _ars_api.set_ars_attribute = &set_ars_attribute_fn;
  _ars_api.get_ars_attribute = &get_ars_attribute_fn;
  *ars_api = &_ars_api;
}

} // namespace facebook::fboss
