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

#include <folly/logging/xlog.h>

#include <optional>
#include <tuple>

extern "C" {
#include <sai.h>
}

namespace facebook::fboss {

class PortApi;

struct SaiPortTraits {
  static constexpr sai_object_type_t ObjectType = SAI_OBJECT_TYPE_PORT;
  using SaiApiT = PortApi;
  struct Attributes {
    using EnumType = sai_port_attr_t;
    using AdminState = SaiAttribute<EnumType, SAI_PORT_ATTR_ADMIN_STATE, bool>;
    using HwLaneList = SaiAttribute<
        EnumType,
        SAI_PORT_ATTR_HW_LANE_LIST,
        std::vector<uint32_t>>;
    using Speed = SaiAttribute<EnumType, SAI_PORT_ATTR_SPEED, sai_uint32_t>;
    using Type = SaiAttribute<EnumType, SAI_PORT_ATTR_TYPE, sai_int32_t>;
    using QosNumberOfQueues = SaiAttribute<
        EnumType,
        SAI_PORT_ATTR_QOS_NUMBER_OF_QUEUES,
        sai_uint32_t>;
    using QosQueueList = SaiAttribute<
        EnumType,
        SAI_PORT_ATTR_QOS_QUEUE_LIST,
        std::vector<sai_object_id_t>>;
    using FecMode = SaiAttribute<EnumType, SAI_PORT_ATTR_FEC_MODE, sai_int32_t>;
    using OperStatus =
        SaiAttribute<EnumType, SAI_PORT_ATTR_OPER_STATUS, sai_int32_t>;
    using InternalLoopbackMode = SaiAttribute<
        EnumType,
        SAI_PORT_ATTR_INTERNAL_LOOPBACK_MODE,
        sai_int32_t>;
    using MediaType =
        SaiAttribute<EnumType, SAI_PORT_ATTR_MEDIA_TYPE, sai_int32_t>;
    using GlobalFlowControlMode = SaiAttribute<
        EnumType,
        SAI_PORT_ATTR_GLOBAL_FLOW_CONTROL_MODE,
        sai_int32_t>;
    using PortVlanId =
        SaiAttribute<EnumType, SAI_PORT_ATTR_PORT_VLAN_ID, sai_uint16_t>;
    using Preemphasis = SaiAttribute<
        EnumType,
        SAI_PORT_ATTR_SERDES_PREEMPHASIS,
        std::vector<uint32_t>>;
    using Mtu = SaiAttribute<EnumType, SAI_PORT_ATTR_MTU, sai_uint32_t>;
    using QosDscpToTcMap = SaiAttribute<
        EnumType,
        SAI_PORT_ATTR_QOS_DSCP_TO_TC_MAP,
        SaiObjectIdT,
        SaiObjectIdDefault>;
    using QosTcToQueueMap = SaiAttribute<
        EnumType,
        SAI_PORT_ATTR_QOS_TC_TO_QUEUE_MAP,
        SaiObjectIdT,
        SaiObjectIdDefault>;
    using DisableTtlDecrement = SaiAttribute<
        EnumType,
        SAI_PORT_ATTR_DECREMENT_TTL,
        bool,
        SaiBoolDefault>;
    using InterfaceType = SaiAttribute<
        EnumType,
        SAI_PORT_ATTR_INTERFACE_TYPE,
        sai_int32_t,
        SaiPortInterfaceTypeDefault>;
  };
  using AdapterKey = PortSaiId;
  using AdapterHostKey = Attributes::HwLaneList;

  using CreateAttributes = std::tuple<
      Attributes::HwLaneList,
      Attributes::Speed,
      std::optional<Attributes::AdminState>,
      std::optional<Attributes::FecMode>,
      std::optional<Attributes::InternalLoopbackMode>,
      std::optional<Attributes::MediaType>,
      std::optional<Attributes::GlobalFlowControlMode>,
      std::optional<Attributes::PortVlanId>,
      std::optional<Attributes::Preemphasis>,
      std::optional<Attributes::Mtu>,
      std::optional<Attributes::QosDscpToTcMap>,
      std::optional<Attributes::QosTcToQueueMap>,
      std::optional<Attributes::DisableTtlDecrement>,
      std::optional<Attributes::InterfaceType>>;

  static constexpr std::array<sai_stat_id_t, 16> CounterIdsToRead = {
      SAI_PORT_STAT_IF_IN_OCTETS,
      SAI_PORT_STAT_IF_IN_UCAST_PKTS,
      SAI_PORT_STAT_IF_IN_MULTICAST_PKTS,
      SAI_PORT_STAT_IF_IN_BROADCAST_PKTS,
      SAI_PORT_STAT_IF_IN_DISCARDS,
      SAI_PORT_STAT_IF_IN_ERRORS,
      SAI_PORT_STAT_PAUSE_RX_PKTS,
      SAI_PORT_STAT_IF_OUT_OCTETS,
      SAI_PORT_STAT_IF_OUT_UCAST_PKTS,
      SAI_PORT_STAT_IF_OUT_MULTICAST_PKTS,
      SAI_PORT_STAT_IF_OUT_BROADCAST_PKTS,
      SAI_PORT_STAT_IF_OUT_DISCARDS,
      SAI_PORT_STAT_IF_OUT_ERRORS,
      SAI_PORT_STAT_PAUSE_TX_PKTS,
      SAI_PORT_STAT_WRED_DROPPED_PACKETS,
      SAI_PORT_STAT_ECN_MARKED_PACKETS,
  };
  static constexpr std::array<sai_stat_id_t, 0> CounterIdsToReadAndClear = {};
};

SAI_ATTRIBUTE_NAME(Port, HwLaneList)
SAI_ATTRIBUTE_NAME(Port, Speed)
SAI_ATTRIBUTE_NAME(Port, AdminState)
SAI_ATTRIBUTE_NAME(Port, FecMode)
SAI_ATTRIBUTE_NAME(Port, OperStatus)
SAI_ATTRIBUTE_NAME(Port, InternalLoopbackMode)
SAI_ATTRIBUTE_NAME(Port, MediaType)
SAI_ATTRIBUTE_NAME(Port, GlobalFlowControlMode)
SAI_ATTRIBUTE_NAME(Port, PortVlanId)
SAI_ATTRIBUTE_NAME(Port, Preemphasis)
SAI_ATTRIBUTE_NAME(Port, Mtu)
SAI_ATTRIBUTE_NAME(Port, QosDscpToTcMap)
SAI_ATTRIBUTE_NAME(Port, QosTcToQueueMap)
SAI_ATTRIBUTE_NAME(Port, DisableTtlDecrement)

SAI_ATTRIBUTE_NAME(Port, QosNumberOfQueues)
SAI_ATTRIBUTE_NAME(Port, QosQueueList)
SAI_ATTRIBUTE_NAME(Port, Type)
SAI_ATTRIBUTE_NAME(Port, InterfaceType)

template <>
struct SaiObjectHasStats<SaiPortTraits> : public std::true_type {};

struct SaiPortSerdesTraits {
  static constexpr sai_object_type_t ObjectType = SAI_OBJECT_TYPE_PORT_SERDES;
  using SaiApiT = PortApi;
  struct Attributes {
    using EnumType = sai_port_serdes_attr_t;
    using PortId =
        SaiAttribute<EnumType, SAI_PORT_SERDES_ATTR_PORT_ID, SaiObjectIdT>;
    using IDriver = SaiAttribute<
        EnumType,
        SAI_PORT_SERDES_ATTR_IDRIVER,
        std::vector<sai_uint32_t>>;
    using TxFirPre1 = SaiAttribute<
        EnumType,
        SAI_PORT_SERDES_ATTR_TX_FIR_PRE1,
        std::vector<sai_uint32_t>>;
    using TxFirPre2 = SaiAttribute<
        EnumType,
        SAI_PORT_SERDES_ATTR_TX_FIR_PRE2,
        std::vector<sai_uint32_t>>;
    using TxFirMain = SaiAttribute<
        EnumType,
        SAI_PORT_SERDES_ATTR_TX_FIR_MAIN,
        std::vector<sai_uint32_t>>;
    using TxFirPost1 = SaiAttribute<
        EnumType,
        SAI_PORT_SERDES_ATTR_TX_FIR_POST1,
        std::vector<sai_uint32_t>>;
    using TxFirPost2 = SaiAttribute<
        EnumType,
        SAI_PORT_SERDES_ATTR_TX_FIR_POST2,
        std::vector<sai_uint32_t>>;
    using TxFirPost3 = SaiAttribute<
        EnumType,
        SAI_PORT_SERDES_ATTR_TX_FIR_POST3,
        std::vector<sai_uint32_t>>;

    /* extension attributes */
    struct AttributeRxCtleCodeIdWrapper {
      std::optional<sai_attr_id_t> operator()();
    };

    struct AttributeRxDspModeIdWrapper {
      std::optional<sai_attr_id_t> operator()();
    };

    struct AttributeRxAfeTrimIdWrapper {
      std::optional<sai_attr_id_t> operator()();
    };

    struct AttributeRxAcCouplingBypassIdWrapper {
      std::optional<sai_attr_id_t> operator()();
    };
    using RxCtleCode = SaiExtensionAttribute<
        std::vector<sai_int32_t>,
        AttributeRxCtleCodeIdWrapper>;
    using RxDspMode = SaiExtensionAttribute<
        std::vector<sai_int32_t>,
        AttributeRxDspModeIdWrapper>;
    using RxAfeTrim = SaiExtensionAttribute<
        std::vector<sai_int32_t>,
        AttributeRxAfeTrimIdWrapper>;
    using RxAcCouplingByPass = SaiExtensionAttribute<
        std::vector<sai_int32_t>,
        AttributeRxAcCouplingBypassIdWrapper>;
  };
  using AdapterKey = PortSerdesSaiId;
  using AdapterHostKey = Attributes::PortId;
  using CreateAttributes = std::tuple<
      Attributes::PortId,
      std::optional<Attributes::IDriver>,
      std::optional<Attributes::TxFirPre1>,
      std::optional<Attributes::TxFirPre2>,
      std::optional<Attributes::TxFirMain>,
      std::optional<Attributes::TxFirPost1>,
      std::optional<Attributes::TxFirPost2>,
      std::optional<Attributes::TxFirPost3>,
      std::optional<Attributes::RxCtleCode>,
      std::optional<Attributes::RxDspMode>,
      std::optional<Attributes::RxAfeTrim>,
      std::optional<Attributes::RxAcCouplingByPass>>;
};

template <>
struct GetObjectKeySupported<SaiPortSerdesTraits> : std::false_type {};

SAI_ATTRIBUTE_NAME(PortSerdes, PortId);
SAI_ATTRIBUTE_NAME(PortSerdes, IDriver);
SAI_ATTRIBUTE_NAME(PortSerdes, TxFirPre1);
SAI_ATTRIBUTE_NAME(PortSerdes, TxFirPre2);
SAI_ATTRIBUTE_NAME(PortSerdes, TxFirMain);
SAI_ATTRIBUTE_NAME(PortSerdes, TxFirPost1);
SAI_ATTRIBUTE_NAME(PortSerdes, TxFirPost2);
SAI_ATTRIBUTE_NAME(PortSerdes, TxFirPost3);
SAI_ATTRIBUTE_NAME(PortSerdes, RxCtleCode);
SAI_ATTRIBUTE_NAME(PortSerdes, RxDspMode);
SAI_ATTRIBUTE_NAME(PortSerdes, RxAfeTrim);
SAI_ATTRIBUTE_NAME(PortSerdes, RxAcCouplingByPass);

class PortApi : public SaiApi<PortApi> {
 public:
  static constexpr sai_api_t ApiType = SAI_API_PORT;
  PortApi() {
    sai_status_t status =
        sai_api_query(ApiType, reinterpret_cast<void**>(&api_));
    saiApiCheckError(status, ApiType, "Failed to query for port api");
  }

 private:
  sai_status_t _create(
      PortSaiId* id,
      sai_object_id_t switch_id,
      size_t count,
      sai_attribute_t* attr_list) const {
    return api_->create_port(rawSaiId(id), switch_id, count, attr_list);
  }
  sai_status_t _remove(PortSaiId key) {
    return api_->remove_port(key);
  }
  sai_status_t _getAttribute(PortSaiId key, sai_attribute_t* attr) const {
    return api_->get_port_attribute(key, 1, attr);
  }
  sai_status_t _setAttribute(PortSaiId key, const sai_attribute_t* attr) {
    return api_->set_port_attribute(key, attr);
  }

  sai_status_t _create(
      PortSerdesSaiId* id,
      sai_object_id_t switch_id,
      size_t count,
      sai_attribute_t* attr_list) {
    return api_->create_port_serdes(rawSaiId(id), switch_id, count, attr_list);
  }

  sai_status_t _remove(PortSerdesSaiId id) {
    return api_->remove_port_serdes(id);
  }

  sai_status_t _getAttribute(PortSerdesSaiId key, sai_attribute_t* attr) const {
    return api_->get_port_serdes_attribute(key, 1, attr);
  }

  sai_status_t _setAttribute(PortSerdesSaiId key, const sai_attribute_t* attr) {
    return api_->set_port_serdes_attribute(key, attr);
  }

  sai_status_t _getStats(
      PortSaiId key,
      uint32_t num_of_counters,
      const sai_stat_id_t* counter_ids,
      sai_stats_mode_t mode,
      uint64_t* counters) const {
    /*
     * Unfortunately not all vendors implement the ext stats api.
     * ext stats api matter only for modes other than the (default)
     * SAI_STATS_MODE_READ. So play defensive and call ext mode only
     * when called with something other than default
     */
    return mode == SAI_STATS_MODE_READ
        ? api_->get_port_stats(key, num_of_counters, counter_ids, counters)
        : api_->get_port_stats_ext(
              key, num_of_counters, counter_ids, mode, counters);
  }

  sai_status_t _clearStats(
      PortSaiId key,
      uint32_t num_of_counters,
      const sai_stat_id_t* counter_ids) const {
    return api_->clear_port_stats(key, num_of_counters, counter_ids);
  }

  sai_port_api_t* api_;
  friend class SaiApi<PortApi>;
};

} // namespace facebook::fboss
