/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "FakeSaiBridge.h"
#include "FakeSai.h"

#include <folly/logging/xlog.h>
#include <optional>

using facebook::fboss::FakeSai;

sai_status_t set_bridge_attribute_fn(
    sai_object_id_t /* bridge_id */,
    const sai_attribute_t* /* attr */) {
  return SAI_STATUS_FAILURE;
}

sai_status_t create_bridge_fn(
    sai_object_id_t* bridge_id,
    sai_object_id_t /* switch_id */,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  auto fs = FakeSai::getInstance();
  std::optional<sai_bridge_type_t> bridgeType;
  // See if we have a bridge type.
  for (int i = 0; i < attr_count; ++i) {
    if (attr_list[i].id == SAI_BRIDGE_ATTR_TYPE) {
      bridgeType = static_cast<sai_bridge_type_t>(attr_list[i].value.s32);
      break;
    }
  }
  // Create bridge based on the input.
  if (!bridgeType) {
    *bridge_id = fs->bridgeManager.create();
  } else {
    *bridge_id = fs->bridgeManager.create(bridgeType.value());
  }
  for (int i = 0; i < attr_count; ++i) {
    if (attr_list[i].id == SAI_BRIDGE_ATTR_TYPE) {
      continue;
    }
    sai_status_t res = set_bridge_attribute_fn(*bridge_id, &attr_list[i]);
    if (res != SAI_STATUS_SUCCESS) {
      fs->bridgeManager.remove(*bridge_id);
      return res;
    }
  }
  return SAI_STATUS_SUCCESS;
}

sai_status_t remove_bridge_fn(sai_object_id_t bridge_id) {
  auto fs = FakeSai::getInstance();
  fs->bridgeManager.remove(bridge_id);
  return SAI_STATUS_SUCCESS;
}

sai_status_t get_bridge_attribute_fn(
    sai_object_id_t bridge_id,
    uint32_t attr_count,
    sai_attribute_t* attr) {
  auto fs = FakeSai::getInstance();
  const auto& bridge = fs->bridgeManager.get(bridge_id);
  for (int i = 0; i < attr_count; ++i) {
    switch (attr[i].id) {
      case SAI_BRIDGE_ATTR_TYPE:
        attr[i].value.s32 = bridge.type;
        break;
      default:
        return SAI_STATUS_INVALID_PARAMETER;
    }
  }
  return SAI_STATUS_SUCCESS;
}

sai_status_t create_bridge_port_fn(
    sai_object_id_t* bridge_port_id,
    sai_object_id_t /* switch_id */,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  auto fs = FakeSai::getInstance();
  std::optional<sai_object_id_t> portId;
  std::optional<int32_t> type;
  std::optional<int32_t> learningMode;
  std::optional<bool> adminState;
  for (int i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      case SAI_BRIDGE_PORT_ATTR_PORT_ID:
        portId = attr_list[i].value.oid;
        break;
      case SAI_BRIDGE_PORT_ATTR_TYPE:
        type = attr_list[i].value.s32;
        break;
      case SAI_BRIDGE_PORT_ATTR_ADMIN_STATE:
        adminState = attr_list[i].value.booldata;
        break;
      case SAI_BRIDGE_PORT_ATTR_FDB_LEARNING_MODE:
        learningMode = attr_list[i].value.s32;
        break;
      default:
        return SAI_STATUS_INVALID_PARAMETER;
    }
  }
  if (!portId || !type) {
    return SAI_STATUS_INVALID_PARAMETER;
  }
  *bridge_port_id = fs->bridgeManager.createMember(
      0,
      type.value(),
      portId.value(),
      adminState.value(),
      learningMode.value());
  return SAI_STATUS_SUCCESS;
}

sai_status_t remove_bridge_port_fn(sai_object_id_t bridge_port_id) {
  auto fs = FakeSai::getInstance();
  fs->bridgeManager.removeMember(bridge_port_id);
  return SAI_STATUS_SUCCESS;
}

sai_status_t get_bridge_port_attribute_fn(
    sai_object_id_t bridge_port_id,
    uint32_t attr_count,
    sai_attribute_t* attr) {
  auto fs = FakeSai::getInstance();
  auto& bridgePort = fs->bridgeManager.getMember(bridge_port_id);
  for (int i = 0; i < attr_count; ++i) {
    switch (attr[i].id) {
      case SAI_BRIDGE_PORT_ATTR_BRIDGE_ID:
        attr[i].value.oid = 0;
        break;
      case SAI_BRIDGE_PORT_ATTR_PORT_ID:
        attr[i].value.oid = bridgePort.portId;
        break;
      case SAI_BRIDGE_PORT_ATTR_ADMIN_STATE:
        attr[i].value.booldata = bridgePort.adminState;
        break;
      case SAI_BRIDGE_PORT_ATTR_TYPE:
        attr[i].value.s32 = bridgePort.type;
        break;
      case SAI_BRIDGE_PORT_ATTR_FDB_LEARNING_MODE:
        attr[i].value.s32 = bridgePort.learningMode;
        break;
      default:
        return SAI_STATUS_INVALID_PARAMETER;
    }
  }
  return SAI_STATUS_SUCCESS;
}

sai_status_t set_bridge_port_attribute_fn(
    sai_object_id_t bridge_port_id,
    const sai_attribute_t* attr) {
  auto fs = FakeSai::getInstance();
  auto& bridgePort = fs->bridgeManager.getMember(bridge_port_id);
  sai_status_t res;
  if (!attr) {
    return SAI_STATUS_INVALID_PARAMETER;
  }
  switch (attr->id) {
    case SAI_BRIDGE_PORT_ATTR_FDB_LEARNING_MODE:
      bridgePort.learningMode = attr->value.s32;
      res = SAI_STATUS_SUCCESS;
      break;
    case SAI_BRIDGE_PORT_ATTR_ADMIN_STATE:
      bridgePort.adminState = attr->value.booldata;
      res = SAI_STATUS_SUCCESS;
      break;
    default:
      res = SAI_STATUS_INVALID_PARAMETER;
      break;
  }
  return res;
}

namespace facebook::fboss {

static sai_bridge_api_t _bridge_api;

void populate_bridge_api(sai_bridge_api_t** bridge_api) {
  _bridge_api.create_bridge = &create_bridge_fn;
  _bridge_api.remove_bridge = &remove_bridge_fn;
  _bridge_api.set_bridge_attribute = &set_bridge_attribute_fn;
  _bridge_api.get_bridge_attribute = &get_bridge_attribute_fn;
  _bridge_api.create_bridge_port = &create_bridge_port_fn;
  _bridge_api.remove_bridge_port = &remove_bridge_port_fn;
  _bridge_api.set_bridge_port_attribute = &set_bridge_port_attribute_fn;
  _bridge_api.get_bridge_port_attribute = &get_bridge_port_attribute_fn;
  *bridge_api = &_bridge_api;
}

} // namespace facebook::fboss
