/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "FakeSai.h"
#include "FakeSaiPort.h"

#include <folly/logging/xlog.h>
#include <folly/Optional.h>

using facebook::fboss::FakeSai;
using facebook::fboss::FakePort;

sai_status_t create_port_fn(
    sai_object_id_t* port_id,
    sai_object_id_t /* switch_id */,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  auto fs = FakeSai::getInstance();
  folly::Optional<bool> adminState;
  std::vector<uint32_t> lanes;
  folly::Optional<sai_uint32_t> speed;
  for (int i = 0; i < attr_count; ++i) {
    switch(attr_list[i].id) {
      case SAI_PORT_ATTR_ADMIN_STATE:
        adminState = attr_list[i].value.booldata;
        break;
      case SAI_PORT_ATTR_HW_LANE_LIST:
        {
          for (int j = 0; j < attr_list[i].value.u32list.count; ++j) {
            lanes.push_back(attr_list[i].value.u32list.list[j]);
          }
        }
        break;
      case SAI_PORT_ATTR_SPEED:
        speed = attr_list[i].value.u32;
        break;
      default:
        return SAI_STATUS_INVALID_PARAMETER;
    }
  }
  if (lanes.empty() || !speed) {
    return SAI_STATUS_INVALID_PARAMETER;
  }
  *port_id = fs->pm.create(lanes, speed.value());
  if (adminState) {
    auto& port = fs->pm.get(*port_id);
    port.adminState = adminState.value();
  }
  return SAI_STATUS_SUCCESS;
}

sai_status_t remove_port_fn(sai_object_id_t port_id) {
  auto fs = FakeSai::getInstance();
  fs->pm.remove(port_id);
  return SAI_STATUS_SUCCESS;
}

sai_status_t set_port_attribute_fn(
    sai_object_id_t port_id,
    const sai_attribute_t* attr) {
  auto fs = FakeSai::getInstance();
  auto& port = fs->pm.get(port_id);
  sai_status_t res;
  if (!attr) {
    return SAI_STATUS_INVALID_PARAMETER;
  }
  switch (attr->id) {
    case SAI_PORT_ATTR_ADMIN_STATE:
      port.adminState = attr->value.booldata;
      res = SAI_STATUS_SUCCESS;
      break;
    case SAI_PORT_ATTR_HW_LANE_LIST:
      {
        auto& lanes = port.lanes;
        lanes.clear();
        for (int j = 0; j < attr->value.u32list.count; ++j) {
          lanes.push_back(attr->value.u32list.list[j]);
        }
      }
      res = SAI_STATUS_SUCCESS;
      break;
    case SAI_PORT_ATTR_SPEED:
      port.speed = attr->value.u32;
      res = SAI_STATUS_SUCCESS;
      break;
    default:
      res = SAI_STATUS_INVALID_PARAMETER;
      break;
  }
  return res;
}

sai_status_t get_port_attribute_fn(
    sai_object_id_t port_id,
    uint32_t attr_count,
    sai_attribute_t* attr) {
  auto fs = FakeSai::getInstance();
  auto port = fs->pm.get(port_id);
  for (int i = 0; i < attr_count; ++i) {
    switch (attr[i].id) {
      case SAI_PORT_ATTR_ADMIN_STATE:
        attr[i].value.booldata = port.adminState;
        break;
      case SAI_PORT_ATTR_HW_LANE_LIST:
        for (int j = 0; j < port.lanes.size(); ++j) {
          attr[i].value.u32list.list[j] = port.lanes[j];
        }
        attr[i].value.u32list.count = port.lanes.size();
        break;
      case SAI_PORT_ATTR_SPEED:
        attr[i].value.u32 = port.speed;
        break;
      default:
        return SAI_STATUS_INVALID_PARAMETER;
    }
  }
  return SAI_STATUS_SUCCESS;
}

namespace facebook {
namespace fboss {

static sai_port_api_t _port_api;

void populate_port_api(sai_port_api_t** port_api) {
  _port_api.create_port = &create_port_fn;
  _port_api.remove_port = &remove_port_fn;
  _port_api.set_port_attribute = &set_port_attribute_fn;
  _port_api.get_port_attribute = &get_port_attribute_fn;
  *port_api = &_port_api;
}

} // namespace fboss
} // namespace facebook
