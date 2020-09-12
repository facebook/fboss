/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/tracer/HostifApiTracer.h"
#include "fboss/agent/hw/sai/api/SaiVersion.h"
#include "fboss/agent/hw/sai/tracer/Utils.h"

namespace facebook::fboss {

sai_status_t wrap_create_hostif_trap(
    sai_object_id_t* hostif_trap_id,
    sai_object_id_t switch_id,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  auto rv = SaiTracer::getInstance()->hostifApi_->create_hostif_trap(
      hostif_trap_id, switch_id, attr_count, attr_list);

  SaiTracer::getInstance()->logCreateFn(
      "create_hostif_trap",
      hostif_trap_id,
      switch_id,
      attr_count,
      attr_list,
      SAI_OBJECT_TYPE_HOSTIF_TRAP,
      rv);
  return rv;
}

sai_status_t wrap_remove_hostif_trap(sai_object_id_t hostif_trap_id) {
  auto rv =
      SaiTracer::getInstance()->hostifApi_->remove_hostif_trap(hostif_trap_id);

  SaiTracer::getInstance()->logRemoveFn(
      "remove_hostif_trap", hostif_trap_id, SAI_OBJECT_TYPE_HOSTIF_TRAP, rv);
  return rv;
}

sai_status_t wrap_set_hostif_trap_attribute(
    sai_object_id_t hostif_trap_id,
    const sai_attribute_t* attr) {
  auto rv = SaiTracer::getInstance()->hostifApi_->set_hostif_trap_attribute(
      hostif_trap_id, attr);

  SaiTracer::getInstance()->logSetAttrFn(
      "set_hostif_trap_attribute",
      hostif_trap_id,
      attr,
      SAI_OBJECT_TYPE_HOSTIF_TRAP,
      rv);
  return rv;
}

sai_status_t wrap_get_hostif_trap_attribute(
    sai_object_id_t hostif_trap_id,
    uint32_t attr_count,
    sai_attribute_t* attr_list) {
  // TODO(zecheng): Log get functions as well
  return SaiTracer::getInstance()->hostifApi_->get_hostif_trap_attribute(
      hostif_trap_id, attr_count, attr_list);
}

sai_status_t wrap_create_hostif_trap_group(
    sai_object_id_t* hostif_trap_group_id,
    sai_object_id_t switch_id,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  auto rv = SaiTracer::getInstance()->hostifApi_->create_hostif_trap_group(
      hostif_trap_group_id, switch_id, attr_count, attr_list);

  SaiTracer::getInstance()->logCreateFn(
      "create_hostif_trap_group",
      hostif_trap_group_id,
      switch_id,
      attr_count,
      attr_list,
      SAI_OBJECT_TYPE_HOSTIF_TRAP_GROUP,
      rv);
  return rv;
}

sai_status_t wrap_remove_hostif_trap_group(
    sai_object_id_t hostif_trap_group_id) {
  auto rv = SaiTracer::getInstance()->hostifApi_->remove_hostif_trap_group(
      hostif_trap_group_id);

  SaiTracer::getInstance()->logRemoveFn(
      "remove_hostif_trap_group",
      hostif_trap_group_id,
      SAI_OBJECT_TYPE_HOSTIF_TRAP_GROUP,
      rv);
  return rv;
}

sai_status_t wrap_set_hostif_trap_group_attribute(
    sai_object_id_t hostif_trap_group_id,
    const sai_attribute_t* attr) {
  auto rv =
      SaiTracer::getInstance()->hostifApi_->set_hostif_trap_group_attribute(
          hostif_trap_group_id, attr);

  SaiTracer::getInstance()->logSetAttrFn(
      "set_hostif_trap_group_attribute",
      hostif_trap_group_id,
      attr,
      SAI_OBJECT_TYPE_HOSTIF_TRAP_GROUP,
      rv);
  return rv;
}

sai_status_t wrap_get_hostif_trap_group_attribute(
    sai_object_id_t hostif_trap_group_id,
    uint32_t attr_count,
    sai_attribute_t* attr_list) {
  // TODO(zecheng): Log get functions as well
  return SaiTracer::getInstance()->hostifApi_->get_hostif_trap_group_attribute(
      hostif_trap_group_id, attr_count, attr_list);
}

sai_status_t wrap_send_hostif_packet(
    sai_object_id_t hostif_id,
    sai_size_t buffer_size,
    const void* buffer,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  auto rv = SaiTracer::getInstance()->hostifApi_->send_hostif_packet(
      hostif_id, buffer_size, buffer, attr_count, attr_list);

  SaiTracer::getInstance()->logSendHostifPacketFn(
      hostif_id,
      buffer_size,
      static_cast<const uint8_t*>(buffer),
      attr_count,
      attr_list,
      rv);
  return rv;
}

sai_hostif_api_t* wrappedHostifApi() {
  static sai_hostif_api_t hostifWrappers;

  hostifWrappers.create_hostif_trap = &wrap_create_hostif_trap;
  hostifWrappers.remove_hostif_trap = &wrap_remove_hostif_trap;
  hostifWrappers.set_hostif_trap_attribute = &wrap_set_hostif_trap_attribute;
  hostifWrappers.get_hostif_trap_attribute = &wrap_get_hostif_trap_attribute;
  hostifWrappers.create_hostif_trap_group = &wrap_create_hostif_trap_group;
  hostifWrappers.remove_hostif_trap_group = &wrap_remove_hostif_trap_group;
  hostifWrappers.set_hostif_trap_group_attribute =
      &wrap_set_hostif_trap_group_attribute;
  hostifWrappers.get_hostif_trap_group_attribute =
      &wrap_get_hostif_trap_group_attribute;
  hostifWrappers.send_hostif_packet = &wrap_send_hostif_packet;

  return &hostifWrappers;
}

void setHostifTrapAttributes(
    const sai_attribute_t* attr_list,
    uint32_t attr_count,
    std::vector<std::string>& attrLines) {
  for (int i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      case SAI_HOSTIF_TRAP_ATTR_TRAP_GROUP:
        attrLines.push_back(oidAttr(attr_list, i));
        break;
      case SAI_HOSTIF_TRAP_ATTR_PACKET_ACTION:
      case SAI_HOSTIF_TRAP_ATTR_TRAP_TYPE:
        attrLines.push_back(s32Attr(attr_list, i));
        break;
      case SAI_HOSTIF_TRAP_ATTR_TRAP_PRIORITY:
        attrLines.push_back(u32Attr(attr_list, i));
        break;
      default:
        // TODO(zecheng): Better check for newly added attributes (T69350100)
        break;
    }
  }
}

void setHostifTrapGroupAttributes(
    const sai_attribute_t* attr_list,
    uint32_t attr_count,
    std::vector<std::string>& attrLines) {
  for (int i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      case SAI_HOSTIF_TRAP_GROUP_ATTR_POLICER:
        attrLines.push_back(oidAttr(attr_list, i));
        break;
      case SAI_HOSTIF_TRAP_GROUP_ATTR_QUEUE:
        attrLines.push_back(u32Attr(attr_list, i));
        break;
      default:
        // TODO(zecheng): Better check for newly added attributes (T69350100)
        break;
    }
  }
}

void setHostifPacketAttributes(
    const sai_attribute_t* attr_list,
    uint32_t attr_count,
    std::vector<std::string>& attrLines) {
  for (int i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      case SAI_HOSTIF_PACKET_ATTR_HOSTIF_TX_TYPE:
        attrLines.push_back(s32Attr(attr_list, i));
        break;
      case SAI_HOSTIF_PACKET_ATTR_EGRESS_PORT_OR_LAG:
        attrLines.push_back(oidAttr(attr_list, i));
        break;
      case SAI_HOSTIF_PACKET_ATTR_EGRESS_QUEUE_INDEX:
        attrLines.push_back(u8Attr(attr_list, i));
        break;
      default:
        // TODO(zecheng): Better check for newly added attributes (T69350100)
        break;
    }
  }
}

} // namespace facebook::fboss
