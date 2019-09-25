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
  bool isInitialized() const {
    return inited_;
  }
  void setInitStatus(bool inited) {
    inited_ = inited;
  }
  sai_object_id_t id;

 private:
  folly::MacAddress srcMac_;
  bool inited_{false};
};

using FakeSwitchManager = FakeManager<sai_object_id_t, FakeSwitch>;

void populate_switch_api(sai_switch_api_t** switch_api);
} // namespace fboss
} // namespace facebook
