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
#include "fboss/agent/hw/sai/api/Types.h"

#include <folly/MacAddress.h>
#include <folly/logging/xlog.h>

#include <tuple>
#include <vector>

extern "C" {
#include <sai.h>
}

namespace facebook {
namespace fboss {

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
  using CreateAttributes = std::tuple<
      std::optional<Attributes::Queue>,
      std::optional<Attributes::Policer>>;
};

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

struct HostifApiParameters {
  static constexpr sai_api_t ApiType = SAI_API_HOSTIF;
  struct TxPacketAttributes {
    using EnumType = sai_hostif_packet_attr_t;
    using TxType = SaiAttribute<
        EnumType,
        SAI_HOSTIF_PACKET_ATTR_HOSTIF_TX_TYPE,
        sai_int32_t>;
    using EgressPortOrLag = SaiAttribute<
        EnumType,
        SAI_HOSTIF_PACKET_ATTR_EGRESS_PORT_OR_LAG,
        SaiObjectIdT>;
    using TxAttributes =
        SaiAttributeTuple<TxType, SaiAttributeOptional<EgressPortOrLag>>;
    TxPacketAttributes(const TxAttributes& attrs) {
      std::tie(txType, egressPortOrLag) = attrs.value();
    }
    TxAttributes attrs() const {
      return {txType, egressPortOrLag};
    }
    folly::Optional<typename EgressPortOrLag::ValueType> egressPortOrLag;
    typename TxType::ValueType txType;
  };

  struct RxPacketAttributes {
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
    using RxAttributes = SaiAttributeTuple<TrapId, IngressPort, IngressLag>;
    RxPacketAttributes(const RxAttributes& attrs) {
      std::tie(trapId, ingressPort, ingressLag) = attrs.value();
    }
    RxAttributes attrs() const {
      return {trapId, ingressPort, ingressLag};
    }
    typename TrapId::ValueType trapId;
    typename IngressPort::ValueType ingressPort;
    typename IngressLag::ValueType ingressLag;
  };

  struct HostifApiPacket {
    void* buffer;
    size_t size;
    HostifApiPacket(void* buffer, size_t size) : buffer(buffer), size(size) {}
  };

  struct Attributes {
    using EnumType = sai_hostif_trap_group_attr_t;
    using Queue =
        SaiAttribute<EnumType, SAI_HOSTIF_TRAP_GROUP_ATTR_QUEUE, sai_uint32_t>;
    using Policer = SaiAttribute<
        EnumType,
        SAI_HOSTIF_TRAP_GROUP_ATTR_POLICER,
        SaiObjectIdT>;
    using CreateAttributes = SaiAttributeTuple<
        SaiAttributeOptional<Queue>,
        SaiAttributeOptional<Policer>>;
    Attributes(const CreateAttributes& attrs) {
      std::tie(queue, policer) = attrs.value();
    }
    CreateAttributes attrs() const {
      return {queue, policer};
    }
    bool operator==(const Attributes& other) const {
      return attrs() == other.attrs();
    }
    bool operator!=(const Attributes& other) const {
      return !(*this == other);
    }
    folly::Optional<typename Queue::ValueType> queue;
    folly::Optional<typename Policer::ValueType> policer;
  };

  struct MemberAttributes {
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
    using CreateAttributes = SaiAttributeTuple<
        TrapType,
        PacketAction,
        SaiAttributeOptional<TrapPriority>,
        SaiAttributeOptional<TrapGroup>>;
    MemberAttributes(const CreateAttributes& attrs) {
      std::tie(trapType, packetAction, trapPriority, trapGroup) = attrs.value();
    }
    CreateAttributes attrs() const {
      return {trapType, packetAction, trapPriority, trapGroup};
    }
    bool operator==(const MemberAttributes& other) const {
      return attrs() == other.attrs();
    }
    bool operator!=(const MemberAttributes& other) const {
      return !(*this == other);
    }

    typename TrapType::ValueType trapType;
    typename PacketAction::ValueType packetAction;
    folly::Optional<typename TrapPriority::ValueType> trapPriority;
    folly::Optional<typename TrapGroup::ValueType> trapGroup;
  };
};

class HostifApi : public SaiApi<HostifApi, HostifApiParameters> {
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

  sai_status_t _create(
      sai_object_id_t* hostif_trap_group_id,
      sai_attribute_t* attr_list,
      size_t count,
      sai_object_id_t switch_id) {
    return api_->create_hostif_trap_group(
        hostif_trap_group_id, switch_id, count, attr_list);
  }
  sai_status_t _remove(sai_object_id_t hostif_trap_group_id) {
    return api_->remove_hostif_trap_group(hostif_trap_group_id);
  }
  sai_status_t _getAttr(sai_attribute_t* attr, sai_object_id_t id) const {
    return api_->get_hostif_trap_group_attribute(id, 1, attr);
  }
  sai_status_t _setAttr(const sai_attribute_t* attr, sai_object_id_t id) {
    return api_->set_hostif_trap_group_attribute(id, attr);
  }
  sai_status_t _createMember(
      sai_object_id_t* hostif_trap_id,
      sai_attribute_t* attr_list,
      size_t count,
      sai_object_id_t switch_id) {
    return api_->create_hostif_trap(
        hostif_trap_id, switch_id, count, attr_list);
  }
  sai_status_t _removeMember(sai_object_id_t hostif_trap_id) {
    return api_->remove_hostif_trap(hostif_trap_id);
  }
  sai_status_t _getMemberAttr(
      sai_attribute_t* attr,
      sai_object_id_t hostif_trap_id) const {
    return api_->get_hostif_trap_attribute(hostif_trap_id, 1, attr);
  }
  sai_status_t _setMemberAttr(
      const sai_attribute_t* attr,
      sai_object_id_t hostif_trap_id) {
    return api_->set_hostif_trap_attribute(hostif_trap_id, attr);
  }
  sai_hostif_api_t* api_;
  friend class SaiApi<HostifApi, HostifApiParameters>;

 public:
  sai_status_t send(
      const HostifApiParameters::TxPacketAttributes::TxAttributes& attributes,
      sai_object_id_t switch_id,
      HostifApiParameters::HostifApiPacket& txPacket) {
    std::vector<sai_attribute_t> saiAttributeTs = attributes.saiAttrs();
    return api_->send_hostif_packet(
        switch_id,
        txPacket.size,
        txPacket.buffer,
        saiAttributeTs.size(),
        saiAttributeTs.data());
  }
};
} // namespace fboss
} // namespace facebook
