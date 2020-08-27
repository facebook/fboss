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

#include "fboss/agent/hw/sai/api/SaiApi.h"
#include "fboss/agent/hw/sai/api/SaiAttribute.h"
#include "fboss/agent/hw/sai/api/SaiAttributeDataTypes.h"
#include "fboss/agent/hw/sai/api/SaiVersion.h"
#include "fboss/agent/hw/sai/api/Types.h"

#include <folly/MacAddress.h>
#include <folly/logging/xlog.h>

#include <tuple>
#include <vector>

extern "C" {
#include <sai.h>
}

namespace facebook::fboss {

class HostifApi;

struct SaiHostifTrapGroupTraits {
  static constexpr sai_object_type_t ObjectType =
      SAI_OBJECT_TYPE_HOSTIF_TRAP_GROUP;
  using SaiApiT = HostifApi;
  struct Attributes {
    using EnumType = sai_hostif_trap_group_attr_t;
    using Queue =
        SaiAttribute<EnumType, SAI_HOSTIF_TRAP_GROUP_ATTR_QUEUE, sai_uint32_t>;
    using Policer = SaiAttribute<
        EnumType,
        SAI_HOSTIF_TRAP_GROUP_ATTR_POLICER,
        SaiObjectIdT>;
  };

  using AdapterKey = HostifTrapGroupSaiId;
  // Queue may not always uniquely identify a trap group the more general
  // adapter host key type is the set of trap types in the trap group -- but
  // this works for now, and is quite a bit simpler.
  using AdapterHostKey = Attributes::Queue;

  // Queue is optional, but as noted above treating it at as mandatory is
  // temporarily convenient
  // TODO(borisb): possibly redesign AdapterHostKey and CreateAttributes here
  using CreateAttributes =
      std::tuple<Attributes::Queue, std::optional<Attributes::Policer>>;
};

SAI_ATTRIBUTE_NAME(HostifTrapGroup, Queue)
SAI_ATTRIBUTE_NAME(HostifTrapGroup, Policer)

struct SaiHostifTrapTraits {
  static constexpr sai_object_type_t ObjectType = SAI_OBJECT_TYPE_HOSTIF_TRAP;
  using SaiApiT = HostifApi;
  struct Attributes {
    using EnumType = sai_hostif_trap_attr_t;
    using PacketAction =
        SaiAttribute<EnumType, SAI_HOSTIF_TRAP_ATTR_PACKET_ACTION, sai_int32_t>;
    using TrapGroup =
        SaiAttribute<EnumType, SAI_HOSTIF_TRAP_ATTR_TRAP_GROUP, SaiObjectIdT>;
    using TrapPriority = SaiAttribute<
        EnumType,
        SAI_HOSTIF_TRAP_ATTR_TRAP_PRIORITY,
        sai_uint32_t>;
    using TrapType =
        SaiAttribute<EnumType, SAI_HOSTIF_TRAP_ATTR_TRAP_TYPE, sai_int32_t>;
  };
  using AdapterKey = HostifTrapSaiId;
  using AdapterHostKey = Attributes::TrapType;
  using CreateAttributes = std::tuple<
      Attributes::TrapType,
      Attributes::PacketAction,
      std::optional<Attributes::TrapPriority>,
      std::optional<Attributes::TrapGroup>>;
};

SAI_ATTRIBUTE_NAME(HostifTrap, TrapType)
SAI_ATTRIBUTE_NAME(HostifTrap, PacketAction)
SAI_ATTRIBUTE_NAME(HostifTrap, TrapPriority)
SAI_ATTRIBUTE_NAME(HostifTrap, TrapGroup)

// TX and RX packets aren't proper SaiObjectTraits, but we'll follow the pattern
// for defining their attributes
struct SaiTxPacketTraits {
  struct Attributes {
    using EnumType = sai_hostif_packet_attr_t;
    using TxType = SaiAttribute<
        EnumType,
        SAI_HOSTIF_PACKET_ATTR_HOSTIF_TX_TYPE,
        sai_int32_t>;
    using EgressPortOrLag = SaiAttribute<
        EnumType,
        SAI_HOSTIF_PACKET_ATTR_EGRESS_PORT_OR_LAG,
        SaiObjectIdT>;
    using EgressQueueIndex = SaiAttribute<
        EnumType,
        SAI_HOSTIF_PACKET_ATTR_EGRESS_QUEUE_INDEX,
        sai_uint8_t>;
  };
  using TxAttributes = std::tuple<
      Attributes::TxType,
      std::optional<Attributes::EgressPortOrLag>,
      std::optional<Attributes::EgressQueueIndex>>;
};
SAI_ATTRIBUTE_NAME(TxPacket, TxType)
SAI_ATTRIBUTE_NAME(TxPacket, EgressPortOrLag)
SAI_ATTRIBUTE_NAME(TxPacket, EgressQueueIndex)

struct SaiRxPacketTraits {
  struct Attributes {
    using EnumType = sai_hostif_packet_attr_t;
    using TrapId = SaiAttribute<
        EnumType,
        SAI_HOSTIF_PACKET_ATTR_HOSTIF_TRAP_ID,
        SaiObjectIdT>;
    using IngressPort = SaiAttribute<
        EnumType,
        SAI_HOSTIF_PACKET_ATTR_INGRESS_PORT,
        SaiObjectIdT>;
    using IngressLag = SaiAttribute<
        EnumType,
        SAI_HOSTIF_PACKET_ATTR_INGRESS_LAG,
        SaiObjectIdT>;
  };
  using RxAttributes = std::tuple<
      Attributes::TrapId,
      Attributes::IngressPort,
      Attributes::IngressLag>;
};

SAI_ATTRIBUTE_NAME(RxPacket, TrapId)
SAI_ATTRIBUTE_NAME(RxPacket, IngressPort)
SAI_ATTRIBUTE_NAME(RxPacket, IngressLag)

struct SaiHostifApiPacket {
  void* buffer;
  size_t size;
  SaiHostifApiPacket(void* buffer, size_t size) : buffer(buffer), size(size) {}
};

class HostifApi : public SaiApi<HostifApi> {
 public:
  static constexpr sai_api_t ApiType = SAI_API_HOSTIF;
  HostifApi() {
    sai_status_t status =
        sai_api_query(ApiType, reinterpret_cast<void**>(&api_));
    saiApiCheckError(status, ApiType, "Failed to query for switch api");
  }

 private:
  sai_status_t _create(
      HostifTrapGroupSaiId* id,
      sai_object_id_t switch_id,
      size_t count,
      sai_attribute_t* attr_list) {
    return api_->create_hostif_trap_group(
        rawSaiId(id), switch_id, count, attr_list);
  }
  sai_status_t _create(
      HostifTrapSaiId* id,
      sai_object_id_t switch_id,
      size_t count,
      sai_attribute_t* attr_list) {
    return api_->create_hostif_trap(rawSaiId(id), switch_id, count, attr_list);
  }

  sai_status_t _remove(HostifTrapGroupSaiId hostif_trap_group_id) {
    return api_->remove_hostif_trap_group(hostif_trap_group_id);
  }
  sai_status_t _remove(HostifTrapSaiId hostif_trap_id) {
    return api_->remove_hostif_trap(hostif_trap_id);
  }
  sai_status_t _getAttribute(HostifTrapGroupSaiId id, sai_attribute_t* attr)
      const {
    return api_->get_hostif_trap_group_attribute(id, 1, attr);
  }
  sai_status_t _getAttribute(HostifTrapSaiId id, sai_attribute_t* attr) const {
    return api_->get_hostif_trap_attribute(id, 1, attr);
  }
  sai_status_t _setAttribute(
      HostifTrapGroupSaiId id,
      const sai_attribute_t* attr) {
    return api_->set_hostif_trap_group_attribute(id, attr);
  }
  sai_status_t _setAttribute(HostifTrapSaiId id, const sai_attribute_t* attr) {
    return api_->set_hostif_trap_attribute(id, attr);
  }

  sai_hostif_api_t* api_;
  friend class SaiApi<HostifApi>;

 public:
  sai_status_t send(
      const SaiTxPacketTraits::TxAttributes& attributes,
      sai_object_id_t switch_id,
      const SaiHostifApiPacket& txPacket) {
    std::vector<sai_attribute_t> saiAttributeTs = saiAttrs(attributes);
    return api_->send_hostif_packet(
        switch_id,
        txPacket.size,
        txPacket.buffer,
        saiAttributeTs.size(),
        saiAttributeTs.data());
  }
};

} // namespace facebook::fboss
