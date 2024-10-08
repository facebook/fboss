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
#include "fboss/agent/hw/sai/api/SaiDefaultAttributeValues.h"
#include "fboss/agent/hw/sai/api/Types.h"
#include "fboss/agent/types.h"

#include <folly/MacAddress.h>
#include <folly/logging/xlog.h>

#include <optional>
#include <tuple>
#include <vector>

extern "C" {
#include <sai.h>
}

namespace facebook::fboss {

class SwitchApi;

struct SaiSwitchTraits {
  static constexpr sai_object_type_t ObjectType = SAI_OBJECT_TYPE_SWITCH;
  using SaiApiT = SwitchApi;
  struct Attributes {
    using EnumType = sai_switch_attr_t;
    using CpuPort = SaiAttribute<
        EnumType,
        SAI_SWITCH_ATTR_CPU_PORT,
        SaiObjectIdT,
        SaiObjectIdDefault>;
    using DefaultVirtualRouterId = SaiAttribute<
        EnumType,
        SAI_SWITCH_ATTR_DEFAULT_VIRTUAL_ROUTER_ID,
        SaiObjectIdT,
        SaiObjectIdDefault>;
    using DefaultVlanId = SaiAttribute<
        EnumType,
        SAI_SWITCH_ATTR_DEFAULT_VLAN_ID,
        SaiObjectIdT,
        SaiObjectIdDefault>;
    using PortNumber =
        SaiAttribute<EnumType, SAI_SWITCH_ATTR_PORT_NUMBER, sai_uint32_t>;
    using PortList = SaiAttribute<
        EnumType,
        SAI_SWITCH_ATTR_PORT_LIST,
        std::vector<sai_object_id_t>>;
    using SrcMac = SaiAttribute<
        EnumType,
        SAI_SWITCH_ATTR_SRC_MAC_ADDRESS,
        folly::MacAddress,
        SaiMacAddressDefault>;
    using HwInfo = SaiAttribute<
        EnumType,
        SAI_SWITCH_ATTR_SWITCH_HARDWARE_INFO,
        std::vector<int8_t>,
        SaiS8ListDefault>;
    using Default1QBridgeId = SaiAttribute<
        EnumType,
        SAI_SWITCH_ATTR_DEFAULT_1Q_BRIDGE_ID,
        SaiObjectIdT,
        SaiObjectIdDefault>;
    using InitSwitch =
        SaiAttribute<EnumType, SAI_SWITCH_ATTR_INIT_SWITCH, bool>;
    using MaxNumberOfSupportedPorts = SaiAttribute<
        EnumType,
        SAI_SWITCH_ATTR_MAX_NUMBER_OF_SUPPORTED_PORTS,
        sai_uint32_t>;
    using SwitchShellEnable = SaiAttribute<
        EnumType,
        SAI_SWITCH_ATTR_SWITCH_SHELL_ENABLE,
        bool,
        SaiBoolDefaultFalse>;
    using NumberOfUnicastQueues = SaiAttribute<
        EnumType,
        SAI_SWITCH_ATTR_NUMBER_OF_UNICAST_QUEUES,
        sai_uint32_t,
        SaiIntDefault<sai_uint32_t>>;
    using NumberOfMulticastQueues = SaiAttribute<
        EnumType,
        SAI_SWITCH_ATTR_NUMBER_OF_MULTICAST_QUEUES,
        sai_uint32_t,
        SaiIntDefault<sai_uint32_t>>;
    using NumberOfQueues = SaiAttribute<
        EnumType,
        SAI_SWITCH_ATTR_NUMBER_OF_QUEUES,
        sai_uint32_t,
        SaiIntDefault<sai_uint32_t>>;
    using NumberOfCpuQueues = SaiAttribute<
        EnumType,
        SAI_SWITCH_ATTR_NUMBER_OF_CPU_QUEUES,
        sai_uint32_t,
        SaiIntDefault<sai_uint32_t>>;
    using EcmpHash = SaiAttribute<
        EnumType,
        SAI_SWITCH_ATTR_ECMP_HASH,
        SaiObjectIdT,
        SaiObjectIdDefault>;
    using LagHash = SaiAttribute<
        EnumType,
        SAI_SWITCH_ATTR_LAG_HASH,
        SaiObjectIdT,
        SaiObjectIdDefault>;
    using EcmpHashV4 = SaiAttribute<
        EnumType,
        SAI_SWITCH_ATTR_ECMP_HASH_IPV4,
        SaiObjectIdT,
        SaiObjectIdDefault>;
    using LagHashV4 = SaiAttribute<
        EnumType,
        SAI_SWITCH_ATTR_LAG_HASH_IPV4,
        SaiObjectIdT,
        SaiObjectIdDefault>;
    using EcmpHashV6 = SaiAttribute<
        EnumType,
        SAI_SWITCH_ATTR_ECMP_HASH_IPV6,
        SaiObjectIdT,
        SaiObjectIdDefault>;
    using LagHashV6 = SaiAttribute<
        EnumType,
        SAI_SWITCH_ATTR_LAG_HASH_IPV6,
        SaiObjectIdT,
        SaiObjectIdDefault>;
    using EcmpDefaultHashSeed = SaiAttribute<
        EnumType,
        SAI_SWITCH_ATTR_ECMP_DEFAULT_HASH_SEED,
        sai_uint32_t,
        SaiIntDefault<sai_uint32_t>>;
    using LagDefaultHashSeed = SaiAttribute<
        EnumType,
        SAI_SWITCH_ATTR_LAG_DEFAULT_HASH_SEED,
        sai_uint32_t,
        SaiIntDefault<sai_uint32_t>>;
    using EcmpDefaultHashAlgorithm = SaiAttribute<
        EnumType,
        SAI_SWITCH_ATTR_ECMP_DEFAULT_HASH_ALGORITHM,
        int32_t,
        SaiIntDefault<int32_t>>;
    using LagDefaultHashAlgorithm = SaiAttribute<
        EnumType,
        SAI_SWITCH_ATTR_LAG_DEFAULT_HASH_ALGORITHM,
        int32_t,
        SaiIntDefault<int32_t>>;
    using SwitchRestartWarm = SaiAttribute<
        EnumType,
        SAI_SWITCH_ATTR_RESTART_WARM,
        bool,
        SaiBoolDefaultFalse>;
    using SwitchPreShutdown = SaiAttribute<
        EnumType,
        SAI_SWITCH_ATTR_PRE_SHUTDOWN,
        bool,
        SaiBoolDefaultFalse>;
    using QosDscpToTcMap = SaiAttribute<
        EnumType,
        SAI_SWITCH_ATTR_QOS_DSCP_TO_TC_MAP,
        SaiObjectIdT,
        SaiObjectIdDefault>;
    using QosTcToQueueMap = SaiAttribute<
        EnumType,
        SAI_SWITCH_ATTR_QOS_TC_TO_QUEUE_MAP,
        SaiObjectIdT,
        SaiObjectIdDefault>;
    using QosExpToTcMap = SaiAttribute<
        EnumType,
        SAI_SWITCH_ATTR_QOS_MPLS_EXP_TO_TC_MAP,
        SaiObjectIdT,
        SaiObjectIdDefault>;
    using QosTcToExpMap = SaiAttribute<
        EnumType,
        SAI_SWITCH_ATTR_QOS_TC_AND_COLOR_TO_MPLS_EXP_MAP,
        SaiObjectIdT,
        SaiObjectIdDefault>;
    using AclEntryMinimumPriority = SaiAttribute<
        EnumType,
        SAI_SWITCH_ATTR_ACL_ENTRY_MINIMUM_PRIORITY,
        sai_uint32_t>;
    using AclEntryMaximumPriority = SaiAttribute<
        EnumType,
        SAI_SWITCH_ATTR_ACL_ENTRY_MAXIMUM_PRIORITY,
        sai_uint32_t>;
    using MacAgingTime = SaiAttribute<
        EnumType,
        SAI_SWITCH_ATTR_FDB_AGING_TIME,
        sai_uint32_t,
        SaiIntDefault<sai_uint32_t>>;
    using FdbDstUserMetaDataRange = SaiAttribute<
        EnumType,
        SAI_SWITCH_ATTR_FDB_DST_USER_META_DATA_RANGE,
        sai_u32_range_t,
        SaiIntRangeDefault<sai_u32_range_t>>;
    using RouteDstUserMetaDataRange = SaiAttribute<
        EnumType,
        SAI_SWITCH_ATTR_ROUTE_DST_USER_META_DATA_RANGE,
        sai_u32_range_t,
        SaiIntRangeDefault<sai_u32_range_t>>;
    using NeighborDstUserMetaDataRange = SaiAttribute<
        EnumType,
        SAI_SWITCH_ATTR_NEIGHBOR_DST_USER_META_DATA_RANGE,
        sai_u32_range_t,
        SaiIntRangeDefault<sai_u32_range_t>>;
    using AvailableIpv4RouteEntry = SaiAttribute<
        EnumType,
        SAI_SWITCH_ATTR_AVAILABLE_IPV4_ROUTE_ENTRY,
        sai_uint32_t,
        SaiIntDefault<sai_uint32_t>>;
    using AvailableIpv6RouteEntry = SaiAttribute<
        EnumType,
        SAI_SWITCH_ATTR_AVAILABLE_IPV6_ROUTE_ENTRY,
        sai_uint32_t,
        SaiIntDefault<sai_uint32_t>>;
    using AvailableIpv4NextHopEntry = SaiAttribute<
        EnumType,
        SAI_SWITCH_ATTR_AVAILABLE_IPV4_NEXTHOP_ENTRY,
        sai_uint32_t,
        SaiIntDefault<sai_uint32_t>>;
    using AvailableIpv6NextHopEntry = SaiAttribute<
        EnumType,
        SAI_SWITCH_ATTR_AVAILABLE_IPV6_NEXTHOP_ENTRY,
        sai_uint32_t,
        SaiIntDefault<sai_uint32_t>>;
    using AvailableNextHopGroupEntry = SaiAttribute<
        EnumType,
        SAI_SWITCH_ATTR_AVAILABLE_NEXT_HOP_GROUP_ENTRY,
        sai_uint32_t,
        SaiIntDefault<sai_uint32_t>>;
    using AvailableNextHopGroupMemberEntry = SaiAttribute<
        EnumType,
        SAI_SWITCH_ATTR_AVAILABLE_NEXT_HOP_GROUP_MEMBER_ENTRY,
        sai_uint32_t,
        SaiIntDefault<sai_uint32_t>>;
    using AvailableIpv4NeighborEntry = SaiAttribute<
        EnumType,
        SAI_SWITCH_ATTR_AVAILABLE_IPV4_NEIGHBOR_ENTRY,
        sai_uint32_t,
        SaiIntDefault<sai_uint32_t>>;
    using AvailableIpv6NeighborEntry = SaiAttribute<
        EnumType,
        SAI_SWITCH_ATTR_AVAILABLE_IPV6_NEIGHBOR_ENTRY,
        sai_uint32_t,
        SaiIntDefault<sai_uint32_t>>;
    using IngressAcl = SaiAttribute<
        EnumType,
        SAI_SWITCH_ATTR_INGRESS_ACL,
        SaiObjectIdT,
        SaiObjectIdDefault>;
    using TamObject = SaiAttribute<
        EnumType,
        SAI_SWITCH_ATTR_TAM_OBJECT_ID,
        std::vector<sai_object_id_t>,
        SaiObjectIdListDefault>;
    using NumberOfFabricPorts = SaiAttribute<
        EnumType,
        SAI_SWITCH_ATTR_NUMBER_OF_FABRIC_PORTS,
        sai_uint32_t>;
    using FabricPortList = SaiAttribute<
        EnumType,
        SAI_SWITCH_ATTR_FABRIC_PORT_LIST,
        std::vector<sai_object_id_t>>;
    /*
     * Use ECN thresholds for ECT traffic, if not set WRED thresholds
     * would be used
     */
    using UseEcnThresholds = SaiAttribute<
        EnumType,
        SAI_SWITCH_ATTR_ECN_ECT_THRESHOLD_ENABLE,
        bool,
        SaiBoolDefaultFalse>;
    using FirmwarePathName = SaiAttribute<
        EnumType,
        SAI_SWITCH_ATTR_FIRMWARE_PATH_NAME,
        std::vector<sai_int8_t>,
        SaiS8ListDefault>;
    using FirmwareLoadMethod = SaiAttribute<
        EnumType,
        SAI_SWITCH_ATTR_FIRMWARE_LOAD_METHOD,
        int32_t,
        SaiIntDefault<int32_t>>;
    using FirmwareLoadType = SaiAttribute<
        EnumType,
        SAI_SWITCH_ATTR_FIRMWARE_LOAD_TYPE,
        int32_t,
        SaiIntDefault<int32_t>>;
    using HardwareAccessBus = SaiAttribute<
        EnumType,
        SAI_SWITCH_ATTR_HARDWARE_ACCESS_BUS,
        int32_t,
        SaiIntDefault<int32_t>>;
    using PlatformContext = SaiAttribute<
        EnumType,
        SAI_SWITCH_ATTR_PLATFROM_CONTEXT,
        sai_uint64_t,
        SaiIntDefault<sai_uint64_t>>;
    using SwitchProfileId = SaiAttribute<
        EnumType,
        SAI_SWITCH_ATTR_SWITCH_PROFILE_ID,
        sai_uint32_t,
        SaiIntDefault<sai_uint32_t>>;
    using FirmwareStatus = SaiAttribute<
        EnumType,
        SAI_SWITCH_ATTR_FIRMWARE_STATUS,
        bool,
        SaiBoolDefaultTrue>;
    using FirmwareMajorVersion = SaiAttribute<
        EnumType,
        SAI_SWITCH_ATTR_FIRMWARE_MAJOR_VERSION,
        sai_uint32_t,
        SaiIntDefault<sai_uint32_t>>;
    using FirmwareMinorVersion = SaiAttribute<
        EnumType,
        SAI_SWITCH_ATTR_FIRMWARE_MINOR_VERSION,
        sai_uint32_t,
        SaiIntDefault<sai_uint32_t>>;
    using PortConnectorList = SaiAttribute<
        EnumType,
        SAI_SWITCH_ATTR_PORT_CONNECTOR_LIST,
        std::vector<sai_object_id_t>,
        SaiObjectIdListDefault>;
    using SwitchId = SaiAttribute<
        EnumType,
        SAI_SWITCH_ATTR_SWITCH_ID,
        sai_uint32_t,
        SaiIntDefault<sai_uint32_t>>;
    using MaxSystemCores = SaiAttribute<
        EnumType,
        SAI_SWITCH_ATTR_MAX_SYSTEM_CORES,
        sai_uint32_t,
        SaiIntDefault<sai_uint32_t>>;
    using SysPortConfigList = SaiAttribute<
        EnumType,
        SAI_SWITCH_ATTR_SYSTEM_PORT_CONFIG_LIST,
        std::vector<sai_system_port_config_t>,
        SaiListDefault<sai_system_port_config_list_t>>;
    using SwitchType = SaiAttribute<
        EnumType,
        SAI_SWITCH_ATTR_TYPE,
        sai_uint32_t,
        SaiSwitchTypeDefault>;
    using RegisterReadFn = SaiAttribute<
        EnumType,
        SAI_SWITCH_ATTR_REGISTER_READ,
        sai_pointer_t,
        SaiPointerDefault>;
    using RegisterWriteFn = SaiAttribute<
        EnumType,
        SAI_SWITCH_ATTR_REGISTER_WRITE,
        sai_pointer_t,
        SaiPointerDefault>;
    /*
     * Set the default value to 1 since we read the stats only
     * from sw cache.
     */
    using CounterRefreshInterval = SaiAttribute<
        EnumType,
        SAI_SWITCH_ATTR_COUNTER_REFRESH_INTERVAL,
        sai_uint32_t,
        SaiInt1Default>;
#if SAI_API_VERSION >= SAI_VERSION(1, 10, 2)
    using MaxEcmpMemberCount = SaiAttribute<
        EnumType,
        SAI_SWITCH_ATTR_MAX_ECMP_MEMBER_COUNT,
        sai_uint32_t,
        SaiIntDefault<sai_uint32_t>>;
    using EcmpMemberCount = SaiAttribute<
        EnumType,
        SAI_SWITCH_ATTR_ECMP_MEMBER_COUNT,
        sai_uint32_t,
        SaiIntDefault<sai_uint32_t>>;
#endif
    /* extension attributes */
    struct AttributeLedIdWrapper {
      std::optional<sai_attr_id_t> operator()();
    };
    struct AttributeLedResetIdWrapper {
      std::optional<sai_attr_id_t> operator()();
    };
    using Led =
        SaiExtensionAttribute<std::vector<sai_uint32_t>, AttributeLedIdWrapper>;
    using LedReset = SaiExtensionAttribute<
        std::vector<sai_uint32_t>,
        AttributeLedResetIdWrapper>;

    struct AttributeAclFieldListWrapper {
      std::optional<sai_attr_id_t> operator()();
    };
    using AclFieldList = SaiExtensionAttribute<
        std::vector<sai_int32_t>,
        AttributeAclFieldListWrapper>;
    struct AttributeEgressPoolAvaialableSizeIdWrapper {
      std::optional<sai_attr_id_t> operator()();
    };
    using EgressPoolAvaialableSize = SaiExtensionAttribute<
        sai_uint32_t,
        AttributeEgressPoolAvaialableSizeIdWrapper>;

    struct HwEccErrorInitiateWrapper {
      std::optional<sai_attr_id_t> operator()();
    };
    using HwEccErrorInitiate =
        SaiExtensionAttribute<sai_uint16_t, HwEccErrorInitiateWrapper>;
    struct AttributeDllPathWrapper {
      std::optional<sai_attr_id_t> operator()();
    };
    using DllPath =
        SaiExtensionAttribute<std::vector<int8_t>, AttributeDllPathWrapper>;
    struct AttributeRestartIssuWrapper {
      std::optional<sai_attr_id_t> operator()();
    };
    using RestartIssu =
        SaiExtensionAttribute<bool, AttributeRestartIssuWrapper>;
    struct AttributeDelayDropCongThreshold {
      std::optional<sai_attr_id_t> operator()();
    };
    using DelayDropCongThreshold =
        SaiExtensionAttribute<sai_uint8_t, AttributeDelayDropCongThreshold>;
    struct AttributeForceTrafficOverFabricWrapper {
      std::optional<sai_attr_id_t> operator()();
    };
    using ForceTrafficOverFabric =
        SaiExtensionAttribute<bool, AttributeForceTrafficOverFabricWrapper>;
    struct AttributeWarmBootTargetVersionWrapper {
      std::optional<sai_attr_id_t> operator()();
    };
    using WarmBootTargetVersion = SaiExtensionAttribute<
        std::vector<sai_int8_t>,
        AttributeWarmBootTargetVersionWrapper>;

    struct AttributeSwitchIsolateWrapper {
      std::optional<sai_attr_id_t> operator()();
    };
    using SwitchIsolate =
        SaiExtensionAttribute<bool, AttributeSwitchIsolateWrapper>;
#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)
    using CreditWd = SaiAttribute<
        EnumType,
        SAI_SWITCH_ATTR_CREDIT_WD,
        bool,
        SaiBoolDefaultTrue>;
    using CreditWdTimer = SaiAttribute<
        EnumType,
        SAI_SWITCH_ATTR_CREDIT_WD_TIMER,
        sai_uint32_t,
        SaiIntValueDefault<sai_uint32_t, 500>>;
#endif
    struct AttributeMaxCoresWrapper {
      std::optional<sai_attr_id_t> operator()();
    };
    using MaxCores = SaiExtensionAttribute<
        sai_uint32_t,
        AttributeMaxCoresWrapper,
        SaiIntDefault<uint32_t>>;
    struct AttributeSdkBootTimeWrapper {
      std::optional<sai_attr_id_t> operator()();
    };
    using SdkBootTime = SaiExtensionAttribute<
        sai_uint32_t,
        AttributeSdkBootTimeWrapper,
        SaiIntDefault<uint32_t>>;
    using PfcDlrPacketAction = SaiAttribute<
        EnumType,
        SAI_SWITCH_ATTR_PFC_DLR_PACKET_ACTION,
        sai_int32_t,
        SaiPacketActionDefaultDrop>;
    struct AttributeFabricRemoteReachablePortList {
      std::optional<sai_attr_id_t> operator()();
    };
    using FabricRemoteReachablePortList = SaiExtensionAttribute<
        std::vector<sai_object_id_t>,
        AttributeFabricRemoteReachablePortList>;
    struct AttributeRouteNoImplicitMetaDataWrapper {
      std::optional<sai_attr_id_t> operator()();
    };
    using RouteNoImplicitMetaData =
        SaiExtensionAttribute<bool, AttributeRouteNoImplicitMetaDataWrapper>;
    struct AttributeRouteAllowImplicitMetaDataWrapper {
      std::optional<sai_attr_id_t> operator()();
    };
    using RouteAllowImplicitMetaData =
        SaiExtensionAttribute<bool, AttributeRouteAllowImplicitMetaDataWrapper>;
    struct AttributeMultiStageLocalSwitchIdsWrapper {
      std::optional<sai_attr_id_t> operator()();
    };
    using MultiStageLocalSwitchIds = SaiExtensionAttribute<
        std::vector<sai_uint32_t>,
        AttributeMultiStageLocalSwitchIdsWrapper,
        SaiU32ListDefault>;
    struct AttributeVoqLatencyMinLocalNs {
      std::optional<sai_attr_id_t> operator()();
    };
    using VoqLatencyMinLocalNs = SaiExtensionAttribute<
        sai_int32_t,
        AttributeVoqLatencyMinLocalNs,
        SaiIntDefault<sai_int32_t>>;
    struct AttributeVoqLatencyMaxLocalNs {
      std::optional<sai_attr_id_t> operator()();
    };
    using VoqLatencyMaxLocalNs = SaiExtensionAttribute<
        sai_int32_t,
        AttributeVoqLatencyMaxLocalNs,
        SaiIntDefault<sai_int32_t>>;
    struct AttributeVoqLatencyMinLevel1Ns {
      std::optional<sai_attr_id_t> operator()();
    };
    using VoqLatencyMinLevel1Ns = SaiExtensionAttribute<
        sai_int32_t,
        AttributeVoqLatencyMinLevel1Ns,
        SaiIntDefault<sai_int32_t>>;
    struct AttributeVoqLatencyMaxLevel1Ns {
      std::optional<sai_attr_id_t> operator()();
    };
    using VoqLatencyMaxLevel1Ns = SaiExtensionAttribute<
        sai_int32_t,
        AttributeVoqLatencyMaxLevel1Ns,
        SaiIntDefault<sai_int32_t>>;
    struct AttributeVoqLatencyMinLevel2Ns {
      std::optional<sai_attr_id_t> operator()();
    };
    using VoqLatencyMinLevel2Ns = SaiExtensionAttribute<
        sai_int32_t,
        AttributeVoqLatencyMinLevel2Ns,
        SaiIntDefault<sai_int32_t>>;
    struct AttributeVoqLatencyMaxLevel2Ns {
      std::optional<sai_attr_id_t> operator()();
    };
    using VoqLatencyMaxLevel2Ns = SaiExtensionAttribute<
        sai_int32_t,
        AttributeVoqLatencyMaxLevel2Ns,
        SaiIntDefault<sai_int32_t>>;
#if SAI_API_VERSION >= SAI_VERSION(1, 14, 0)
    using ArsProfile = SaiAttribute<
        EnumType,
        SAI_SWITCH_ATTR_ARS_PROFILE,
        SaiObjectIdT,
        SaiObjectIdDefault>;
#endif
    struct AttributeReachabilityGroupList {
      std::optional<sai_attr_id_t> operator()();
    };
    using ReachabilityGroupList = SaiExtensionAttribute<
        std::vector<sai_uint32_t>,
        AttributeReachabilityGroupList,
        SaiU32ListDefault>;
    struct AttributeFabricLinkLayerFlowControlThreshold {
      std::optional<sai_attr_id_t> operator()();
    };
    using FabricLinkLayerFlowControlThreshold = SaiExtensionAttribute<
        std::vector<sai_uint32_t>,
        AttributeFabricLinkLayerFlowControlThreshold,
        SaiU32ListDefault>;
  };
  using AdapterKey = SwitchSaiId;
  using AdapterHostKey = std::monostate;
  using CreateAttributes = std::tuple<
      Attributes::InitSwitch,
      std::optional<Attributes::HwInfo>,
      std::optional<Attributes::SrcMac>,
      std::optional<Attributes::SwitchShellEnable>,
      std::optional<Attributes::EcmpHashV4>,
      std::optional<Attributes::EcmpHashV6>,
      std::optional<Attributes::LagHashV4>,
      std::optional<Attributes::LagHashV6>,
      std::optional<Attributes::EcmpDefaultHashSeed>,
      std::optional<Attributes::LagDefaultHashSeed>,
      std::optional<Attributes::EcmpDefaultHashAlgorithm>,
      std::optional<Attributes::LagDefaultHashAlgorithm>,
      std::optional<Attributes::SwitchRestartWarm>,
      std::optional<Attributes::QosDscpToTcMap>,
      std::optional<Attributes::QosTcToQueueMap>,
      std::optional<Attributes::QosExpToTcMap>,
      std::optional<Attributes::QosTcToExpMap>,
      std::optional<Attributes::MacAgingTime>,
      std::optional<Attributes::IngressAcl>,
      std::optional<Attributes::AclFieldList>,
      std::optional<Attributes::TamObject>,
      std::optional<Attributes::UseEcnThresholds>,
      std::optional<Attributes::CounterRefreshInterval>,
      std::optional<Attributes::FirmwarePathName>,
      std::optional<Attributes::FirmwareLoadMethod>,
      std::optional<Attributes::FirmwareLoadType>,
      std::optional<Attributes::HardwareAccessBus>,
      std::optional<Attributes::PlatformContext>,
      std::optional<Attributes::SwitchProfileId>,
      std::optional<Attributes::SwitchId>,
      std::optional<Attributes::MaxSystemCores>,
      std::optional<Attributes::SysPortConfigList>,
      std::optional<Attributes::SwitchType>,
      std::optional<Attributes::RegisterReadFn>,
      std::optional<Attributes::RegisterWriteFn>
#if SAI_API_VERSION >= SAI_VERSION(1, 10, 2)
      ,
      std::optional<Attributes::MaxEcmpMemberCount>,
      std::optional<Attributes::EcmpMemberCount>
#endif
      ,
      std::optional<Attributes::DllPath>,
      std::optional<Attributes::RestartIssu>,
      std::optional<Attributes::SwitchIsolate>,
#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)
      std::optional<Attributes::CreditWd>,
      std::optional<Attributes::CreditWdTimer>,
#endif
      std::optional<Attributes::MaxCores>,
      std::optional<Attributes::PfcDlrPacketAction>,
      std::optional<Attributes::RouteNoImplicitMetaData>,
      std::optional<Attributes::RouteAllowImplicitMetaData>,
      std::optional<Attributes::MultiStageLocalSwitchIds>,
      std::optional<Attributes::VoqLatencyMinLocalNs>,
      std::optional<Attributes::VoqLatencyMaxLocalNs>,
      std::optional<Attributes::VoqLatencyMinLevel1Ns>,
      std::optional<Attributes::VoqLatencyMaxLevel1Ns>,
      std::optional<Attributes::VoqLatencyMinLevel2Ns>,
      std::optional<Attributes::VoqLatencyMaxLevel2Ns>,
#if SAI_API_VERSION >= SAI_VERSION(1, 14, 0)
      std::optional<Attributes::ArsProfile>,
#endif
      std::optional<Attributes::ReachabilityGroupList>,
      std::optional<Attributes::DelayDropCongThreshold>,
      std::optional<Attributes::FabricLinkLayerFlowControlThreshold>>;

#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)
  static constexpr std::array<sai_stat_id_t, 3> CounterIdsToRead = {
      SAI_SWITCH_STAT_REACHABILITY_DROP,
      SAI_SWITCH_STAT_GLOBAL_DROP,
      SAI_SWITCH_STAT_PACKET_INTEGRITY_DROP,
  };
#else
  static constexpr std::array<sai_stat_id_t, 2> CounterIdsToRead = {
      SAI_SWITCH_STAT_REACHABILITY_DROP,
      SAI_SWITCH_STAT_GLOBAL_DROP,
  };
#endif
  static constexpr std::array<sai_stat_id_t, 0> CounterIdsToReadAndClear = {};
  static const std::vector<sai_stat_id_t>& dramStats();
  static const std::vector<sai_stat_id_t>& rciWatermarkStats();
  static const std::vector<sai_stat_id_t>& dtlWatermarkStats();
  static const std::vector<sai_stat_id_t>& dramBlockTime();
  static const std::vector<sai_stat_id_t>& egressCoreBufferWatermarkBytes();
  static const std::vector<sai_stat_id_t>& deletedCredits();
};

SAI_ATTRIBUTE_NAME(Switch, InitSwitch)
SAI_ATTRIBUTE_NAME(Switch, HwInfo)
SAI_ATTRIBUTE_NAME(Switch, SrcMac)
SAI_ATTRIBUTE_NAME(Switch, SwitchShellEnable)
SAI_ATTRIBUTE_NAME(Switch, EcmpHashV4)
SAI_ATTRIBUTE_NAME(Switch, EcmpHashV6)
SAI_ATTRIBUTE_NAME(Switch, LagHashV4)
SAI_ATTRIBUTE_NAME(Switch, LagHashV6)
SAI_ATTRIBUTE_NAME(Switch, EcmpDefaultHashSeed)
SAI_ATTRIBUTE_NAME(Switch, LagDefaultHashSeed)
SAI_ATTRIBUTE_NAME(Switch, EcmpDefaultHashAlgorithm)
SAI_ATTRIBUTE_NAME(Switch, LagDefaultHashAlgorithm)
SAI_ATTRIBUTE_NAME(Switch, SwitchRestartWarm)
SAI_ATTRIBUTE_NAME(Switch, SwitchPreShutdown)

SAI_ATTRIBUTE_NAME(Switch, CpuPort)
SAI_ATTRIBUTE_NAME(Switch, DefaultVlanId)
SAI_ATTRIBUTE_NAME(Switch, PortNumber)
SAI_ATTRIBUTE_NAME(Switch, PortList)
SAI_ATTRIBUTE_NAME(Switch, Default1QBridgeId)
SAI_ATTRIBUTE_NAME(Switch, DefaultVirtualRouterId)
SAI_ATTRIBUTE_NAME(Switch, NumberOfMulticastQueues)
SAI_ATTRIBUTE_NAME(Switch, LagHash)
SAI_ATTRIBUTE_NAME(Switch, EcmpHash)
SAI_ATTRIBUTE_NAME(Switch, QosDscpToTcMap)
SAI_ATTRIBUTE_NAME(Switch, QosTcToQueueMap)
SAI_ATTRIBUTE_NAME(Switch, QosExpToTcMap)
SAI_ATTRIBUTE_NAME(Switch, QosTcToExpMap)

SAI_ATTRIBUTE_NAME(Switch, AclEntryMinimumPriority)
SAI_ATTRIBUTE_NAME(Switch, AclEntryMaximumPriority)

SAI_ATTRIBUTE_NAME(Switch, MacAgingTime)

SAI_ATTRIBUTE_NAME(Switch, FdbDstUserMetaDataRange)
SAI_ATTRIBUTE_NAME(Switch, RouteDstUserMetaDataRange)
SAI_ATTRIBUTE_NAME(Switch, NeighborDstUserMetaDataRange)

SAI_ATTRIBUTE_NAME(Switch, AvailableIpv4RouteEntry)
SAI_ATTRIBUTE_NAME(Switch, AvailableIpv6RouteEntry)
SAI_ATTRIBUTE_NAME(Switch, AvailableIpv4NextHopEntry)
SAI_ATTRIBUTE_NAME(Switch, AvailableIpv6NextHopEntry)
SAI_ATTRIBUTE_NAME(Switch, AvailableNextHopGroupEntry)
SAI_ATTRIBUTE_NAME(Switch, AvailableNextHopGroupMemberEntry)
SAI_ATTRIBUTE_NAME(Switch, AvailableIpv4NeighborEntry)
SAI_ATTRIBUTE_NAME(Switch, AvailableIpv6NeighborEntry)

SAI_ATTRIBUTE_NAME(Switch, Led)
SAI_ATTRIBUTE_NAME(Switch, LedReset)

SAI_ATTRIBUTE_NAME(Switch, IngressAcl)

SAI_ATTRIBUTE_NAME(Switch, AclFieldList)
SAI_ATTRIBUTE_NAME(Switch, TamObject)
SAI_ATTRIBUTE_NAME(Switch, NumberOfFabricPorts)
SAI_ATTRIBUTE_NAME(Switch, FabricPortList)
SAI_ATTRIBUTE_NAME(Switch, UseEcnThresholds)
SAI_ATTRIBUTE_NAME(Switch, EgressPoolAvaialableSize)
SAI_ATTRIBUTE_NAME(Switch, CounterRefreshInterval)

SAI_ATTRIBUTE_NAME(Switch, FirmwarePathName)
SAI_ATTRIBUTE_NAME(Switch, FirmwareLoadMethod)
SAI_ATTRIBUTE_NAME(Switch, FirmwareLoadType)
SAI_ATTRIBUTE_NAME(Switch, HardwareAccessBus)
SAI_ATTRIBUTE_NAME(Switch, PlatformContext)
SAI_ATTRIBUTE_NAME(Switch, SwitchProfileId)
SAI_ATTRIBUTE_NAME(Switch, FirmwareStatus)
SAI_ATTRIBUTE_NAME(Switch, FirmwareMajorVersion)
SAI_ATTRIBUTE_NAME(Switch, FirmwareMinorVersion)
SAI_ATTRIBUTE_NAME(Switch, PortConnectorList)
SAI_ATTRIBUTE_NAME(Switch, SwitchId)
SAI_ATTRIBUTE_NAME(Switch, MaxSystemCores)
SAI_ATTRIBUTE_NAME(Switch, SysPortConfigList)
SAI_ATTRIBUTE_NAME(Switch, SwitchType)
SAI_ATTRIBUTE_NAME(Switch, RegisterReadFn)
SAI_ATTRIBUTE_NAME(Switch, RegisterWriteFn)

SAI_ATTRIBUTE_NAME(Switch, HwEccErrorInitiate)
#if SAI_API_VERSION >= SAI_VERSION(1, 10, 2)
SAI_ATTRIBUTE_NAME(Switch, MaxEcmpMemberCount)
SAI_ATTRIBUTE_NAME(Switch, EcmpMemberCount)
#endif
SAI_ATTRIBUTE_NAME(Switch, DllPath)
SAI_ATTRIBUTE_NAME(Switch, RestartIssu)
SAI_ATTRIBUTE_NAME(Switch, DelayDropCongThreshold)
SAI_ATTRIBUTE_NAME(Switch, ForceTrafficOverFabric)
SAI_ATTRIBUTE_NAME(Switch, WarmBootTargetVersion)
SAI_ATTRIBUTE_NAME(Switch, SwitchIsolate)
#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)
SAI_ATTRIBUTE_NAME(Switch, CreditWd)
SAI_ATTRIBUTE_NAME(Switch, CreditWdTimer)
#endif
SAI_ATTRIBUTE_NAME(Switch, MaxCores)
SAI_ATTRIBUTE_NAME(Switch, PfcDlrPacketAction)
SAI_ATTRIBUTE_NAME(Switch, SdkBootTime)
SAI_ATTRIBUTE_NAME(Switch, FabricRemoteReachablePortList)
SAI_ATTRIBUTE_NAME(Switch, RouteNoImplicitMetaData)
SAI_ATTRIBUTE_NAME(Switch, RouteAllowImplicitMetaData)
SAI_ATTRIBUTE_NAME(Switch, MultiStageLocalSwitchIds)
SAI_ATTRIBUTE_NAME(Switch, VoqLatencyMinLocalNs);
SAI_ATTRIBUTE_NAME(Switch, VoqLatencyMaxLocalNs);
SAI_ATTRIBUTE_NAME(Switch, VoqLatencyMinLevel1Ns);
SAI_ATTRIBUTE_NAME(Switch, VoqLatencyMaxLevel1Ns);
SAI_ATTRIBUTE_NAME(Switch, VoqLatencyMinLevel2Ns);
SAI_ATTRIBUTE_NAME(Switch, VoqLatencyMaxLevel2Ns);
#if SAI_API_VERSION >= SAI_VERSION(1, 14, 0)
SAI_ATTRIBUTE_NAME(Switch, ArsProfile)
#endif
SAI_ATTRIBUTE_NAME(Switch, ReachabilityGroupList);
SAI_ATTRIBUTE_NAME(Switch, FabricLinkLayerFlowControlThreshold);

template <>
struct SaiObjectHasStats<SaiSwitchTraits> : public std::true_type {};

class SwitchApi : public SaiApi<SwitchApi> {
 public:
  static constexpr sai_api_t ApiType = SAI_API_SWITCH;
  SwitchApi() {
    sai_status_t status =
        sai_api_query(ApiType, reinterpret_cast<void**>(&api_));
    saiApiCheckError(status, ApiType, "Failed to query for switch api");
  }
  const sai_switch_api_t* api() const {
    return api_;
  }

  void registerRxCallback(
      SwitchSaiId id,
      sai_packet_event_notification_fn rx_cb) const;
  void registerPortStateChangeCallback(
      SwitchSaiId id,
      sai_port_state_change_notification_fn port_state_change_cb) const;
  void registerFdbEventCallback(
      SwitchSaiId id,
      sai_fdb_event_notification_fn fdb_event_cb) const;
  void registerTamEventCallback(
      SwitchSaiId id,
      sai_tam_event_notification_fn tam_event_cb) const;
  void registerSwitchEventCallback(SwitchSaiId id, void* switch_event_cb) const;
  void registerQueuePfcDeadlockNotificationCallback(
      SwitchSaiId id,
      sai_queue_pfc_deadlock_notification_fn queue_pfc_deadlock_notification_cb)
      const;
#if SAI_API_VERSION >= SAI_VERSION(1, 13, 0)
  void registerTxReadyStatusChangeCallback(
      SwitchSaiId id,
      sai_port_host_tx_ready_notification_fn port_state_change_cb) const;
#endif

  void unregisterRxCallback(SwitchSaiId switch_id) const {
    registerRxCallback(switch_id, nullptr);
  }
  void unregisterPortStateChangeCallback(SwitchSaiId id) const {
    registerPortStateChangeCallback(id, nullptr);
  }
  void unregisterFdbEventCallback(SwitchSaiId id) const {
    registerFdbEventCallback(id, nullptr);
  }
  void unregisterTamEventCallback(SwitchSaiId id) const {
    registerTamEventCallback(id, nullptr);
  }
  void unregisterSwitchEventCallback(SwitchSaiId id) const {
    registerSwitchEventCallback(id, nullptr);
  }
  void unregisterQueuePfcDeadlockNotificationCallback(SwitchSaiId id) const {
    registerQueuePfcDeadlockNotificationCallback(id, nullptr);
  }
#if SAI_API_VERSION >= SAI_VERSION(1, 13, 0)
  void unregisterTxReadyStatusChangeCallback(SwitchSaiId id) const {
    registerTxReadyStatusChangeCallback(id, nullptr);
  }
#endif

 private:
  sai_status_t _create(
      SwitchSaiId* id,
      sai_object_id_t /* switch_id */,
      size_t attr_count,
      sai_attribute_t* attr_list) const {
    return api_->create_switch(rawSaiId(id), attr_count, attr_list);
  }
  sai_status_t _remove(SwitchSaiId id) const {
    return api_->remove_switch(id);
  }
  sai_status_t _getAttribute(SwitchSaiId id, sai_attribute_t* attr) const {
    return api_->get_switch_attribute(id, 1, attr);
  }
  sai_status_t _setAttribute(SwitchSaiId id, const sai_attribute_t* attr)
      const {
    return api_->set_switch_attribute(id, attr);
  }
  sai_status_t _getStats(
      SwitchSaiId key,
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
        ? api_->get_switch_stats(key, num_of_counters, counter_ids, counters)
        : api_->get_switch_stats_ext(
              key, num_of_counters, counter_ids, mode, counters);
  }

  sai_status_t _clearStats(
      SwitchSaiId key,
      uint32_t num_of_counters,
      const sai_stat_id_t* counter_ids) const {
    return api_->clear_switch_stats(key, num_of_counters, counter_ids);
  }

  sai_switch_api_t* api_;
  friend class SaiApi<SwitchApi>;
};

} // namespace facebook::fboss
