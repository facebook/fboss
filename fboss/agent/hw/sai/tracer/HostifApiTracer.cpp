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
#include <typeindex>
#include <utility>

#include "fboss/agent/hw/sai/api/HostifApi.h"
#include "fboss/agent/hw/sai/tracer/Utils.h"

using folly::to;

namespace {
std::map<int32_t, std::pair<std::string, std::size_t>> _HostifTrapMap{
    SAI_ATTR_MAP(HostifTrap, TrapType),
    SAI_ATTR_MAP(HostifTrap, PacketAction),
    SAI_ATTR_MAP(HostifTrap, TrapPriority),
    SAI_ATTR_MAP(HostifTrap, TrapGroup),
    SAI_ATTR_MAP(HostifTrap, TrapCounterId),
};

std::map<int32_t, std::pair<std::string, std::size_t>> _HostifTrapGroupMap{
    SAI_ATTR_MAP(HostifTrapGroup, Queue),
    SAI_ATTR_MAP(HostifTrapGroup, Policer),
};

std::map<int32_t, std::pair<std::string, std::size_t>> _HostifPacketMap{
    SAI_ATTR_MAP(TxPacket, TxType),
    SAI_ATTR_MAP(TxPacket, EgressPortOrLag),
    SAI_ATTR_MAP(TxPacket, EgressQueueIndex),
};

} // namespace

namespace facebook::fboss {

WRAP_CREATE_FUNC(hostif_trap, SAI_OBJECT_TYPE_HOSTIF_TRAP, hostif);
WRAP_REMOVE_FUNC(hostif_trap, SAI_OBJECT_TYPE_HOSTIF_TRAP, hostif);
WRAP_SET_ATTR_FUNC(hostif_trap, SAI_OBJECT_TYPE_HOSTIF_TRAP, hostif);
WRAP_GET_ATTR_FUNC(hostif_trap, SAI_OBJECT_TYPE_HOSTIF_TRAP, hostif);

WRAP_CREATE_FUNC(hostif_trap_group, SAI_OBJECT_TYPE_HOSTIF_TRAP_GROUP, hostif);
WRAP_REMOVE_FUNC(hostif_trap_group, SAI_OBJECT_TYPE_HOSTIF_TRAP_GROUP, hostif);
WRAP_SET_ATTR_FUNC(
    hostif_trap_group,
    SAI_OBJECT_TYPE_HOSTIF_TRAP_GROUP,
    hostif);
WRAP_GET_ATTR_FUNC(
    hostif_trap_group,
    SAI_OBJECT_TYPE_HOSTIF_TRAP_GROUP,
    hostif);

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

SET_SAI_ATTRIBUTES(HostifTrap)
SET_SAI_ATTRIBUTES(HostifTrapGroup)
SET_SAI_ATTRIBUTES(HostifPacket)

} // namespace facebook::fboss
