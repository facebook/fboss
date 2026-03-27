/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/api/SaiAttribute.h"

#include <array>
#include <cstring>
#include <vector>

void _fill(std::vector<std::array<uint8_t, 16>>& src, sai_segment_list_t& dst) {
  dst.count = static_cast<uint32_t>(src.size());
  dst.list = src.empty() ? nullptr : reinterpret_cast<sai_ip6_t*>(src.data());
}

void _fill(
    const sai_segment_list_t& src,
    std::vector<std::array<uint8_t, 16>>& dst) {
  dst.resize(src.count);
  for (uint32_t i = 0; i < src.count; i++) {
    // NOLINTNEXTLINE(facebook-hte-ParameterUncheckedArrayBounds)
    std::memcpy(dst[i].data(), &src.list[i], 16);
  }
}

void _realloc(
    const sai_segment_list_t& src,
    std::vector<std::array<uint8_t, 16>>& dst) {
  dst.resize(src.count);
}
