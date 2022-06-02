/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/sai/fake/FakeSaiSystemPort.h"
#include "fboss/agent/hw/sai/fake/FakeSai.h"

#include "fboss/agent/hw/sai/api/AddressUtil.h"

#include <folly/logging/xlog.h>
#include <optional>

using facebook::fboss::FakeSai;
using facebook::fboss::FakeSystemPort;

sai_status_t create_system_port_fn(
    sai_object_id_t* id,
    sai_object_id_t switch_id,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  auto fs = FakeSai::getInstance();
  return SAI_STATUS_SUCCESS;
}

sai_status_t remove_system_port_fn(sai_object_id_t system_port_entry) {
  auto fs = FakeSai::getInstance();
  return SAI_STATUS_SUCCESS;
}

sai_status_t set_system_port_attribute_fn(
    sai_object_id_t system_port_entry,
    const sai_attribute_t* attr) {
  auto fs = FakeSai::getInstance();
  switch (attr->id) {
    default:
      return SAI_STATUS_INVALID_PARAMETER;
  }
  return SAI_STATUS_SUCCESS;
}

sai_status_t get_system_port_attribute_fn(
    sai_object_id_t system_port_entry,
    uint32_t attr_count,
    sai_attribute_t* attr_list) {
  auto fs = FakeSai::getInstance();
  for (int i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      default:
        return SAI_STATUS_INVALID_PARAMETER;
    }
  }
  return SAI_STATUS_SUCCESS;
}

namespace facebook::fboss {
static sai_system_port_api_t _system_port_api;

void populate_system_port_api(sai_system_port_api_t** system_port_api) {
  _system_port_api.create_system_port = &create_system_port_fn;
  _system_port_api.remove_system_port = &remove_system_port_fn;
  _system_port_api.set_system_port_attribute = &set_system_port_attribute_fn;
  _system_port_api.get_system_port_attribute = &get_system_port_attribute_fn;
  *system_port_api = &_system_port_api;
  // TODO
}
} // namespace facebook::fboss
