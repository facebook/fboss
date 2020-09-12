/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/sai/fake/FakeSaiWred.h"
#include "fboss/agent/hw/sai/fake/FakeSai.h"

#include <folly/logging/xlog.h>

using facebook::fboss::FakeSai;

sai_status_t set_wred_attribute_fn(
    sai_object_id_t wred_id,
    const sai_attribute_t* attr) {
  auto fs = FakeSai::getInstance();
  auto& wred = fs->wredManager.get(wred_id);
  sai_status_t res;
  if (!attr) {
    return SAI_STATUS_INVALID_PARAMETER;
  }
  res = SAI_STATUS_SUCCESS;
  switch (attr->id) {
    case SAI_WRED_ATTR_GREEN_ENABLE:
      wred.setGreenEnable(attr->value.booldata);
      break;
    case SAI_WRED_ATTR_GREEN_MIN_THRESHOLD:
      wred.setGreenMinThreshold(attr->value.u32);
      break;
    case SAI_WRED_ATTR_GREEN_MAX_THRESHOLD:
      wred.setGreenMaxThreshold(attr->value.u32);
      break;
    case SAI_WRED_ATTR_ECN_MARK_MODE:
      wred.setEcnMarkMode(attr->value.s32);
      break;
    case SAI_WRED_ATTR_ECN_GREEN_MIN_THRESHOLD:
      wred.setEcnGreenMinThreshold(attr->value.u32);
      break;
    case SAI_WRED_ATTR_ECN_GREEN_MAX_THRESHOLD:
      wred.setEcnGreenMaxThreshold(attr->value.u32);
      break;
    default:
      res = SAI_STATUS_INVALID_PARAMETER;
      break;
  }
  return res;
}

sai_status_t create_wred_fn(
    sai_object_id_t* wred_id,
    sai_object_id_t /*switch_id */,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  auto fs = FakeSai::getInstance();

  std::optional<bool> greenEnable;
  std::optional<sai_uint32_t> greenMinThreshold;
  std::optional<sai_uint32_t> greenMaxThreshold;
  std::optional<sai_int32_t> ecnMarkMode;
  std::optional<sai_uint32_t> ecnGreenMinThreshold;
  std::optional<sai_uint32_t> ecnGreenMaxThreshold;

  for (int i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      case SAI_WRED_ATTR_GREEN_ENABLE:
        greenEnable = attr_list[i].value.booldata;
        break;
      case SAI_WRED_ATTR_GREEN_MIN_THRESHOLD:
        greenMinThreshold = attr_list[i].value.u32;
        break;
      case SAI_WRED_ATTR_GREEN_MAX_THRESHOLD:
        greenMaxThreshold = attr_list[i].value.u32;
        break;
      case SAI_WRED_ATTR_ECN_MARK_MODE:
        ecnMarkMode = attr_list[i].value.s32;
        break;
      case SAI_WRED_ATTR_ECN_GREEN_MIN_THRESHOLD:
        ecnGreenMinThreshold = attr_list[i].value.u32;
        break;
      case SAI_WRED_ATTR_ECN_GREEN_MAX_THRESHOLD:
        ecnGreenMaxThreshold = attr_list[i].value.u32;
        break;
      default:
        return SAI_STATUS_INVALID_PARAMETER;
        break;
    }
  }

  if (!greenEnable || !greenMinThreshold || !ecnGreenMaxThreshold ||
      !ecnMarkMode || !ecnGreenMinThreshold || !ecnGreenMaxThreshold) {
    return SAI_STATUS_INVALID_PARAMETER;
  }

  *wred_id = fs->wredManager.create(
      greenEnable.value(),
      greenMinThreshold.value(),
      greenMaxThreshold.value(),
      ecnMarkMode.value(),
      ecnGreenMinThreshold.value(),
      ecnGreenMaxThreshold.value());

  return SAI_STATUS_SUCCESS;
}

sai_status_t remove_wred_fn(sai_object_id_t wred_id) {
  auto fs = FakeSai::getInstance();
  fs->wredManager.remove(wred_id);
  return SAI_STATUS_SUCCESS;
}

sai_status_t get_wred_attribute_fn(
    sai_object_id_t wred_id,
    uint32_t attr_count,
    sai_attribute_t* attr_list) {
  auto fs = FakeSai::getInstance();
  auto& wred = fs->wredManager.get(wred_id);
  for (int i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      case SAI_WRED_ATTR_GREEN_ENABLE:
        attr_list[i].value.booldata = wred.getGreenEnable();
        break;
      case SAI_WRED_ATTR_GREEN_MIN_THRESHOLD:
        attr_list[i].value.u32 = wred.getGreenMinThreshold();
        break;
      case SAI_WRED_ATTR_GREEN_MAX_THRESHOLD:
        attr_list[i].value.u32 = wred.getGreenMaxThreshold();
        break;
      case SAI_WRED_ATTR_ECN_MARK_MODE:
        attr_list[i].value.s32 = wred.getEcnMarkMode();
        break;
      case SAI_WRED_ATTR_ECN_GREEN_MIN_THRESHOLD:
        attr_list[i].value.u32 = wred.getEcnGreenMinThreshold();
        break;
      case SAI_WRED_ATTR_ECN_GREEN_MAX_THRESHOLD:
        attr_list[i].value.u32 = wred.getEcnGreenMaxThreshold();
        break;
      default:
        return SAI_STATUS_NOT_SUPPORTED;
    }
  }
  return SAI_STATUS_SUCCESS;
}

namespace facebook::fboss {

static sai_wred_api_t _wred_api;

void populate_wred_api(sai_wred_api_t** wred_api) {
  _wred_api.create_wred = &create_wred_fn;
  _wred_api.remove_wred = &remove_wred_fn;
  _wred_api.set_wred_attribute = &set_wred_attribute_fn;
  _wred_api.get_wred_attribute = &get_wred_attribute_fn;
  *wred_api = &_wred_api;
}

} // namespace facebook::fboss
