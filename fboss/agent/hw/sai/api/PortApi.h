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

#include <limits>
#include <optional>
#include <tuple>

extern "C" {
#include <sai.h>
}

bool operator==(const sai_map_t& lhs, const sai_map_t& rhs);
bool operator!=(const sai_map_t& lhs, const sai_map_t& rhs);

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
#if SAI_API_VERSION >= SAI_VERSION(1, 10, 3)
    struct AttributeCrcErrorDetect {
      std::optional<sai_attr_id_t> operator()();
    };
    using CrcErrorDetect =
        SaiExtensionAttribute<sai_latch_status_t, AttributeCrcErrorDetect>;
#endif
    struct AttributeFdrEnable {
      std::optional<sai_attr_id_t> operator()();
    };
    using FdrEnable =
        SaiExtensionAttribute<bool, AttributeFdrEnable, SaiBoolDefaultFalse>;
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
    using FabricIsolate = SaiAttribute<
        EnumType,
        SAI_PORT_ATTR_FABRIC_ISOLATE,
        bool,
        SaiBoolDefaultFalse>;
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
    using PrbsPolynomial = SaiAttribute<
        EnumType,
        SAI_PORT_ATTR_PRBS_POLYNOMIAL,
        sai_uint32_t,
        SaiIntDefault<sai_uint32_t>>;
    using PrbsConfig = SaiAttribute<
        EnumType,
        SAI_PORT_ATTR_PRBS_CONFIG,
        sai_int32_t,
        SaiPrbsConfigDefault>;
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
#if SAI_API_VERSION >= SAI_VERSION(1, 13, 0)
    /*
     * PPM = Parts per million
     * RX_FREQUENCY_OFFSET_PPM => amount that a receiver serdes has to
     * compensate for clock differences from the remote side
     */
    using RxFrequencyPPM = SaiAttribute<
        EnumType,
        SAI_PORT_ATTR_RX_FREQUENCY_OFFSET_PPM,
        std::vector<sai_port_frequency_offset_ppm_values_t>>;
    using RxSNR = SaiAttribute<
        EnumType,
        SAI_PORT_ATTR_RX_SNR,
        std::vector<sai_port_snr_values_t>>;
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
    using FabricReachability = SaiAttribute<
        EnumType,
        SAI_PORT_ATTR_FABRIC_REACHABILITY,
        sai_fabric_port_reachability_t>;

    struct AttributeRxLaneSquelchEnable {
      std::optional<sai_attr_id_t> operator()();
    };
    using RxLaneSquelchEnable =
        SaiExtensionAttribute<bool, AttributeRxLaneSquelchEnable>;
#if SAI_API_VERSION >= SAI_VERSION(1, 10, 2)
    using PfcTcDldInterval = SaiAttribute<
        EnumType,
        SAI_PORT_ATTR_PFC_TC_DLD_INTERVAL,
        std::vector<sai_map_t>,
        SaiListDefault<sai_map_list_t>>;
    using PfcTcDlrInterval = SaiAttribute<
        EnumType,
        SAI_PORT_ATTR_PFC_TC_DLR_INTERVAL,
        std::vector<sai_map_t>,
        SaiListDefault<sai_map_list_t>>;
    using PfcTcDldIntervalRange = SaiAttribute<
        EnumType,
        SAI_PORT_ATTR_PFC_TC_DLD_INTERVAL_RANGE,
        sai_u32_range_t,
        SaiIntRangeDefault<sai_u32_range_t>>;
    using PfcTcDlrIntervalRange = SaiAttribute<
        EnumType,
        SAI_PORT_ATTR_PFC_TC_DLR_INTERVAL_RANGE,
        sai_u32_range_t,
        SaiIntRangeDefault<sai_u32_range_t>>;
#endif
    using SystemPort = SaiAttribute<
        EnumType,
        SAI_PORT_ATTR_SYSTEM_PORT,
        SaiObjectIdT,
        SaiObjectIdDefault>;
#if SAI_API_VERSION >= SAI_VERSION(1, 13, 0)
    using TxReadyStatus =
        SaiAttribute<EnumType, SAI_PORT_ATTR_HOST_TX_READY_STATUS, sai_int32_t>;
#endif
#if SAI_API_VERSION >= SAI_VERSION(1, 14, 0)
    using ArsEnable = SaiAttribute<
        EnumType,
        SAI_PORT_ATTR_ARS_ENABLE,
        bool,
        SaiBoolDefaultFalse>;
    using ArsPortLoadScalingFactor = SaiAttribute<
        EnumType,
        SAI_PORT_ATTR_ARS_PORT_LOAD_SCALING_FACTOR,
        sai_uint32_t,
        SaiIntDefault<sai_uint32_t>>;
    using ArsPortLoadPastWeight = SaiAttribute<
        EnumType,
        SAI_PORT_ATTR_ARS_PORT_LOAD_PAST_WEIGHT,
        sai_uint32_t,
        SaiIntDefault<sai_uint32_t>>;
    using ArsPortLoadFutureWeight = SaiAttribute<
        EnumType,
        SAI_PORT_ATTR_ARS_PORT_LOAD_FUTURE_WEIGHT,
        sai_uint32_t,
        SaiIntDefault<sai_uint32_t>>;
#endif
    struct AttributeCablePropogationDelayNS {
      std::optional<sai_attr_id_t> operator()();
    };
    using CablePropogationDelayNS = SaiExtensionAttribute<
        sai_uint32_t,
        AttributeCablePropogationDelayNS,
        SaiIntValueDefault<uint32_t, std::numeric_limits<uint32_t>::max()>>;
    struct AttributeFabricDataCellsFilterStatus {
      std::optional<sai_attr_id_t> operator()();
    };
    using FabricDataCellsFilterStatus = SaiExtensionAttribute<
        bool,
        AttributeFabricDataCellsFilterStatus,
        SaiBoolDefaultFalse>;
    struct AttributeReachabilityGroup {
      std::optional<sai_attr_id_t> operator()();
    };
    using ReachabilityGroup = SaiExtensionAttribute<
        sai_uint32_t,
        AttributeReachabilityGroup,
        SaiIntDefault<sai_uint32_t>>;
  };
  using AdapterKey = PortSaiId;

  /*
   * On some platforms:
   *  - HwLane values are unique only for given Port Type.
   *  - For example, LOGICAL (NIF) and Fabric port may carry same HwLane id.
   *  - HwLaneList is thus insufficient to unambiguously create ports.
   *  - Instead, port create needs Port Type, HwLaneList (and speed).
   *
   * However, the current SAI Spec (v1.12) defines Port Type as Read Only
   * attribute:
   * https://github.com/opencomputeproject/SAI/blob/v1.12.0/inc/saiport.h#L496-L511
   *
   * In future, we will look to enhance the SAI spec to support Port Type attr
   * during creation. In the meantime, use Port Type as part of the
   * AdapterHostKey, CreateAttributes for such platforms.
   *
   * Note:
   *   - Enhancing AdapterHostKey as well as CreateAttribute means that the
   *     AdatperHostKey remains computable from AdaterKey, CreateAttributes: a
   *     requirement for our design.
   *   - However, this also means that attempt to create port will pass Type (a
   *     Read Only attribute today) and that call will fail.
   *   - This is not a problem at the moment since passing Type is a
   *     requirement only on DNX and ports are always pre-created on this
   *     platform and loaded by SaiStore during init and never created.
   *   - In future, if we support breakout ports on DNX, we would need to call
   *     create port which would fail: but as mentioned above, we plan to
   *     enhance the SAI spec and work with SAI vendors to support passing port
   *     type during creation.
   */

  using AdapterHostKey = std::tuple<
#if defined(BRCM_SAI_SDK_DNX)
      Attributes::Type,
#endif
      Attributes::HwLaneList>;

  using CreateAttributes = std::tuple<
#if defined(BRCM_SAI_SDK_DNX)
      Attributes::Type,
#endif
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
#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)
      std::optional<Attributes::PortLoopbackMode>,
#else
      std::optional<Attributes::InternalLoopbackMode>,
#endif
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
      std::optional<Attributes::PrbsPolynomial>,
      std::optional<Attributes::PrbsConfig>,
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
      std::optional<Attributes::LinkTrainingEnable>,
      std::optional<Attributes::FdrEnable>,
      std::optional<Attributes::RxLaneSquelchEnable>,
#if SAI_API_VERSION >= SAI_VERSION(1, 10, 2)
      std::optional<Attributes::PfcTcDldInterval>,
      std::optional<Attributes::PfcTcDlrInterval>,
#endif
#if SAI_API_VERSION >= SAI_VERSION(1, 14, 0)
      std::optional<Attributes::ArsEnable>,
      std::optional<Attributes::ArsPortLoadScalingFactor>,
      std::optional<Attributes::ArsPortLoadPastWeight>,
      std::optional<Attributes::ArsPortLoadFutureWeight>,
#endif
      std::optional<Attributes::ReachabilityGroup>>;
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
  static constexpr std::array<sai_stat_id_t, 16> PfcCounterIdsToRead = {
      SAI_PORT_STAT_PFC_0_RX_PKTS,
      SAI_PORT_STAT_PFC_1_RX_PKTS,
      SAI_PORT_STAT_PFC_2_RX_PKTS,
      SAI_PORT_STAT_PFC_3_RX_PKTS,
      SAI_PORT_STAT_PFC_4_RX_PKTS,
      SAI_PORT_STAT_PFC_5_RX_PKTS,
      SAI_PORT_STAT_PFC_6_RX_PKTS,
      SAI_PORT_STAT_PFC_7_RX_PKTS,
      SAI_PORT_STAT_PFC_0_TX_PKTS,
      SAI_PORT_STAT_PFC_1_TX_PKTS,
      SAI_PORT_STAT_PFC_2_TX_PKTS,
      SAI_PORT_STAT_PFC_3_TX_PKTS,
      SAI_PORT_STAT_PFC_4_TX_PKTS,
      SAI_PORT_STAT_PFC_5_TX_PKTS,
      SAI_PORT_STAT_PFC_6_TX_PKTS,
      SAI_PORT_STAT_PFC_7_TX_PKTS,
  };
  static constexpr std::array<sai_stat_id_t, 8> PfcXonToXoffCounterIdsToRead = {
      SAI_PORT_STAT_PFC_0_ON2OFF_RX_PKTS,
      SAI_PORT_STAT_PFC_1_ON2OFF_RX_PKTS,
      SAI_PORT_STAT_PFC_2_ON2OFF_RX_PKTS,
      SAI_PORT_STAT_PFC_3_ON2OFF_RX_PKTS,
      SAI_PORT_STAT_PFC_4_ON2OFF_RX_PKTS,
      SAI_PORT_STAT_PFC_5_ON2OFF_RX_PKTS,
      SAI_PORT_STAT_PFC_6_ON2OFF_RX_PKTS,
      SAI_PORT_STAT_PFC_7_ON2OFF_RX_PKTS,
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
#if SAI_API_VERSION >= SAI_VERSION(1, 13, 0)
SAI_ATTRIBUTE_NAME(Port, RxFrequencyPPM)
SAI_ATTRIBUTE_NAME(Port, RxSNR)
#endif
SAI_ATTRIBUTE_NAME(Port, LinkTrainingEnable)
SAI_ATTRIBUTE_NAME(Port, SerdesLaneList)
SAI_ATTRIBUTE_NAME(Port, DiagModeEnable)
SAI_ATTRIBUTE_NAME(Port, FdrEnable)
#if SAI_API_VERSION >= SAI_VERSION(1, 10, 3)
SAI_ATTRIBUTE_NAME(Port, CrcErrorDetect)
#endif
SAI_ATTRIBUTE_NAME(Port, FabricAttached);
SAI_ATTRIBUTE_NAME(Port, FabricAttachedPortIndex);
SAI_ATTRIBUTE_NAME(Port, FabricAttachedSwitchId);
SAI_ATTRIBUTE_NAME(Port, FabricAttachedSwitchType);
SAI_ATTRIBUTE_NAME(Port, FabricReachability);
SAI_ATTRIBUTE_NAME(Port, RxLaneSquelchEnable);
#if SAI_API_VERSION >= SAI_VERSION(1, 10, 2)
SAI_ATTRIBUTE_NAME(Port, PfcTcDldInterval);
SAI_ATTRIBUTE_NAME(Port, PfcTcDlrInterval);
SAI_ATTRIBUTE_NAME(Port, PfcTcDldIntervalRange);
SAI_ATTRIBUTE_NAME(Port, PfcTcDlrIntervalRange);
#endif
SAI_ATTRIBUTE_NAME(Port, SystemPort);
#if SAI_API_VERSION >= SAI_VERSION(1, 13, 0)
SAI_ATTRIBUTE_NAME(Port, TxReadyStatus)
#endif
SAI_ATTRIBUTE_NAME(Port, CablePropogationDelayNS)
SAI_ATTRIBUTE_NAME(Port, FabricDataCellsFilterStatus)
#if SAI_API_VERSION >= SAI_VERSION(1, 14, 0)
SAI_ATTRIBUTE_NAME(Port, ArsEnable)
SAI_ATTRIBUTE_NAME(Port, ArsPortLoadScalingFactor)
SAI_ATTRIBUTE_NAME(Port, ArsPortLoadPastWeight)
SAI_ATTRIBUTE_NAME(Port, ArsPortLoadFutureWeight)
#endif
SAI_ATTRIBUTE_NAME(Port, ReachabilityGroup)

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
    using TxFirPre2 = SaiAttribute<
        EnumType,
        SAI_PORT_SERDES_ATTR_TX_FIR_PRE2,
        std::vector<sai_uint32_t>,
        SaiU32ListDefault>;
    using TxFirPre3 = SaiAttribute<
        EnumType,
        SAI_PORT_SERDES_ATTR_TX_FIR_PRE3,
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
    struct AttributeTxLutModeIdWrapper {
      std::optional<sai_attr_id_t> operator()();
    };
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
    using TxLutMode = SaiExtensionAttribute<
        std::vector<sai_int32_t>,
        AttributeTxLutModeIdWrapper>;
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
    // Tx Attributes
    struct AttributeTxDiffEncoderEnWrapper {
      std::optional<sai_attr_id_t> operator()();
    };
    struct AttributeTxDigGainWrapper {
      std::optional<sai_attr_id_t> operator()();
    };
    struct AttributeTxFfeCoeff0Wrapper {
      std::optional<sai_attr_id_t> operator()();
    };
    struct AttributeTxFfeCoeff1Wrapper {
      std::optional<sai_attr_id_t> operator()();
    };
    struct AttributeTxFfeCoeff2Wrapper {
      std::optional<sai_attr_id_t> operator()();
    };
    struct AttributeTxFfeCoeff3Wrapper {
      std::optional<sai_attr_id_t> operator()();
    };
    struct AttributeTxFfeCoeff4Wrapper {
      std::optional<sai_attr_id_t> operator()();
    };
    struct AttributeTxDriverSwingWrapper {
      std::optional<sai_attr_id_t> operator()();
    };

    // Rx Attributes
    struct AttributeRxInstgBoost1StartWrapper {
      std::optional<sai_attr_id_t> operator()();
    };
    struct AttributeRxInstgBoost1StepWrapper {
      std::optional<sai_attr_id_t> operator()();
    };
    struct AttributeRxInstgBoost1StopWrapper {
      std::optional<sai_attr_id_t> operator()();
    };
    struct AttributeRxInstgBoost2OrHrStartWrapper {
      std::optional<sai_attr_id_t> operator()();
    };
    struct AttributeRxInstgBoost2OrHrStepWrapper {
      std::optional<sai_attr_id_t> operator()();
    };
    struct AttributeRxInstgBoost2OrHrStopWrapper {
      std::optional<sai_attr_id_t> operator()();
    };
    struct AttributeRxInstgC1Start1p7Wrapper {
      std::optional<sai_attr_id_t> operator()();
    };
    struct AttributeRxInstgC1Step1p7Wrapper {
      std::optional<sai_attr_id_t> operator()();
    };
    struct AttributeRxInstgC1Stop1p7Wrapper {
      std::optional<sai_attr_id_t> operator()();
    };
    struct AttributeRxInstgDfeStart1p7Wrapper {
      std::optional<sai_attr_id_t> operator()();
    };
    struct AttributeRxInstgDfeStep1p7Wrapper {
      std::optional<sai_attr_id_t> operator()();
    };
    struct AttributeRxInstgDfeStop1p7Wrapper {
      std::optional<sai_attr_id_t> operator()();
    };
    struct AttributeRxEnableScanSelectionWrapper {
      std::optional<sai_attr_id_t> operator()();
    };
    struct AttributeRxInstgScanUseSrSettingsWrapper {
      std::optional<sai_attr_id_t> operator()();
    };
    struct AttributeRxCdrCfgOvEnWrapper {
      std::optional<sai_attr_id_t> operator()();
    };
    struct AttributeRxCdrTdet1stOrdStepOvValWrapper {
      std::optional<sai_attr_id_t> operator()();
    };
    struct AttributeRxCdrTdet2ndOrdStepOvValWrapper {
      std::optional<sai_attr_id_t> operator()();
    };
    struct AttributeRxCdrTdetFineStepOvValWrapper {
      std::optional<sai_attr_id_t> operator()();
    };

    using TxDiffEncoderEn = SaiExtensionAttribute<
        std::vector<sai_int32_t>,
        AttributeTxDiffEncoderEnWrapper>;
    using TxDigGain = SaiExtensionAttribute<
        std::vector<sai_int32_t>,
        AttributeTxDigGainWrapper>;
    using TxFfeCoeff0 = SaiExtensionAttribute<
        std::vector<sai_int32_t>,
        AttributeTxFfeCoeff0Wrapper>;
    using TxFfeCoeff1 = SaiExtensionAttribute<
        std::vector<sai_int32_t>,
        AttributeTxFfeCoeff1Wrapper>;
    using TxFfeCoeff2 = SaiExtensionAttribute<
        std::vector<sai_int32_t>,
        AttributeTxFfeCoeff2Wrapper>;
    using TxFfeCoeff3 = SaiExtensionAttribute<
        std::vector<sai_int32_t>,
        AttributeTxFfeCoeff3Wrapper>;
    using TxFfeCoeff4 = SaiExtensionAttribute<
        std::vector<sai_int32_t>,
        AttributeTxFfeCoeff4Wrapper>;
    using TxDriverSwing = SaiExtensionAttribute<
        std::vector<sai_int32_t>,
        AttributeTxDriverSwingWrapper>;

    using RxInstgBoost1Start = SaiExtensionAttribute<
        std::vector<sai_int32_t>,
        AttributeRxInstgBoost1StartWrapper>;
    using RxInstgBoost1Step = SaiExtensionAttribute<
        std::vector<sai_int32_t>,
        AttributeRxInstgBoost1StepWrapper>;
    using RxInstgBoost1Stop = SaiExtensionAttribute<
        std::vector<sai_int32_t>,
        AttributeRxInstgBoost1StopWrapper>;
    using RxInstgBoost2OrHrStart = SaiExtensionAttribute<
        std::vector<sai_int32_t>,
        AttributeRxInstgBoost2OrHrStartWrapper>;
    using RxInstgBoost2OrHrStep = SaiExtensionAttribute<
        std::vector<sai_int32_t>,
        AttributeRxInstgBoost2OrHrStepWrapper>;
    using RxInstgBoost2OrHrStop = SaiExtensionAttribute<
        std::vector<sai_int32_t>,
        AttributeRxInstgBoost2OrHrStopWrapper>;
    using RxInstgC1Start1p7 = SaiExtensionAttribute<
        std::vector<sai_int32_t>,
        AttributeRxInstgC1Start1p7Wrapper>;
    using RxInstgC1Step1p7 = SaiExtensionAttribute<
        std::vector<sai_int32_t>,
        AttributeRxInstgC1Step1p7Wrapper>;
    using RxInstgC1Stop1p7 = SaiExtensionAttribute<
        std::vector<sai_int32_t>,
        AttributeRxInstgC1Stop1p7Wrapper>;
    using RxInstgDfeStart1p7 = SaiExtensionAttribute<
        std::vector<sai_int32_t>,
        AttributeRxInstgDfeStart1p7Wrapper>;
    using RxInstgDfeStep1p7 = SaiExtensionAttribute<
        std::vector<sai_int32_t>,
        AttributeRxInstgDfeStep1p7Wrapper>;
    using RxInstgDfeStop1p7 = SaiExtensionAttribute<
        std::vector<sai_int32_t>,
        AttributeRxInstgDfeStop1p7Wrapper>;
    using RxEnableScanSelection = SaiExtensionAttribute<
        std::vector<sai_int32_t>,
        AttributeRxEnableScanSelectionWrapper>;
    using RxInstgScanUseSrSettings = SaiExtensionAttribute<
        std::vector<sai_int32_t>,
        AttributeRxInstgScanUseSrSettingsWrapper>;
    using RxCdrCfgOvEn = SaiExtensionAttribute<
        std::vector<sai_int32_t>,
        AttributeRxCdrCfgOvEnWrapper>;
    using RxCdrTdet1stOrdStepOvVal = SaiExtensionAttribute<
        std::vector<sai_int32_t>,
        AttributeRxCdrTdet1stOrdStepOvValWrapper>;
    using RxCdrTdet2ndOrdStepOvVal = SaiExtensionAttribute<
        std::vector<sai_int32_t>,
        AttributeRxCdrTdet2ndOrdStepOvValWrapper>;
    using RxCdrTdetFineStepOvVal = SaiExtensionAttribute<
        std::vector<sai_int32_t>,
        AttributeRxCdrTdetFineStepOvValWrapper>;
  };
  using AdapterKey = PortSerdesSaiId;
  using AdapterHostKey = Attributes::PortId;
  using CreateAttributes = std::tuple<
      Attributes::PortId,
      std::optional<Attributes::Preemphasis>,

      std::optional<Attributes::IDriver>,
      std::optional<Attributes::TxFirPre1>,
      std::optional<Attributes::TxFirPre2>,
      std::optional<Attributes::TxFirPre3>,
      std::optional<Attributes::TxFirMain>,
      std::optional<Attributes::TxFirPost1>,
      std::optional<Attributes::TxFirPost2>,
      std::optional<Attributes::TxFirPost3>,
      std::optional<Attributes::TxLutMode>,
      std::optional<Attributes::RxCtleCode>,
      std::optional<Attributes::RxDspMode>,
      std::optional<Attributes::RxAfeTrim>,
      std::optional<Attributes::RxAcCouplingByPass>,
      std::optional<Attributes::RxAfeAdaptiveEnable>,
      std::optional<Attributes::TxDiffEncoderEn>,
      std::optional<Attributes::TxDigGain>,
      std::optional<Attributes::TxFfeCoeff0>,
      std::optional<Attributes::TxFfeCoeff1>,
      std::optional<Attributes::TxFfeCoeff2>,
      std::optional<Attributes::TxFfeCoeff3>,
      std::optional<Attributes::TxFfeCoeff4>,
      std::optional<Attributes::TxDriverSwing>,
      std::optional<Attributes::RxInstgBoost1Start>,
      std::optional<Attributes::RxInstgBoost1Step>,
      std::optional<Attributes::RxInstgBoost1Stop>,
      std::optional<Attributes::RxInstgBoost2OrHrStart>,
      std::optional<Attributes::RxInstgBoost2OrHrStep>,
      std::optional<Attributes::RxInstgBoost2OrHrStop>,
      std::optional<Attributes::RxInstgC1Start1p7>,
      std::optional<Attributes::RxInstgC1Step1p7>,
      std::optional<Attributes::RxInstgC1Stop1p7>,
      std::optional<Attributes::RxInstgDfeStart1p7>,
      std::optional<Attributes::RxInstgDfeStep1p7>,
      std::optional<Attributes::RxInstgDfeStop1p7>,
      std::optional<Attributes::RxEnableScanSelection>,
      std::optional<Attributes::RxInstgScanUseSrSettings>,
      std::optional<Attributes::RxCdrCfgOvEn>,
      std::optional<Attributes::RxCdrTdet1stOrdStepOvVal>,
      std::optional<Attributes::RxCdrTdet2ndOrdStepOvVal>,
      std::optional<Attributes::RxCdrTdetFineStepOvVal>>;
};

SAI_ATTRIBUTE_NAME(PortSerdes, PortId);
SAI_ATTRIBUTE_NAME(PortSerdes, Preemphasis);
SAI_ATTRIBUTE_NAME(PortSerdes, IDriver);
SAI_ATTRIBUTE_NAME(PortSerdes, TxFirPre1);
SAI_ATTRIBUTE_NAME(PortSerdes, TxFirPre2);
SAI_ATTRIBUTE_NAME(PortSerdes, TxFirPre3);
SAI_ATTRIBUTE_NAME(PortSerdes, TxFirMain);
SAI_ATTRIBUTE_NAME(PortSerdes, TxFirPost1);
SAI_ATTRIBUTE_NAME(PortSerdes, TxFirPost2);
SAI_ATTRIBUTE_NAME(PortSerdes, TxFirPost3);
SAI_ATTRIBUTE_NAME(PortSerdes, TxLutMode);
SAI_ATTRIBUTE_NAME(PortSerdes, RxCtleCode);
SAI_ATTRIBUTE_NAME(PortSerdes, RxDspMode);
SAI_ATTRIBUTE_NAME(PortSerdes, RxAfeTrim);
SAI_ATTRIBUTE_NAME(PortSerdes, RxAcCouplingByPass);
SAI_ATTRIBUTE_NAME(PortSerdes, RxAfeAdaptiveEnable);
SAI_ATTRIBUTE_NAME(PortSerdes, TxDiffEncoderEn);
SAI_ATTRIBUTE_NAME(PortSerdes, TxDigGain);
SAI_ATTRIBUTE_NAME(PortSerdes, TxFfeCoeff0);
SAI_ATTRIBUTE_NAME(PortSerdes, TxFfeCoeff1);
SAI_ATTRIBUTE_NAME(PortSerdes, TxFfeCoeff2);
SAI_ATTRIBUTE_NAME(PortSerdes, TxFfeCoeff3);
SAI_ATTRIBUTE_NAME(PortSerdes, TxFfeCoeff4);
SAI_ATTRIBUTE_NAME(PortSerdes, TxDriverSwing);
SAI_ATTRIBUTE_NAME(PortSerdes, RxInstgBoost1Start);
SAI_ATTRIBUTE_NAME(PortSerdes, RxInstgBoost1Step);
SAI_ATTRIBUTE_NAME(PortSerdes, RxInstgBoost1Stop);
SAI_ATTRIBUTE_NAME(PortSerdes, RxInstgBoost2OrHrStart);
SAI_ATTRIBUTE_NAME(PortSerdes, RxInstgBoost2OrHrStep);
SAI_ATTRIBUTE_NAME(PortSerdes, RxInstgBoost2OrHrStop);
SAI_ATTRIBUTE_NAME(PortSerdes, RxInstgC1Start1p7);
SAI_ATTRIBUTE_NAME(PortSerdes, RxInstgC1Step1p7);
SAI_ATTRIBUTE_NAME(PortSerdes, RxInstgC1Stop1p7);
SAI_ATTRIBUTE_NAME(PortSerdes, RxInstgDfeStart1p7);
SAI_ATTRIBUTE_NAME(PortSerdes, RxInstgDfeStep1p7);
SAI_ATTRIBUTE_NAME(PortSerdes, RxInstgDfeStop1p7);
SAI_ATTRIBUTE_NAME(PortSerdes, RxEnableScanSelection);
SAI_ATTRIBUTE_NAME(PortSerdes, RxInstgScanUseSrSettings);
SAI_ATTRIBUTE_NAME(PortSerdes, RxCdrCfgOvEn);
SAI_ATTRIBUTE_NAME(PortSerdes, RxCdrTdet1stOrdStepOvVal);
SAI_ATTRIBUTE_NAME(PortSerdes, RxCdrTdet2ndOrdStepOvVal);
SAI_ATTRIBUTE_NAME(PortSerdes, RxCdrTdetFineStepOvVal);

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

#if SAI_API_VERSION >= SAI_VERSION(1, 13, 0)
  sai_status_t _bulkGetAttribute(
      PortSaiId* keys,
      uint32_t* attrCount,
      sai_attribute_t** attr,
      sai_status_t* retStatus,
      size_t objectCount) const {
    sai_object_id_t rawIds[objectCount];
    for (auto idx = 0; idx < objectCount; idx++) {
      rawIds[idx] = *rawSaiId(&keys[idx]);
    }

    return api_->get_ports_attribute(
        objectCount,
        rawIds,
        attrCount,
        attr,
        SAI_BULK_OP_ERROR_MODE_STOP_ON_ERROR,
        retStatus);
  }
#endif

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
