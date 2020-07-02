/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/sai/fake/FakeSaiBuffer.h"
#include "fboss/agent/hw/sai/fake/FakeSai.h"

#include <folly/logging/xlog.h>
#include <optional>

using facebook::fboss::FakeQueue;
using facebook::fboss::FakeSai;

sai_status_t create_buffer_pool_fn(
    sai_object_id_t* buffer_pool_id,
    sai_object_id_t /* switch_id */,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  auto fs = FakeSai::getInstance();
  std::optional<sai_buffer_pool_type_t> poolType;
  std::optional<sai_uint64_t> poolSize;
  std::optional<sai_buffer_pool_threshold_mode_t> threshMode;
  for (int i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      case SAI_BUFFER_POOL_ATTR_TYPE:
        poolType = static_cast<sai_buffer_pool_type_t>(attr_list[i].value.s32);
        break;
      case SAI_BUFFER_POOL_ATTR_SIZE:
        poolSize = attr_list[i].value.u64;
        break;
      case SAI_BUFFER_POOL_ATTR_THRESHOLD_MODE:
        threshMode = static_cast<sai_buffer_pool_threshold_mode_t>(
            attr_list[i].value.s32);
        break;
      default:
        return SAI_STATUS_INVALID_PARAMETER;
    }
  }
  if (!poolType || !poolSize || !threshMode) {
    return SAI_STATUS_INVALID_PARAMETER;
  }
  *buffer_pool_id = fs->bufferPoolManager.create(
      poolType.value(), poolSize.value(), threshMode.value());
  return SAI_STATUS_SUCCESS;
}

sai_status_t remove_buffer_pool_fn(sai_object_id_t pool_id) {
  auto fs = FakeSai::getInstance();
  fs->bufferPoolManager.remove(pool_id);
  return SAI_STATUS_SUCCESS;
}

sai_status_t set_buffer_pool_attribute_fn(
    sai_object_id_t /*buffer_pool_id*/,
    const sai_attribute_t* /*attr*/) {
  return SAI_STATUS_NOT_SUPPORTED;
}

sai_status_t get_buffer_pool_attribute_fn(
    sai_object_id_t pool_id,
    uint32_t attr_count,
    sai_attribute_t* attr) {
  auto fs = FakeSai::getInstance();
  auto pool = fs->bufferPoolManager.get(pool_id);
  for (int i = 0; i < attr_count; ++i) {
    switch (attr[i].id) {
      case SAI_BUFFER_POOL_ATTR_TYPE:
        attr[i].value.s32 = pool.poolType;
        break;
      case SAI_BUFFER_POOL_ATTR_SIZE:
        attr[i].value.u64 = pool.size;
        break;
      case SAI_BUFFER_POOL_ATTR_THRESHOLD_MODE:
        attr[i].value.s32 = pool.threshMode;
        break;
      default:
        return SAI_STATUS_INVALID_PARAMETER;
    }
  }
  return SAI_STATUS_SUCCESS;
}

namespace facebook::fboss {

static sai_buffer_api_t _buffer_api;

void populate_buffer_api(sai_buffer_api_t** buffer_api) {
  _buffer_api.create_buffer_pool = &create_buffer_pool_fn;
  _buffer_api.remove_buffer_pool = &remove_buffer_pool_fn;
  _buffer_api.set_buffer_pool_attribute = &set_buffer_pool_attribute_fn;
  _buffer_api.get_buffer_pool_attribute = &get_buffer_pool_attribute_fn;
  *buffer_api = &_buffer_api;
}

} // namespace facebook::fboss
