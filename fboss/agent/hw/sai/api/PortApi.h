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
    struct AttributeSerdesLaneList {
      std::optional<sai_attr_id_t> operator()();
    };
    using SerdesLaneList =
        SaiExtensionAttribute<std::vector<uint32_t>, AttributeSerdesLaneList>;
    struct AttributeDiagModeEnable {
      std::optional<sai_attr_id_t> operator()();
    };
    using DiagModeEnable = SaiExtensionAttribute<bool, AttributeDiagModeEnable>;
    using Speed = SaiAttribute<EnumType, SAI_PORT_ATTR_SPEED, sai_uint32_t>;
    using Type = SaiAttribute<EnumType, SAI_PORT_ATTR_TYPE, sai_int32_t>;
    using QosNumberOfQueues = SaiAttribute<
        EnumType,
        SAI_PORT_ATTR_QOS_NUMBER_OF_QUEUES,
        sai_uint32_t,
        SaiIntDefault<sai_uint32_t>>;
    using QosQueueList = SaiAttribute<
        EnumType,
        SAI_PORT_ATTR_QOS_QUEUE_LIST,
        std::vector<sai_object_id_t>,
        SaiObjectIdListDefault>;
    using FecMode = SaiAttribute<EnumType, SAI_PORT_ATTR_FEC_MODE, sai_int32_t>;
    using OperStatus =
        SaiAttribute<EnumType, SAI_PORT_ATTR_OPER_STATUS, sai_int32_t>;
    using InternalLoopbackMode = SaiAttribute<
        EnumType,
        SAI_PORT_ATTR_INTERNAL_LOOPBACK_MODE,
        sai_int32_t,
        SaiIntDefault<sai_int32_t>>;
#if SAI_API_VERSION >= SAI_VERSION(1, 11, 0)
    using FabricIsolate =
        SaiAttribute<EnumType, SAI_PORT_ATTR_FABRIC_ISOLATE, bool>;
#endif
#if SAI_API_VERSION >= SAI_VERSION(1, 10, 0)
    using PortLoopbackMode = SaiAttribute<
        EnumType,
        SAI_PORT_ATTR_LOOPBACK_MODE,
        sai_int32_t,
        SaiIntDefault<sai_int32_t>>;
    using UseExtendedFec = SaiAttribute<
        EnumType,
        SAI_PORT_ATTR_USE_EXTENDED_FEC,
        bool,
        SaiBoolDefaultFalse>;
    using ExtendedFecMode = SaiAttribute<
        EnumType,
        SAI_PORT_ATTR_FEC_MODE_EXTENDED,
        sai_int32_t,
        SaiIntDefault<sai_int32_t>>;
#endif
    using MediaType = SaiAttribute<
        EnumType,
        SAI_PORT_ATTR_MEDIA_TYPE,
        sai_int32_t,
        SaiIntDefault<sai_int32_t>>;
    using GlobalFlowControlMode = SaiAttribute<
        EnumType,
        SAI_PORT_ATTR_GLOBAL_FLOW_CONTROL_MODE,
        sai_int32_t,
        SaiIntDefault<sai_int32_t>>;
    using PortVlanId = SaiAttribute<
        EnumType,
        SAI_PORT_ATTR_PORT_VLAN_ID,
        sai_uint16_t,
        SaiIntDefault<sai_uint16_t>>;
    using Mtu = SaiAttribute<
        EnumType,
        SAI_PORT_ATTR_MTU,
        sai_uint32_t,
        SaiIntDefault<sai_uint32_t>>;
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
        SAI_PORT_ATTR_DISABLE_DECREMENT_TTL,
        bool,
        SaiBoolDefaultFalse>;
    using InterfaceType = SaiAttribute<
        EnumType,
        SAI_PORT_ATTR_INTERFACE_TYPE,
        sai_int32_t,
        SaiPortInterfaceTypeDefault>;
    using PktTxEnable = SaiAttribute<
        EnumType,
        SAI_PORT_ATTR_PKT_TX_ENABLE,
        bool,
        SaiBoolDefaultTrue>;
    using SerdesId = SaiAttribute<
        EnumType,
        SAI_PORT_ATTR_PORT_SERDES_ID,
        SaiObjectIdT,
        SaiObjectIdDefault>;
    using IngressMirrorSession = SaiAttribute<
        EnumType,
        SAI_PORT_ATTR_INGRESS_MIRROR_SESSION,
        std::vector<sai_object_id_t>,
        SaiObjectIdListDefault>;
    using EgressMirrorSession = SaiAttribute<
        EnumType,
        SAI_PORT_ATTR_EGRESS_MIRROR_SESSION,
        std::vector<sai_object_id_t>,
        SaiObjectIdListDefault>;
    using IngressSamplePacketEnable = SaiAttribute<
        EnumType,
        SAI_PORT_ATTR_INGRESS_SAMPLEPACKET_ENABLE,
        SaiObjectIdT,
        SaiObjectIdDefault>;
    using EgressSamplePacketEnable = SaiAttribute<
        EnumType,
        SAI_PORT_ATTR_EGRESS_SAMPLEPACKET_ENABLE,
        SaiObjectIdT,
        SaiObjectIdDefault>;
    using IngressSampleMirrorSession = SaiAttribute<
        EnumType,
        SAI_PORT_ATTR_INGRESS_SAMPLE_MIRROR_SESSION,
        std::vector<sai_object_id_t>,
        SaiObjectIdListDefault>;
    using EgressSampleMirrorSession = SaiAttribute<
        EnumType,
        SAI_PORT_ATTR_EGRESS_SAMPLE_MIRROR_SESSION,
        std::vector<sai_object_id_t>,
        SaiObjectIdListDefault>;
    using PrbsPolynomial =
        SaiAttribute<EnumType, SAI_PORT_ATTR_PRBS_POLYNOMIAL, sai_uint32_t>;
    using PrbsConfig =
        SaiAttribute<EnumType, SAI_PORT_ATTR_PRBS_CONFIG, sai_int32_t>;
#if SAI_API_VERSION >= SAI_VERSION(1, 8, 1)
    using PrbsRxState = SaiAttribute<
        EnumType,
        SAI_PORT_ATTR_PRBS_RX_STATE,
        sai_prbs_rx_state_t,
        SaiPrbsRxStateDefault>;
#endif
    using IngressMacSecAcl = SaiAttribute<
        EnumType,
        SAI_PORT_ATTR_INGRESS_MACSEC_ACL,
        SaiObjectIdT,
        SaiObjectIdDefault>;
    using EgressMacSecAcl = SaiAttribute<
        EnumType,
        SAI_PORT_ATTR_EGRESS_MACSEC_ACL,
        SaiObjectIdT,
        SaiObjectIdDefault>;
    struct AttributeSystemPortId {
      std::optional<sai_attr_id_t> operator()();
    };
    using SystemPortId =
        SaiExtensionAttribute<sai_uint16_t, AttributeSystemPortId>;
    using PtpMode = SaiAttribute<
        EnumType,
        SAI_PORT_ATTR_PTP_MODE,
        sai_int32_t,
        SaiIntDefault<sai_int32_t>>;
    using PortEyeValues = SaiAttribute<
        EnumType,
        SAI_PORT_ATTR_EYE_VALUES,
        std::vector<sai_port_lane_eye_values_t>,
        SaiPortEyeValuesDefault>;
    using PriorityFlowControlMode = SaiAttribute<
        EnumType,
        SAI_PORT_ATTR_PRIORITY_FLOW_CONTROL_MODE,
        sai_int32_t,
        SaiIntDefault<sai_int32_t>>;
    using PriorityFlowControl = SaiAttribute<
        EnumType,
        SAI_PORT_ATTR_PRIORITY_FLOW_CONTROL,
        sai_uint8_t,
        SaiIntDefault<sai_uint8_t>>;
#if !defined(TAJO_SDK)
    using PriorityFlowControlRx = SaiAttribute<
        EnumType,
        SAI_PORT_ATTR_PRIORITY_FLOW_CONTROL_RX,
        sai_uint8_t,
        SaiIntDefault<sai_uint8_t>>;
    using PriorityFlowControlTx = SaiAttribute<
        EnumType,
        SAI_PORT_ATTR_PRIORITY_FLOW_CONTROL_TX,
        sai_uint8_t,
        SaiIntDefault<sai_uint8_t>>;
#endif
    using PortErrStatus = SaiAttribute<
        EnumType,
        SAI_PORT_ATTR_ERR_STATUS_LIST,
        std::vector<sai_port_err_status_t>,
        SaiPortErrStatusDefault>;
    using IngressPriorityGroupList = SaiAttribute<
        EnumType,
        SAI_PORT_ATTR_INGRESS_PRIORITY_GROUP_LIST,
        std::vector<sai_object_id_t>,
        SaiObjectIdListDefault>;
    using NumberOfIngressPriorityGroups = SaiAttribute<
        EnumType,
        SAI_PORT_ATTR_NUMBER_OF_INGRESS_PRIORITY_GROUPS,
        sai_uint32_t,
        SaiIntDefault<sai_uint32_t>>;
    using QosTcToPriorityGroupMap = SaiAttribute<
        EnumType,
        SAI_PORT_ATTR_QOS_TC_TO_PRIORITY_GROUP_MAP,
        SaiObjectIdT,
        SaiObjectIdDefault>;
    using QosPfcPriorityToQueueMap = SaiAttribute<
        EnumType,
        SAI_PORT_ATTR_QOS_PFC_PRIORITY_TO_QUEUE_MAP,
        SaiObjectIdT,
        SaiObjectIdDefault>;
#if SAI_API_VERSION >= SAI_VERSION(1, 10, 3) || defined(TAJO_SDK_VERSION_1_42_8)
    using RxSignalDetect = SaiAttribute<
        EnumType,
        SAI_PORT_ATTR_RX_SIGNAL_DETECT,
        std::vector<sai_port_lane_latch_status_t>>;
    using RxLockStatus = SaiAttribute<
        EnumType,
        SAI_PORT_ATTR_RX_LOCK_STATUS,
        std::vector<sai_port_lane_latch_status_t>>;
    using FecAlignmentLock = SaiAttribute<
        EnumType,
        SAI_PORT_ATTR_FEC_ALIGNMENT_LOCK,
        std::vector<sai_port_lane_latch_status_t>>;
    using PcsRxLinkStatus = SaiAttribute<
        EnumType,
        SAI_PORT_ATTR_PCS_RX_LINK_STATUS,
        sai_latch_status_t>;
#endif
#if SAI_API_VERSION >= SAI_VERSION(1, 9, 0)
    using InterFrameGap = SaiAttribute<
        EnumType,
        SAI_PORT_ATTR_IPG,
        sai_uint32_t,
        SaiInt96Default>;
#endif
    using LinkTrainingEnable = SaiAttribute<
        EnumType,
        SAI_PORT_ATTR_LINK_TRAINING_ENABLE,
        bool,
        SaiBoolDefaultTrue>;
    using FabricAttached =
        SaiAttribute<EnumType, SAI_PORT_ATTR_FABRIC_ATTACHED, bool>;
    using FabricAttachedPortIndex = SaiAttribute<
        EnumType,
        SAI_PORT_ATTR_FABRIC_ATTACHED_PORT_INDEX,
        sai_uint32_t>;
    using FabricAttachedSwitchId = SaiAttribute<
        EnumType,
        SAI_PORT_ATTR_FABRIC_ATTACHED_SWITCH_ID,
        sai_uint32_t>;
    using FabricAttachedSwitchType = SaiAttribute<
        EnumType,
        SAI_PORT_ATTR_FABRIC_ATTACHED_SWITCH_TYPE,
        sai_uint32_t>;
  };
  using AdapterKey = PortSaiId;
  using AdapterHostKey = Attributes::HwLaneList;

  using CreateAttributes = std::tuple<
      Attributes::HwLaneList,
      Attributes::Speed,
      std::optional<Attributes::AdminState>,
      std::optional<Attributes::FecMode>,
#if SAI_API_VERSION >= SAI_VERSION(1, 10, 0)
      std::optional<Attributes::UseExtendedFec>,
      std::optional<Attributes::ExtendedFecMode>,
#endif
#if SAI_API_VERSION >= SAI_VERSION(1, 11, 0)
      std::optional<Attributes::FabricIsolate>,
#endif
      std::optional<Attributes::InternalLoopbackMode>,
      std::optional<Attributes::MediaType>,
      std::optional<Attributes::GlobalFlowControlMode>,
      std::optional<Attributes::PortVlanId>,
      std::optional<Attributes::Mtu>,
      std::optional<Attributes::QosDscpToTcMap>,
      std::optional<Attributes::QosTcToQueueMap>,
      std::optional<Attributes::DisableTtlDecrement>,
      std::optional<Attributes::InterfaceType>,
      std::optional<Attributes::PktTxEnable>,
      std::optional<Attributes::IngressMirrorSession>,
      std::optional<Attributes::EgressMirrorSession>,
      std::optional<Attributes::IngressSamplePacketEnable>,
      std::optional<Attributes::EgressSamplePacketEnable>,
      std::optional<Attributes::IngressSampleMirrorSession>,
      std::optional<Attributes::EgressSampleMirrorSession>,
      std::optional<Attributes::IngressMacSecAcl>,
      std::optional<Attributes::EgressMacSecAcl>,
      std::optional<Attributes::SystemPortId>,
      std::optional<Attributes::PtpMode>,
      std::optional<Attributes::PriorityFlowControlMode>,
      std::optional<Attributes::PriorityFlowControl>,
#if !defined(TAJO_SDK)
      std::optional<Attributes::PriorityFlowControlRx>,
      std::optional<Attributes::PriorityFlowControlTx>,
#endif
      std::optional<Attributes::QosTcToPriorityGroupMap>,
      std::optional<Attributes::QosPfcPriorityToQueueMap>,
#if SAI_API_VERSION >= SAI_VERSION(1, 9, 0)
      std::optional<Attributes::InterFrameGap>,
#endif
      std::optional<Attributes::LinkTrainingEnable>>;
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
  static constexpr std::array<sai_stat_id_t, 24> PfcCounterIdsToRead = {
      SAI_PORT_STAT_PFC_0_RX_PKTS,        SAI_PORT_STAT_PFC_1_RX_PKTS,
      SAI_PORT_STAT_PFC_2_RX_PKTS,        SAI_PORT_STAT_PFC_3_RX_PKTS,
      SAI_PORT_STAT_PFC_4_RX_PKTS,        SAI_PORT_STAT_PFC_5_RX_PKTS,
      SAI_PORT_STAT_PFC_6_RX_PKTS,        SAI_PORT_STAT_PFC_7_RX_PKTS,
      SAI_PORT_STAT_PFC_0_TX_PKTS,        SAI_PORT_STAT_PFC_1_TX_PKTS,
      SAI_PORT_STAT_PFC_2_TX_PKTS,        SAI_PORT_STAT_PFC_3_TX_PKTS,
      SAI_PORT_STAT_PFC_4_TX_PKTS,        SAI_PORT_STAT_PFC_5_TX_PKTS,
      SAI_PORT_STAT_PFC_6_TX_PKTS,        SAI_PORT_STAT_PFC_7_TX_PKTS,
      SAI_PORT_STAT_PFC_0_ON2OFF_RX_PKTS, SAI_PORT_STAT_PFC_1_ON2OFF_RX_PKTS,
      SAI_PORT_STAT_PFC_2_ON2OFF_RX_PKTS, SAI_PORT_STAT_PFC_3_ON2OFF_RX_PKTS,
      SAI_PORT_STAT_PFC_4_ON2OFF_RX_PKTS, SAI_PORT_STAT_PFC_5_ON2OFF_RX_PKTS,
      SAI_PORT_STAT_PFC_6_ON2OFF_RX_PKTS, SAI_PORT_STAT_PFC_7_ON2OFF_RX_PKTS,
  };
  static constexpr std::array<sai_stat_id_t, 0> CounterIdsToReadAndClear = {};
};

SAI_ATTRIBUTE_NAME(Port, HwLaneList)
SAI_ATTRIBUTE_NAME(Port, Speed)
SAI_ATTRIBUTE_NAME(Port, AdminState)
SAI_ATTRIBUTE_NAME(Port, FecMode)
SAI_ATTRIBUTE_NAME(Port, OperStatus)
SAI_ATTRIBUTE_NAME(Port, InternalLoopbackMode)
#if SAI_API_VERSION >= SAI_VERSION(1, 10, 0)
SAI_ATTRIBUTE_NAME(Port, PortLoopbackMode)
SAI_ATTRIBUTE_NAME(Port, UseExtendedFec)
SAI_ATTRIBUTE_NAME(Port, ExtendedFecMode)
#endif
#if SAI_API_VERSION >= SAI_VERSION(1, 11, 0)
SAI_ATTRIBUTE_NAME(Port, FabricIsolate)
#endif
SAI_ATTRIBUTE_NAME(Port, MediaType)
SAI_ATTRIBUTE_NAME(Port, GlobalFlowControlMode)
SAI_ATTRIBUTE_NAME(Port, PortVlanId)
SAI_ATTRIBUTE_NAME(Port, Mtu)
SAI_ATTRIBUTE_NAME(Port, QosDscpToTcMap)
SAI_ATTRIBUTE_NAME(Port, QosTcToQueueMap)
SAI_ATTRIBUTE_NAME(Port, DisableTtlDecrement)

SAI_ATTRIBUTE_NAME(Port, QosNumberOfQueues)
SAI_ATTRIBUTE_NAME(Port, QosQueueList)
SAI_ATTRIBUTE_NAME(Port, Type)
SAI_ATTRIBUTE_NAME(Port, InterfaceType)
SAI_ATTRIBUTE_NAME(Port, PktTxEnable)
SAI_ATTRIBUTE_NAME(Port, SerdesId)
SAI_ATTRIBUTE_NAME(Port, IngressMirrorSession)
SAI_ATTRIBUTE_NAME(Port, EgressMirrorSession)
SAI_ATTRIBUTE_NAME(Port, IngressSamplePacketEnable)
SAI_ATTRIBUTE_NAME(Port, EgressSamplePacketEnable)
SAI_ATTRIBUTE_NAME(Port, IngressSampleMirrorSession)
SAI_ATTRIBUTE_NAME(Port, EgressSampleMirrorSession)

SAI_ATTRIBUTE_NAME(Port, PrbsPolynomial)
SAI_ATTRIBUTE_NAME(Port, PrbsConfig)
#if SAI_API_VERSION >= SAI_VERSION(1, 8, 1)
SAI_ATTRIBUTE_NAME(Port, PrbsRxState)
#endif
SAI_ATTRIBUTE_NAME(Port, IngressMacSecAcl)
SAI_ATTRIBUTE_NAME(Port, EgressMacSecAcl)
SAI_ATTRIBUTE_NAME(Port, SystemPortId)
SAI_ATTRIBUTE_NAME(Port, PtpMode)
SAI_ATTRIBUTE_NAME(Port, PortEyeValues)
SAI_ATTRIBUTE_NAME(Port, PriorityFlowControlMode)
SAI_ATTRIBUTE_NAME(Port, PriorityFlowControl)
#if !defined(TAJO_SDK)
SAI_ATTRIBUTE_NAME(Port, PriorityFlowControlRx)
SAI_ATTRIBUTE_NAME(Port, PriorityFlowControlTx)
#endif
SAI_ATTRIBUTE_NAME(Port, PortErrStatus)
SAI_ATTRIBUTE_NAME(Port, IngressPriorityGroupList)
SAI_ATTRIBUTE_NAME(Port, NumberOfIngressPriorityGroups)
SAI_ATTRIBUTE_NAME(Port, QosTcToPriorityGroupMap)
SAI_ATTRIBUTE_NAME(Port, QosPfcPriorityToQueueMap)
#if SAI_API_VERSION >= SAI_VERSION(1, 10, 3) || defined(TAJO_SDK_VERSION_1_42_8)
SAI_ATTRIBUTE_NAME(Port, RxSignalDetect)
SAI_ATTRIBUTE_NAME(Port, RxLockStatus)
SAI_ATTRIBUTE_NAME(Port, FecAlignmentLock)
SAI_ATTRIBUTE_NAME(Port, PcsRxLinkStatus)
#endif
#if SAI_API_VERSION >= SAI_VERSION(1, 9, 0)
SAI_ATTRIBUTE_NAME(Port, InterFrameGap)
#endif
SAI_ATTRIBUTE_NAME(Port, LinkTrainingEnable)
SAI_ATTRIBUTE_NAME(Port, SerdesLaneList)
SAI_ATTRIBUTE_NAME(Port, DiagModeEnable)
SAI_ATTRIBUTE_NAME(Port, FabricAttached);
SAI_ATTRIBUTE_NAME(Port, FabricAttachedPortIndex);
SAI_ATTRIBUTE_NAME(Port, FabricAttachedSwitchId);
SAI_ATTRIBUTE_NAME(Port, FabricAttachedSwitchType);

template <>
struct SaiObjectHasStats<SaiPortTraits> : public std::true_type {};

struct SaiPortSerdesTraits {
  static constexpr sai_object_type_t ObjectType = SAI_OBJECT_TYPE_PORT_SERDES;
  using SaiApiT = PortApi;
  struct Attributes {
    using EnumType = sai_port_serdes_attr_t;
    using PortId =
        SaiAttribute<EnumType, SAI_PORT_SERDES_ATTR_PORT_ID, SaiObjectIdT>;
    using Preemphasis = SaiAttribute<
        EnumType,
        SAI_PORT_SERDES_ATTR_PREEMPHASIS,
        std::vector<uint32_t>,
        SaiU32ListDefault>;
    using IDriver = SaiAttribute<
        EnumType,
        SAI_PORT_SERDES_ATTR_IDRIVER,
        std::vector<sai_uint32_t>,
        SaiU32ListDefault>;
    using TxFirPre1 = SaiAttribute<
        EnumType,
        SAI_PORT_SERDES_ATTR_TX_FIR_PRE1,
        std::vector<sai_uint32_t>,
        SaiU32ListDefault>;
    using TxFirPre2 = SaiAttribute<
        EnumType,
        SAI_PORT_SERDES_ATTR_TX_FIR_PRE2,
        std::vector<sai_uint32_t>,
        SaiU32ListDefault>;
    using TxFirMain = SaiAttribute<
        EnumType,
        SAI_PORT_SERDES_ATTR_TX_FIR_MAIN,
        std::vector<sai_uint32_t>,
        SaiU32ListDefault>;
    using TxFirPost1 = SaiAttribute<
        EnumType,
        SAI_PORT_SERDES_ATTR_TX_FIR_POST1,
        std::vector<sai_uint32_t>,
        SaiU32ListDefault>;
    using TxFirPost2 = SaiAttribute<
        EnumType,
        SAI_PORT_SERDES_ATTR_TX_FIR_POST2,
        std::vector<sai_uint32_t>,
        SaiU32ListDefault>;
    using TxFirPost3 = SaiAttribute<
        EnumType,
        SAI_PORT_SERDES_ATTR_TX_FIR_POST3,
        std::vector<sai_uint32_t>,
        SaiU32ListDefault>;

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

    struct AttributeRxAfeAdaptiveEnableWrapper {
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
    using RxAfeAdaptiveEnable = SaiExtensionAttribute<
        std::vector<sai_int32_t>,
        AttributeRxAfeAdaptiveEnableWrapper>;
  };
  using AdapterKey = PortSerdesSaiId;
  using AdapterHostKey = Attributes::PortId;
  using CreateAttributes = std::tuple<
      Attributes::PortId,
      std::optional<Attributes::Preemphasis>,
      std::optional<Attributes::IDriver>,
      std::optional<Attributes::TxFirPre1>,
#if SAI_API_VERSION >= SAI_VERSION(1, 10, 0)
      std::optional<Attributes::TxFirPre2>,
#endif
      std::optional<Attributes::TxFirMain>,
      std::optional<Attributes::TxFirPost1>,
#if SAI_API_VERSION >= SAI_VERSION(1, 10, 0)
      std::optional<Attributes::TxFirPost2>,
      std::optional<Attributes::TxFirPost3>,
#endif
      std::optional<Attributes::RxCtleCode>,
      std::optional<Attributes::RxDspMode>,
      std::optional<Attributes::RxAfeTrim>,
      std::optional<Attributes::RxAcCouplingByPass>,
      std::optional<Attributes::RxAfeAdaptiveEnable>>;
};

SAI_ATTRIBUTE_NAME(PortSerdes, PortId);
SAI_ATTRIBUTE_NAME(PortSerdes, Preemphasis);
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
SAI_ATTRIBUTE_NAME(PortSerdes, RxAfeAdaptiveEnable);

struct SaiPortConnectorTraits {
  static constexpr sai_object_type_t ObjectType =
      SAI_OBJECT_TYPE_PORT_CONNECTOR;
  using SaiApiT = PortApi;
  struct Attributes {
    using EnumType = sai_port_connector_attr_t;
    using LineSidePortId = SaiAttribute<
        EnumType,
        SAI_PORT_CONNECTOR_ATTR_LINE_SIDE_PORT_ID,
        SaiObjectIdT>;
    using SystemSidePortId = SaiAttribute<
        EnumType,
        SAI_PORT_CONNECTOR_ATTR_SYSTEM_SIDE_PORT_ID,
        SaiObjectIdT>;
  };
  using AdapterKey = PortConnectorSaiId;
  using AdapterHostKey =
      std::tuple<Attributes::LineSidePortId, Attributes::SystemSidePortId>;
  using CreateAttributes = AdapterHostKey;
};

SAI_ATTRIBUTE_NAME(PortConnector, LineSidePortId);
SAI_ATTRIBUTE_NAME(PortConnector, SystemSidePortId);

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
  sai_status_t _remove(PortSaiId key) const {
    return api_->remove_port(key);
  }
  sai_status_t _getAttribute(PortSaiId key, sai_attribute_t* attr) const {
    return api_->get_port_attribute(key, 1, attr);
  }
  sai_status_t _setAttribute(PortSaiId key, const sai_attribute_t* attr) const {
    return api_->set_port_attribute(key, attr);
  }

  sai_status_t _create(
      PortSerdesSaiId* id,
      sai_object_id_t switch_id,
      size_t count,
      sai_attribute_t* attr_list) const {
    return api_->create_port_serdes(rawSaiId(id), switch_id, count, attr_list);
  }

  sai_status_t _remove(PortSerdesSaiId id) const {
    return api_->remove_port_serdes(id);
  }

  sai_status_t _getAttribute(PortSerdesSaiId key, sai_attribute_t* attr) const {
    return api_->get_port_serdes_attribute(key, 1, attr);
  }

  sai_status_t _setAttribute(PortSerdesSaiId key, const sai_attribute_t* attr)
      const {
    return api_->set_port_serdes_attribute(key, attr);
  }

  sai_status_t _create(
      PortConnectorSaiId* id,
      sai_object_id_t switch_id,
      size_t count,
      sai_attribute_t* attr_list) const {
    return api_->create_port_connector(
        rawSaiId(id), switch_id, count, attr_list);
  }

  sai_status_t _remove(PortConnectorSaiId id) const {
    return api_->remove_port_connector(id);
  }

  sai_status_t _getAttribute(PortConnectorSaiId key, sai_attribute_t* attr)
      const {
    return api_->get_port_connector_attribute(key, 1, attr);
  }

  sai_status_t _setAttribute(
      PortConnectorSaiId key,
      const sai_attribute_t* attr) const {
    return api_->set_port_connector_attribute(key, attr);
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
