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

#include <optional>

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
      sai_buffer_pool_threshold_mode_t threshMode,
      std::optional<sai_uint64_t> xoffSize)
      : poolType(poolType),
        size(size),
        threshMode(threshMode),
        xoffSize(xoffSize) {}
  sai_object_id_t id;
  sai_buffer_pool_type_t poolType;
  sai_uint64_t size;
  sai_buffer_pool_threshold_mode_t threshMode;
  std::optional<sai_uint64_t> xoffSize;
};

using FakeBufferPoolManager = FakeManager<sai_object_id_t, FakeBufferPool>;

class FakeBufferProfile {
 public:
  FakeBufferProfile(
      sai_object_id_t poolId,
      std::optional<sai_uint64_t> reservedBytes,
      std::optional<sai_buffer_profile_threshold_mode_t> threshMode,
      std::optional<sai_int8_t> dynamicThreshold,
      std::optional<sai_uint64_t> staticThreshold,
      std::optional<sai_uint64_t> xoffTh,
      std::optional<sai_uint64_t> xonTh,
      std::optional<sai_uint64_t> xonOffsetTh)
      : poolId(poolId),
        reservedBytes(reservedBytes),
        threshMode(threshMode),
        dynamicThreshold(dynamicThreshold),
        staticThreshold(staticThreshold),
        xoffTh(xoffTh),
        xonTh(xonTh),
        xonOffsetTh(xonOffsetTh) {}
  sai_object_id_t id;
  sai_object_id_t poolId;
  std::optional<sai_uint64_t> reservedBytes;
  std::optional<sai_buffer_profile_threshold_mode_t> threshMode;
  std::optional<sai_int8_t> dynamicThreshold;
  std::optional<sai_uint64_t> staticThreshold;
  std::optional<sai_uint64_t> xoffTh;
  std::optional<sai_uint64_t> xonTh;
  std::optional<sai_uint64_t> xonOffsetTh;
};

using FakeBufferProfileManager =
    FakeManager<sai_object_id_t, FakeBufferProfile>;

void populate_buffer_api(sai_buffer_api_t** buffer_api);

class FakeIngressPriorityGroup {
 public:
  FakeIngressPriorityGroup(
      sai_object_id_t port,
      sai_uint8_t index,
      std::optional<sai_object_id_t> bufferProfile)
      : port(port), index(index), bufferProfile(bufferProfile) {}
  sai_object_id_t id;
  sai_object_id_t port;
  sai_uint8_t index;
  std::optional<sai_object_id_t> bufferProfile;
};

using FakeIngressPriorityGroupManager =
    FakeManager<sai_object_id_t, FakeIngressPriorityGroup>;

} // namespace facebook::fboss
