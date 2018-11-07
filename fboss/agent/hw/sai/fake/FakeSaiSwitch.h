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

#include "FakeManager.h"

#include <folly/MacAddress.h>

extern "C" {
  #include <sai.h>
}

sai_status_t set_switch_attribute_fn(
    sai_object_id_t switch_id,
    const sai_attribute_t* attr);

sai_status_t get_switch_attribute_fn(
    sai_object_id_t switch_id,
    uint32_t attr_count,
    sai_attribute_t* attr);

namespace facebook {
namespace fboss {

class FakeSwitch {
 public:
  void setSrcMac(const sai_mac_t& mac) {
    folly::ByteRange r(std::begin(mac), std::end(mac));
    srcMac_ = folly::MacAddress::fromBinary(r);
  }
  folly::MacAddress srcMac() const {
    return srcMac_;
  }
  sai_object_id_t id;
 private:
  folly::MacAddress srcMac_;
};

using FakeSwitchManager = FakeManager<sai_object_id_t, FakeSwitch>;

void populate_switch_api(sai_switch_api_t** switch_api);
}
} // namespace facebook
