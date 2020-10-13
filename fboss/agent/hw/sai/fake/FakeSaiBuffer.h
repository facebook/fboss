/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include "fboss/agent/hw/sai/fake/FakeManager.h"

extern "C" {
#include <sai.h>
}

namespace facebook::fboss {

class FakeBufferPool {
 public:
  FakeBufferPool(
      sai_buffer_pool_type_t poolType,
      sai_uint64_t size,
      sai_buffer_pool_threshold_mode_t threshMode)
      : poolType(poolType), size(size), threshMode(threshMode) {}
  sai_object_id_t id;
  sai_buffer_pool_type_t poolType;
  sai_uint64_t size;
  sai_buffer_pool_threshold_mode_t threshMode;
};

using FakeBufferPoolManager = FakeManager<sai_object_id_t, FakeBufferPool>;

class FakeBufferProfile {
 public:
  FakeBufferProfile(
      sai_object_id_t poolId,
      std::optional<sai_uint64_t> reservedBytes,
      std::optional<sai_buffer_profile_threshold_mode_t> threshMode,
      std::optional<sai_int8_t> dynamicThreshold,
      std::optional<sai_int8_t> staticThreshold)
      : poolId(poolId),
        reservedBytes(reservedBytes),
        threshMode(threshMode),
        dynamicThreshold(dynamicThreshold),
        staticThreshold(staticThreshold) {}
  sai_object_id_t id;
  sai_object_id_t poolId;
  std::optional<sai_uint64_t> reservedBytes;
  std::optional<sai_buffer_profile_threshold_mode_t> threshMode;
  std::optional<sai_int8_t> dynamicThreshold;
  std::optional<sai_int8_t> staticThreshold;
};

using FakeBufferProfileManager =
    FakeManager<sai_object_id_t, FakeBufferProfile>;

void populate_buffer_api(sai_buffer_api_t** buffer_api);

} // namespace facebook::fboss
