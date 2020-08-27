/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/sai/fake/FakeSaiHostif.h"
#include "fboss/agent/hw/sai/api/SaiVersion.h"
#include "fboss/agent/hw/sai/fake/FakeSai.h"

#include <folly/logging/xlog.h>
#include <optional>

using facebook::fboss::FakeHostifTrapGroupManager;
using facebook::fboss::FakeHostifTrapManager;
using facebook::fboss::FakeSai;

sai_status_t send_hostif_fn(
    sai_object_id_t /* switch_id */,
    sai_size_t /* buffer_size */,
    const void* /* buffer */,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  sai_object_id_t tx_port;
  sai_hostif_tx_type_t tx_type;
  sai_uint8_t queueId = 0;
  for (int i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      case SAI_HOSTIF_PACKET_ATTR_HOSTIF_TX_TYPE:
        tx_type = static_cast<sai_hostif_tx_type_t>(attr_list[i].value.s32);
        break;
      case SAI_HOSTIF_PACKET_ATTR_EGRESS_PORT_OR_LAG:
        tx_port = attr_list[i].value.oid;
        break;
      case SAI_HOSTIF_PACKET_ATTR_EGRESS_QUEUE_INDEX:
        queueId = attr_list[i].value.u8;
        break;
    }
  }
  XLOG(INFO) << "Sending packet on port : " << std::hex << tx_port
             << " tx type : " << tx_type << " on queue: " << queueId;

  return SAI_STATUS_SUCCESS;
}

sai_status_t create_hostif_trap_fn(
    sai_object_id_t* hostif_trap_id,
    sai_object_id_t /* switch_id */,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  auto fs = FakeSai::getInstance();
  std::optional<sai_hostif_trap_type_t> trapType;
  std::optional<sai_packet_action_t> packetAction;
  sai_object_id_t trapGroup = 0;
  uint32_t priority = 0;
  for (int i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      case SAI_HOSTIF_TRAP_ATTR_TRAP_TYPE:
        trapType = (sai_hostif_trap_type_t)attr_list[i].value.s32;
        break;
      case SAI_HOSTIF_TRAP_ATTR_PACKET_ACTION:
        packetAction = (sai_packet_action_t)attr_list[i].value.s32;
        break;
      case SAI_HOSTIF_TRAP_ATTR_TRAP_PRIORITY:
        priority = attr_list[i].value.u32;
        break;
      case SAI_HOSTIF_TRAP_ATTR_TRAP_GROUP:
        trapGroup = attr_list[i].value.oid;
        break;
      default:
        break;
    }
  }
  if (!trapType) {
    XLOG(ERR) << "create hostif trap missing trap type";
    return SAI_STATUS_INVALID_PARAMETER;
  }
  if (!packetAction) {
    XLOG(ERR) << "create hostif trap missing packet action";
    return SAI_STATUS_INVALID_PARAMETER;
  }
  *hostif_trap_id = fs->hostIfTrapManager.create(
      trapType.value(), packetAction.value(), priority, trapGroup);
  return SAI_STATUS_SUCCESS;
}

sai_status_t remove_hostif_trap_fn(sai_object_id_t hostif_trap_id) {
  auto fs = FakeSai::getInstance();
  fs->hostIfTrapManager.remove(hostif_trap_id);
  return SAI_STATUS_SUCCESS;
}

sai_status_t set_hostif_trap_attribute_fn(
    sai_object_id_t hostif_trap_id,
    const sai_attribute_t* attr) {
  auto fs = FakeSai::getInstance();
  auto& trap = fs->hostIfTrapManager.get(hostif_trap_id);
  switch (attr->id) {
    case SAI_HOSTIF_TRAP_ATTR_PACKET_ACTION:
      trap.packetAction = attr->value.s32;
      break;
    case SAI_HOSTIF_TRAP_ATTR_TRAP_PRIORITY:
      trap.priority = attr->value.u32;
      break;
    case SAI_HOSTIF_TRAP_ATTR_TRAP_GROUP:
      trap.trapGroup = attr->value.oid;
      break;
    default:
      return SAI_STATUS_INVALID_PARAMETER;
  }
  return SAI_STATUS_SUCCESS;
}

sai_status_t get_hostif_trap_attribute_fn(
    sai_object_id_t hostif_trap_id,
    uint32_t attr_count,
    sai_attribute_t* attr) {
  auto fs = FakeSai::getInstance();
  const auto& hostifTrap = fs->hostIfTrapManager.get(hostif_trap_id);
  for (int i = 0; i < attr_count; ++i) {
    switch (attr[i].id) {
      case SAI_HOSTIF_TRAP_ATTR_TRAP_TYPE:
        attr[i].value.s32 = static_cast<int32_t>(hostifTrap.trapType);
        break;
      case SAI_HOSTIF_TRAP_ATTR_PACKET_ACTION:
        attr[i].value.s32 = static_cast<int32_t>(hostifTrap.packetAction);
        break;
      case SAI_HOSTIF_TRAP_ATTR_TRAP_PRIORITY:
        attr[i].value.u32 = hostifTrap.priority;
        break;
      case SAI_HOSTIF_TRAP_ATTR_TRAP_GROUP:
        attr[i].value.oid = hostifTrap.trapGroup;
        break;
      default:
        return SAI_STATUS_INVALID_PARAMETER;
    }
  }
  return SAI_STATUS_SUCCESS;
}

sai_status_t create_hostif_trap_group_fn(
    sai_object_id_t* hostif_trap_group_id,
    sai_object_id_t /* switch_id */,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  auto fs = FakeSai::getInstance();
  uint32_t queueId = 0;
  sai_object_id_t policer = 0;
  for (int i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      case SAI_HOSTIF_TRAP_GROUP_ATTR_QUEUE:
        queueId = attr_list[i].value.u32;
        break;
      case SAI_HOSTIF_TRAP_GROUP_ATTR_POLICER:
        policer = attr_list[i].value.oid;
        break;
      default:
        break;
    }
  }
  *hostif_trap_group_id = fs->hostifTrapGroupManager.create(queueId, policer);
  return SAI_STATUS_SUCCESS;
}

sai_status_t remove_hostif_trap_group_fn(sai_object_id_t hostif_trap_group_id) {
  auto fs = FakeSai::getInstance();
  fs->hostifTrapGroupManager.remove(hostif_trap_group_id);
  return SAI_STATUS_SUCCESS;
}

sai_status_t set_hostif_trap_group_attribute_fn(
    sai_object_id_t /* hostif_trap_group_id */,
    const sai_attribute_t* attr) {
  switch (attr->id) {
    default:
      return SAI_STATUS_INVALID_PARAMETER;
  }
  return SAI_STATUS_SUCCESS;
}

sai_status_t get_hostif_trap_group_attribute_fn(
    sai_object_id_t hostif_trap_group_id,
    uint32_t attr_count,
    sai_attribute_t* attr) {
  auto fs = FakeSai::getInstance();
  const auto& hostifTrapGroup =
      fs->hostifTrapGroupManager.get(hostif_trap_group_id);
  for (int i = 0; i < attr_count; ++i) {
    switch (attr[i].id) {
      case SAI_HOSTIF_TRAP_GROUP_ATTR_QUEUE:
        attr[i].value.s32 = static_cast<uint32_t>(hostifTrapGroup.queueId);
        break;
      case SAI_HOSTIF_TRAP_GROUP_ATTR_POLICER:
        attr[i].value.oid = hostifTrapGroup.policerId;
        break;
      default:
        return SAI_STATUS_INVALID_PARAMETER;
    }
  }
  return SAI_STATUS_SUCCESS;
}

namespace facebook::fboss {

static sai_hostif_api_t _hostif_api;

void populate_hostif_api(sai_hostif_api_t** hostif_api) {
  _hostif_api.create_hostif_trap_group = &create_hostif_trap_group_fn;
  _hostif_api.remove_hostif_trap_group = &remove_hostif_trap_group_fn;
  _hostif_api.set_hostif_trap_group_attribute =
      &set_hostif_trap_group_attribute_fn;
  _hostif_api.get_hostif_trap_group_attribute =
      &get_hostif_trap_group_attribute_fn;
  _hostif_api.create_hostif_trap = &create_hostif_trap_fn;
  _hostif_api.remove_hostif_trap = &remove_hostif_trap_fn;
  _hostif_api.set_hostif_trap_attribute = &set_hostif_trap_attribute_fn;
  _hostif_api.get_hostif_trap_attribute = &get_hostif_trap_attribute_fn;
  _hostif_api.send_hostif_packet = &send_hostif_fn;
  *hostif_api = &_hostif_api;
}

} // namespace facebook::fboss
