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

#include "fboss/agent/hw/sai/api/AddressUtil.h"

#include <array>
#include <cstdint>
#include <vector>

void _fill(std::vector<folly::IPAddressV6>& src, sai_segment_list_t& dst) {
  static thread_local std::vector<std::array<uint8_t, 16>> buf;
  buf.resize(src.size());
  for (size_t i = 0; i < src.size(); i++) {
    facebook::fboss::toSaiIpAddressV6(
        src[i], reinterpret_cast<sai_ip6_t*>(buf[i].data()));
  }
  dst.count = static_cast<uint32_t>(src.size());
  dst.list = reinterpret_cast<sai_ip6_t*>(buf.data());
}

void _fill(
    const sai_segment_list_t& src,
    std::vector<folly::IPAddressV6>& dst) {
  dst.resize(src.count);
  for (uint32_t i = 0; i < src.count; i++) {
    // NOLINTNEXTLINE(facebook-hte-ParameterUncheckedArrayBounds)
    dst[i] = facebook::fboss::fromSaiIpAddress(src.list[i]);
  }
}

void _realloc(
    const sai_segment_list_t& src,
    std::vector<folly::IPAddressV6>& dst) {
  dst.resize(src.count);
}
