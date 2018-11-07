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

sai_status_t create_router_interface_fn(
    sai_object_id_t* router_interface_id,
    sai_object_id_t switch_id,
    uint32_t attr_count,
    const sai_attribute_t* attr_list);

sai_status_t remove_router_interface_fn(sai_object_id_t router_interface_id);

sai_status_t set_router_interface_attribute_fn(
    sai_object_id_t router_interface_id,
    const sai_attribute_t* attr);

sai_status_t get_router_interface_attribute_fn(
    sai_object_id_t router_interface_id,
    uint32_t attr_count,
    sai_attribute_t* attr);

namespace facebook {
namespace fboss {

class FakeRouterInterface {
 public:
  FakeRouterInterface(
      int32_t type,
      const sai_object_id_t& virtualRouterId,
      const sai_object_id_t vlanId)
      : type(type), virtualRouterId(virtualRouterId), vlanId(vlanId) {}
  void setSrcMac(const sai_mac_t& mac) {
    folly::ByteRange r(std::begin(mac), std::end(mac));
    srcMac_ = folly::MacAddress::fromBinary(r);
  }
  folly::MacAddress srcMac() const {
    return srcMac_;
  }
  int32_t type;
  sai_object_id_t virtualRouterId;
  sai_object_id_t vlanId;
  sai_object_id_t id;
 private:
  folly::MacAddress srcMac_;
};
using FakeRouterInterfaceManager =
    FakeManager<sai_object_id_t, FakeRouterInterface>;

void populate_router_interface_api(
    sai_router_interface_api_t** router_interface_api);
} // namespace fboss
} // namespace facebook
