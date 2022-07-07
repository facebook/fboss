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
  sai_system_port_config_t config;
  bool adminState;
  sai_object_id_t qosToTcMapId;
  for (int i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      case SAI_SYSTEM_PORT_ATTR_CONFIG_INFO:
        config = attr_list[i].value.sysportconfig;
        break;
      case SAI_SYSTEM_PORT_ATTR_ADMIN_STATE:
        adminState = attr_list[i].value.booldata;
        break;
      case SAI_SYSTEM_PORT_ATTR_QOS_TC_TO_QUEUE_MAP:
        qosToTcMapId = attr_list[i].value.oid;
        break;
    }
  }
  *id =
      fs->systemPortManager.create(config, switch_id, adminState, qosToTcMapId);
  return SAI_STATUS_SUCCESS;
}

sai_status_t remove_system_port_fn(sai_object_id_t system_port_id) {
  auto fs = FakeSai::getInstance();
  fs->systemPortManager.remove(system_port_id);
  return SAI_STATUS_SUCCESS;
}

sai_status_t set_system_port_attribute_fn(
    sai_object_id_t system_port_id,
    const sai_attribute_t* attr) {
  auto fs = FakeSai::getInstance();
  auto& systemPort = fs->systemPortManager.get(system_port_id);
  if (!attr) {
    return SAI_STATUS_INVALID_PARAMETER;
  }
  switch (attr->id) {
    case SAI_SYSTEM_PORT_ATTR_CONFIG_INFO:
      systemPort.config = attr->value.sysportconfig;
      break;
    case SAI_SYSTEM_PORT_ATTR_ADMIN_STATE:
      systemPort.adminState = attr->value.booldata;
      break;
    case SAI_SYSTEM_PORT_ATTR_QOS_TC_TO_QUEUE_MAP:
      systemPort.qosToTcMapId = attr->value.oid;
      break;
    default:
      return SAI_STATUS_INVALID_PARAMETER;
  }
  return SAI_STATUS_SUCCESS;
}

sai_status_t get_system_port_attribute_fn(
    sai_object_id_t system_port_id,
    uint32_t attr_count,
    sai_attribute_t* attr_list) {
  auto fs = FakeSai::getInstance();
  auto& systemPort = fs->systemPortManager.get(system_port_id);
  static const auto voqs = 8;
  for (int i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      case SAI_SYSTEM_PORT_ATTR_TYPE:
        attr_list[i].value.u32 = systemPort.switchId == 0
            ? SAI_SYSTEM_PORT_TYPE_LOCAL
            : SAI_SYSTEM_PORT_TYPE_REMOTE;
        break;
      case SAI_SYSTEM_PORT_ATTR_QOS_NUMBER_OF_VOQS:
        attr_list[i].value.u32 = voqs;
        break;
      case SAI_SYSTEM_PORT_ATTR_QOS_VOQ_LIST: {
        if (attr_list[i].value.objlist.count < voqs) {
          attr_list[i].value.objlist.count = voqs;
          return SAI_STATUS_BUFFER_OVERFLOW;
        }
        sai_object_list_t objlist;
        for (auto v = 0; v < voqs; ++v) {
          objlist.list[v] = v + 1;
        }
        attr_list[i].value.objlist = objlist;
      } break;
      case SAI_SYSTEM_PORT_ATTR_PORT:
        attr_list[i].value.oid = systemPort.config.port_id;
        break;
      case SAI_SYSTEM_PORT_ATTR_CONFIG_INFO:
        attr_list[i].value.sysportconfig = systemPort.config;
        break;
      case SAI_SYSTEM_PORT_ATTR_ADMIN_STATE:
        attr_list[i].value.booldata = systemPort.adminState;
        break;
      case SAI_SYSTEM_PORT_ATTR_QOS_TC_TO_QUEUE_MAP:
        attr_list[i].value.oid = systemPort.qosToTcMapId;
        break;
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
