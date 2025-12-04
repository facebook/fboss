// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/api/AclApi.h"
#include "fboss/agent/hw/sai/api/ArsProfileApi.h"
#include "fboss/agent/hw/sai/api/BufferApi.h"
#include "fboss/agent/hw/sai/api/DebugCounterApi.h"
#include "fboss/agent/hw/sai/api/HostifApi.h"
#include "fboss/agent/hw/sai/api/MirrorApi.h"
#include "fboss/agent/hw/sai/api/NextHopGroupApi.h"
#include "fboss/agent/hw/sai/api/PortApi.h"
#include "fboss/agent/hw/sai/api/QueueApi.h"
#include "fboss/agent/hw/sai/api/SwitchApi.h"
#include "fboss/agent/hw/sai/api/SystemPortApi.h"
#include "fboss/agent/hw/sai/api/TamApi.h"

extern "C" {
#include "fboss/agent/hw/sai/api/fake/saifakeextensions.h"
}

namespace facebook::fboss {

namespace detail {
std::optional<sai_int32_t> trapDrops() {
  return std::nullopt;
}
} // namespace detail
std::optional<sai_attr_id_t>
SaiPortSerdesTraits::Attributes::AttributeTxLutModeIdWrapper::operator()() {
  return SAI_PORT_SERDES_ATTR_EXT_TX_LUT_MODE;
}
std::optional<sai_attr_id_t>
SaiPortSerdesTraits::Attributes::AttributeRxCtleCodeIdWrapper::operator()() {
  return SAI_PORT_SERDES_ATTR_EXT_FAKE_RX_CTLE_CODE;
}
std::optional<sai_attr_id_t>
SaiPortSerdesTraits::Attributes::AttributeRxDspModeIdWrapper::operator()() {
  return SAI_PORT_SERDES_ATTR_EXT_FAKE_RX_DSP_MODE;
}
std::optional<sai_attr_id_t>
SaiPortSerdesTraits::Attributes::AttributeRxAfeTrimIdWrapper::operator()() {
  return SAI_PORT_SERDES_ATTR_EXT_FAKE_RX_AFE_TRIM;
}
std::optional<sai_attr_id_t> SaiPortSerdesTraits::Attributes::
    AttributeRxAcCouplingBypassIdWrapper::operator()() {
  return SAI_PORT_SERDES_ATTR_EXT_FAKE_RX_AC_COUPLING_BYPASS;
}

std::optional<sai_attr_id_t>
SaiSwitchTraits::Attributes::AttributeLedIdWrapper::operator()() {
  return SAI_SWITCH_ATTR_EXT_FAKE_LED;
}
std::optional<sai_attr_id_t>
SaiSwitchTraits::Attributes::AttributeLedResetIdWrapper::operator()() {
  return SAI_SWITCH_ATTR_EXT_FAKE_LED_RESET;
}

std::optional<sai_attr_id_t>
SaiTamEventTraits::Attributes::AttributeSwitchEventType::operator()() {
  return SAI_TAM_EVENT_ATTR_FAKE_SWITCH_EVENT_TYPE;
}

std::optional<sai_attr_id_t>
SaiTamEventTraits::Attributes::AttributeDeviceId::operator()() {
  return SAI_TAM_EVENT_ATTR_FAKE_DEVICE_ID;
}

std::optional<sai_attr_id_t>
SaiTamEventTraits::Attributes::AttributeEventId::operator()() {
  return SAI_TAM_EVENT_ATTR_FAKE_SWITCH_EVENT_ID;
}

std::optional<sai_attr_id_t>
SaiTamEventTraits::Attributes::AttributeExtensionsCollectorList::operator()() {
  return SAI_TAM_EVENT_ATTR_FAKE_EXTENSIONS_COLLECTOR_LIST;
}

std::optional<sai_attr_id_t>
SaiTamEventTraits::Attributes::AttributePacketDropTypeMmu::operator()() {
  return SAI_TAM_EVENT_ATTR_FAKE_PACKET_DROP_TYPE_MMU;
}

std::optional<sai_attr_id_t>
SaiTamEventTraits::Attributes::AttributeAgingGroup::operator()() {
  return SAI_TAM_EVENT_ATTR_FAKE_AGING_GROUP;
}

std::optional<sai_attr_id_t>
SaiTamTransportTraits::Attributes::AttributeSrcMacAddress::operator()() {
  return SAI_TAM_TRANSPORT_ATTR_FAKE_SRC_MAC_ADDRESS;
}

std::optional<sai_attr_id_t>
SaiTamTransportTraits::Attributes::AttributeDstMacAddress::operator()() {
  return SAI_TAM_TRANSPORT_ATTR_FAKE_DST_MAC_ADDRESS;
}

std::optional<sai_attr_id_t>
SaiSwitchTraits::Attributes::AttributeAclFieldListWrapper::operator()() {
  return SAI_SWITCH_ATTR_EXT_FAKE_ACL_FIELD_LIST;
}

std::optional<sai_attr_id_t> SaiSwitchTraits::Attributes::
    AttributeEgressPoolAvailableSizeIdWrapper::operator()() {
  return SAI_SWITCH_ATTR_DEFAULT_EGRESS_BUFFER_POOL_SHARED_SIZE;
}

std::optional<sai_attr_id_t>
SaiSwitchTraits::Attributes::HwEccErrorInitiateWrapper::operator()() {
  return SAI_SWITCH_ATTR_EXT_FAKE_HW_ECC_ERROR_INITIATE;
}
std::optional<sai_attr_id_t>
SaiPortTraits::Attributes::AttributeSystemPortId::operator()() {
  return SAI_PORT_ATTR_EXT_FAKE_SYSTEM_PORT_ID;
}
std::optional<sai_attr_id_t>
SaiPortTraits::Attributes::AttributeArsLinkState::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t> SaiArsProfileTraits::Attributes::
    AttributeExtensionSamplingIntervalNanosec::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiPortTraits::Attributes::AttributeIsHyperPortMember::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiPortTraits::Attributes::AttributeHyperPortMemberList::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t> SaiPortSerdesTraits::Attributes::
    AttributeRxAfeAdaptiveEnableWrapper::operator()() {
  return SAI_PORT_SERDES_ATTR_EXT_FAKE_RX_AFE_ADAPTIVE_ENABLE;
}

std::optional<sai_attr_id_t>
SaiSwitchTraits::Attributes::AttributeDllPathWrapper::operator()() {
  return SAI_SWITCH_ATTR_ISSU_CUSTOM_DLL_PATH;
}

std::optional<sai_attr_id_t>
SaiPortTraits::Attributes::AttributeSerdesLaneList::operator()() {
  return SAI_PORT_ATTR_SERDES_LANE_LIST;
}

std::optional<sai_attr_id_t>
SaiPortTraits::Attributes::AttributeStaticModuleId::operator()() {
  return SAI_PORT_ATTR_STATIC_MODULE_ID;
}

std::optional<sai_attr_id_t>
SaiPortTraits::Attributes::AttributeDiagModeEnable::operator()() {
  return SAI_PORT_ATTR_DIAGNOSTICS_MODE_ENABLE;
}

std::optional<sai_attr_id_t>
SaiPortTraits::Attributes::AttributeFdrEnable::operator()() {
  return SAI_PORT_ATTR_FDR_ENABLE;
}

std::optional<sai_attr_id_t>
SaiPortTraits::Attributes::AttributeCrcErrorDetect::operator()() {
  return SAI_PORT_ATTR_CRC_ERROR_TOKEN_DETECT;
}

std::optional<sai_attr_id_t>
SaiPortTraits::Attributes::AttributeRxLaneSquelchEnable::operator()() {
  return SAI_PORT_ATTR_RX_LANE_SQUELCH_ENABLE;
}

std::optional<sai_attr_id_t>
SaiPortTraits::Attributes::AttributeFabricSystemPort::operator()() {
  return SAI_PORT_ATTR_FABRIC_SYSTEM_PORT;
}

std::optional<sai_attr_id_t>
SaiPortSerdesTraits::Attributes::AttributeRVgaWrapper::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiPortSerdesTraits::Attributes::AttributeDcoWrapper::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiPortSerdesTraits::Attributes::AttributeFltMWrapper::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiPortSerdesTraits::Attributes::AttributeFltSWrapper::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiPortSerdesTraits::Attributes::AttributeRxPfWrapper::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiPortSerdesTraits::Attributes::AttributeRxEqP2Wrapper::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiPortSerdesTraits::Attributes::AttributeRxEqP1Wrapper::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiPortSerdesTraits::Attributes::AttributeRxEqMWrapper::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiPortSerdesTraits::Attributes::AttributeRxEq1Wrapper::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiPortSerdesTraits::Attributes::AttributeRxEq2Wrapper::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiPortSerdesTraits::Attributes::AttributeRxEq3Wrapper::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiPortSerdesTraits::Attributes::AttributeRxTap2Wrapper::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiPortSerdesTraits::Attributes::AttributeRxTap1Wrapper::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiPortSerdesTraits::Attributes::AttributeTpChn2Wrapper::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiPortSerdesTraits::Attributes::AttributeTpChn1Wrapper::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiPortSerdesTraits::Attributes::AttributeTpChn0Wrapper::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiPortTraits::Attributes::AttributeCablePropogationDelayNS::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiPortTraits::Attributes::AttributeFabricDataCellsFilterStatus::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiPortTraits::Attributes::AttributeReachabilityGroup::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiPortTraits::Attributes::AttributeFecErrorDetectEnable::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiPortTraits::Attributes::AttributePfcMonitorDirection::operator()() {
  return SAI_PORT_ATTR_PFC_MONITOR_DIRECTION;
}

std::optional<sai_attr_id_t>
SaiPortTraits::Attributes::AttributeAmIdles::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiPortTraits::Attributes::AttributeResetQueueCreditBalance::operator()() {
  return SAI_PORT_ATTR_RESET_QUEUE_CREDIT_BALANCE;
}

std::optional<sai_attr_id_t>
SaiPortTraits::Attributes::AttributePgDropStatus::operator()() {
  return SAI_PORT_ATTR_PORT_PG_PKT_DROP_STATUS;
}

std::optional<sai_attr_id_t>
SaiSwitchTraits::Attributes::AttributeRestartIssuWrapper::operator()() {
  return SAI_SWITCH_ATTR_EXT_RESTART_ISSU;
}

std::optional<sai_attr_id_t>
SaiSwitchTraits::Attributes::AttributeDelayDropCongThreshold::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t> SaiSwitchTraits::Attributes::
    AttributeForceTrafficOverFabricWrapper::operator()() {
  return SAI_SWITCH_ATTR_FORCE_TRAFFIC_OVER_FABRIC;
}

std::optional<sai_attr_id_t> SaiSwitchTraits::Attributes::
    AttributeWarmBootTargetVersionWrapper::operator()() {
  return SAI_SWITCH_ATTR_EXT_WARM_BOOT_TARGET_VERSION;
}

std::optional<sai_attr_id_t>
SaiSwitchTraits::Attributes::AttributeSwitchIsolateWrapper::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiSwitchTraits::Attributes::AttributeMaxCoresWrapper::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiSwitchTraits::Attributes::AttributeSdkBootTimeWrapper::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t> SaiSwitchTraits::Attributes::
    AttributeFabricRemoteReachablePortList::operator()() {
  return SAI_SWITCH_ATTR_FABRIC_REMOTE_REACHABLE_PORT_LIST;
}

std::optional<sai_attr_id_t> SaiSwitchTraits::Attributes::
    AttributeRouteNoImplicitMetaDataWrapper::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t> SaiSwitchTraits::Attributes::
    AttributeRouteAllowImplicitMetaDataWrapper::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t> SaiSwitchTraits::Attributes::
    AttributeMultiStageLocalSwitchIdsWrapper::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t> SaiStaticBufferProfileTraits::Attributes::
    AttributeSharedFadtMaxTh::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t> SaiStaticBufferProfileTraits::Attributes::
    AttributeSharedFadtMinTh::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiStaticBufferProfileTraits::Attributes::AttributeSramFadtMinTh::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiStaticBufferProfileTraits::Attributes::AttributeSramFadtMaxTh::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t> SaiStaticBufferProfileTraits::Attributes::
    AttributeSramFadtXonOffset::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiStaticBufferProfileTraits::Attributes::AttributeSramDynamicTh::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t> SaiStaticBufferProfileTraits::Attributes::
    AttributePgPipelineLatencyBytes::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t> SaiDynamicBufferProfileTraits::Attributes::
    AttributeSharedFadtMaxTh::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t> SaiDynamicBufferProfileTraits::Attributes::
    AttributeSharedFadtMinTh::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t> SaiDynamicBufferProfileTraits::Attributes::
    AttributeSramFadtMinTh::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t> SaiDynamicBufferProfileTraits::Attributes::
    AttributeSramFadtMaxTh::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t> SaiDynamicBufferProfileTraits::Attributes::
    AttributeSramFadtXonOffset::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t> SaiDynamicBufferProfileTraits::Attributes::
    AttributeSramDynamicTh::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t> SaiDynamicBufferProfileTraits::Attributes::
    AttributePgPipelineLatencyBytes::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t> SaiIngressPriorityGroupTraits::Attributes::
    AttributeLosslessEnable::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiSflowMirrorTraits::Attributes::AttributeTcBufferLimit::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiLocalMirrorTraits::Attributes::AttributeTcBufferLimit::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t> SaiEnhancedRemoteMirrorTraits::Attributes::
    AttributeTcBufferLimit::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiSwitchTraits::Attributes::AttributeNoAclsForTrapsWrapper::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiTxPacketTraits::Attributes::AttributePacketType::operator()() {
  return std::nullopt;
}

const std::vector<sai_stat_id_t>& SaiSwitchTraits::dramStats() {
  static const std::vector<sai_stat_id_t> stats;
  return stats;
}

const std::vector<sai_stat_id_t>& SaiSwitchTraits::rciWatermarkStats() {
  static const std::vector<sai_stat_id_t> stats;
  return stats;
}

const std::vector<sai_stat_id_t>& SaiSwitchTraits::dtlWatermarkStats() {
  static const std::vector<sai_stat_id_t> stats;
  return stats;
}

const std::vector<sai_stat_id_t>& SaiSwitchTraits::dramBlockTime() {
  static const std::vector<sai_stat_id_t> stats;
  return stats;
}

const std::vector<sai_stat_id_t>&
SaiSwitchTraits::dramQuarantinedBufferStats() {
  static const std::vector<sai_stat_id_t> stats;
  return stats;
}

const std::vector<sai_stat_id_t>&
SaiSwitchTraits::fabricInterCellJitterWatermarkStats() {
  static const std::vector<sai_stat_id_t> stats;
  return stats;
}

const std::vector<sai_stat_id_t>& SaiSwitchTraits::deletedCredits() {
  static const std::vector<sai_stat_id_t> stats;
  return stats;
}

const std::vector<sai_stat_id_t>&
SaiSwitchTraits::egressCoreBufferWatermarkBytes() {
  static const std::vector<sai_stat_id_t> stats;
  return stats;
}

const std::vector<sai_stat_id_t>& SaiQueueTraits::egressGvoqWatermarkBytes() {
  static const std::vector<sai_stat_id_t> stats;
  return stats;
}

const std::vector<sai_stat_id_t>&
SaiSwitchTraits::sramMinBufferWatermarkBytes() {
  static const std::vector<sai_stat_id_t> stats;
  return stats;
}

const std::vector<sai_stat_id_t>& SaiSwitchTraits::fdrFifoWatermarkBytes() {
  static const std::vector<sai_stat_id_t> stats;
  return stats;
}

const std::vector<sai_stat_id_t>& SaiSwitchTraits::egressFabricCellError() {
  static const std::vector<sai_stat_id_t> stats;
  return stats;
}

const std::vector<sai_stat_id_t>& SaiSwitchTraits::egressNonFabricCellError() {
  static const std::vector<sai_stat_id_t> stats;
  return stats;
}

const std::vector<sai_stat_id_t>&
SaiSwitchTraits::egressNonFabricCellUnpackError() {
  static const std::vector<sai_stat_id_t> stats;
  return stats;
}

const std::vector<sai_stat_id_t>& SaiSwitchTraits::egressParityCellError() {
  static const std::vector<sai_stat_id_t> stats;
  return stats;
}

const std::vector<sai_stat_id_t>& SaiSwitchTraits::ddpPacketError() {
  static const std::vector<sai_stat_id_t> stats;
  return stats;
}

const std::vector<sai_stat_id_t>& SaiSwitchTraits::packetIntegrityError() {
  static const std::vector<sai_stat_id_t> stats;
  return stats;
}

std::optional<sai_attr_id_t>
SaiSwitchTraits::Attributes::AttributeVoqLatencyMinLocalNs::operator()() {
  return SAI_SWITCH_ATTR_VOQ_LATENCY_MIN_LOCAL;
}

std::optional<sai_attr_id_t>
SaiSwitchTraits::Attributes::AttributeVoqLatencyMaxLocalNs::operator()() {
  return SAI_SWITCH_ATTR_VOQ_LATENCY_MAX_LOCAL;
}

std::optional<sai_attr_id_t>
SaiSwitchTraits::Attributes::AttributeVoqLatencyMinLevel1Ns::operator()() {
  return SAI_SWITCH_ATTR_VOQ_LATENCY_MIN_LEVEL_1;
}

std::optional<sai_attr_id_t>
SaiSwitchTraits::Attributes::AttributeVoqLatencyMaxLevel1Ns::operator()() {
  return SAI_SWITCH_ATTR_VOQ_LATENCY_MAX_LEVEL_1;
}

std::optional<sai_attr_id_t>
SaiSwitchTraits::Attributes::AttributeVoqLatencyMinLevel2Ns::operator()() {
  return SAI_SWITCH_ATTR_VOQ_LATENCY_MIN_LEVEL_2;
}

std::optional<sai_attr_id_t>
SaiSwitchTraits::Attributes::AttributeVoqLatencyMaxLevel2Ns::operator()() {
  return SAI_SWITCH_ATTR_VOQ_LATENCY_MAX_LEVEL_2;
}

std::optional<sai_attr_id_t>
SaiSwitchTraits::Attributes::AttributeReachabilityGroupList::operator()() {
  return SAI_SWITCH_ATTR_REACHABILITY_GROUP_LIST;
}

std::optional<sai_attr_id_t> SaiSwitchTraits::Attributes::
    AttributeFabricLinkLayerFlowControlThreshold::operator()() {
  return SAI_SWITCH_ATTR_FABRIC_LLFC_THRESHOLD;
}

std::optional<sai_attr_id_t> SaiSwitchTraits::Attributes::
    AttributeSramFreePercentXoffThWrapper::operator()() {
  return SAI_SWITCH_ATTR_SRAM_FREE_PERCENT_XOFF_TH;
}

std::optional<sai_attr_id_t> SaiSwitchTraits::Attributes::
    AttributeSramFreePercentXonThWrapper::operator()() {
  return SAI_SWITCH_ATTR_SRAM_FREE_PERCENT_XON_TH;
}

std::optional<sai_attr_id_t> SaiSwitchTraits::Attributes::
    AttributeFabricCllfcTxCreditThWrapper::operator()() {
  return SAI_SWITCH_ATTR_FABRIC_CLLFC_TX_CREDIT_TH;
}

std::optional<sai_attr_id_t>
SaiSwitchTraits::Attributes::AttributeVoqDramBoundThWrapper::operator()() {
  return SAI_SWITCH_ATTR_VOQ_DRAM_BOUND_TH;
}

std::optional<sai_attr_id_t>
SaiSwitchTraits::Attributes::AttributeCondEntropyRehashPeriodUS::operator()() {
  return SAI_SWITCH_ATTR_COND_ENTROPY_REHASH_PERIOD_US;
}

std::optional<sai_attr_id_t>
SaiSwitchTraits::Attributes::AttributeShelSrcIp::operator()() {
  return SAI_SWITCH_ATTR_SHEL_SRC_IP;
}

std::optional<sai_attr_id_t>
SaiSwitchTraits::Attributes::AttributeShelDstIp::operator()() {
  return SAI_SWITCH_ATTR_SHEL_DST_IP;
}

std::optional<sai_attr_id_t>
SaiSwitchTraits::Attributes::AttributeShelSrcMac::operator()() {
  return SAI_SWITCH_ATTR_SHEL_SRC_MAC;
}

std::optional<sai_attr_id_t>
SaiSwitchTraits::Attributes::AttributeShelPeriodicInterval::operator()() {
  return SAI_SWITCH_ATTR_SHEL_PERIODIC_INTERVAL;
}

std::optional<sai_attr_id_t>
SaiSwitchTraits::Attributes::AttributeSflowAggrNofSamplesWrapper::operator()() {
  return SAI_SWITCH_ATTR_SFLOW_AGGR_NOF_SAMPLES;
}

std::optional<sai_attr_id_t>
SaiPortSerdesTraits::Attributes::AttributeTxDiffEncoderEnWrapper::operator()() {
  return SAI_PORT_SERDES_ATTR_EXT_TX_DIFF_ENCODER_EN;
}

std::optional<sai_attr_id_t>
SaiPortSerdesTraits::Attributes::AttributeTxDigGainWrapper::operator()() {
  return SAI_PORT_SERDES_ATTR_EXT_TX_DIG_GAIN;
}

std::optional<sai_attr_id_t>
SaiPortSerdesTraits::Attributes::AttributeTxFfeCoeff0Wrapper::operator()() {
  return SAI_PORT_SERDES_ATTR_EXT_TX_FFE_COEFF_0;
}

std::optional<sai_attr_id_t>
SaiPortSerdesTraits::Attributes::AttributeTxFfeCoeff1Wrapper::operator()() {
  return SAI_PORT_SERDES_ATTR_EXT_TX_FFE_COEFF_1;
}

std::optional<sai_attr_id_t>
SaiPortSerdesTraits::Attributes::AttributeTxFfeCoeff2Wrapper::operator()() {
  return SAI_PORT_SERDES_ATTR_EXT_TX_FFE_COEFF_2;
}

std::optional<sai_attr_id_t>
SaiPortSerdesTraits::Attributes::AttributeTxFfeCoeff3Wrapper::operator()() {
  return SAI_PORT_SERDES_ATTR_EXT_TX_FFE_COEFF_3;
}

std::optional<sai_attr_id_t>
SaiPortSerdesTraits::Attributes::AttributeTxFfeCoeff4Wrapper::operator()() {
  return SAI_PORT_SERDES_ATTR_EXT_TX_FFE_COEFF_4;
}

std::optional<sai_attr_id_t>
SaiPortSerdesTraits::Attributes::AttributeTxDriverSwingWrapper::operator()() {
  return SAI_PORT_SERDES_ATTR_EXT_TX_DRIVER_SWING;
}

std::optional<sai_attr_id_t>
SaiPortSerdesTraits::Attributes::AttributeTxLdoBypassWrapper::operator()() {
  return SAI_PORT_SERDES_ATTR_EXT_TX_LDO_BYPASS;
}

std::optional<sai_attr_id_t> SaiPortSerdesTraits::Attributes::
    AttributeRxInstgBoost1StartWrapper::operator()() {
  return SAI_PORT_SERDES_ATTR_EXT_RX_INSTG_BOOST1_STRT;
}

std::optional<sai_attr_id_t> SaiPortSerdesTraits::Attributes::
    AttributeRxInstgBoost1StepWrapper::operator()() {
  return SAI_PORT_SERDES_ATTR_EXT_RX_INSTG_BOOST1_STEP;
}

std::optional<sai_attr_id_t> SaiPortSerdesTraits::Attributes::
    AttributeRxInstgBoost1StopWrapper::operator()() {
  return SAI_PORT_SERDES_ATTR_EXT_RX_INSTG_BOOST1_STOP;
}

std::optional<sai_attr_id_t> SaiPortSerdesTraits::Attributes::
    AttributeRxInstgBoost2OrHrStartWrapper::operator()() {
  return SAI_PORT_SERDES_ATTR_EXT_RX_INSTG_BOOST2_OR_HR_STRT;
}

std::optional<sai_attr_id_t> SaiPortSerdesTraits::Attributes::
    AttributeRxInstgBoost2OrHrStepWrapper::operator()() {
  return SAI_PORT_SERDES_ATTR_EXT_RX_INSTG_BOOST2_OR_HR_STEP;
}

std::optional<sai_attr_id_t> SaiPortSerdesTraits::Attributes::
    AttributeRxInstgBoost2OrHrStopWrapper::operator()() {
  return SAI_PORT_SERDES_ATTR_EXT_RX_INSTG_BOOST2_OR_HR_STOP;
}

std::optional<sai_attr_id_t> SaiPortSerdesTraits::Attributes::
    AttributeRxInstgC1Start1p7Wrapper::operator()() {
  return SAI_PORT_SERDES_ATTR_EXT_RX_INSTG_C1_START_1P7;
}

std::optional<sai_attr_id_t> SaiPortSerdesTraits::Attributes::
    AttributeRxInstgC1Step1p7Wrapper::operator()() {
  return SAI_PORT_SERDES_ATTR_EXT_RX_INSTG_C1_STEP_1P7;
}

std::optional<sai_attr_id_t> SaiPortSerdesTraits::Attributes::
    AttributeRxInstgC1Stop1p7Wrapper::operator()() {
  return SAI_PORT_SERDES_ATTR_EXT_RX_INSTG_C1_STOP_1P7;
}

std::optional<sai_attr_id_t> SaiPortSerdesTraits::Attributes::
    AttributeRxInstgDfeStart1p7Wrapper::operator()() {
  return SAI_PORT_SERDES_ATTR_EXT_RX_INSTG_DFE_START_1P7;
}

std::optional<sai_attr_id_t> SaiPortSerdesTraits::Attributes::
    AttributeRxInstgDfeStep1p7Wrapper::operator()() {
  return SAI_PORT_SERDES_ATTR_EXT_RX_INSTG_DFE_STEP_1P7;
}

std::optional<sai_attr_id_t> SaiPortSerdesTraits::Attributes::
    AttributeRxInstgDfeStop1p7Wrapper::operator()() {
  return SAI_PORT_SERDES_ATTR_EXT_RX_INSTG_DFE_STOP_1P7;
}

std::optional<sai_attr_id_t> SaiPortSerdesTraits::Attributes::
    AttributeRxEnableScanSelectionWrapper ::operator()() {
  return SAI_PORT_SERDES_ATTR_EXT_RX_INSTG_ENABLE_SCAN;
}
std::optional<sai_attr_id_t> SaiPortSerdesTraits::Attributes::
    AttributeRxInstgScanUseSrSettingsWrapper ::operator()() {
  return SAI_PORT_SERDES_ATTR_EXT_RX_INSTG_SCAN_USE_SR_SETTINGS;
}
std::optional<sai_attr_id_t>
SaiPortSerdesTraits::Attributes::AttributeRxCdrCfgOvEnWrapper ::operator()() {
  return SAI_PORT_SERDES_ATTR_EXT_RX_CDR_CFG_OV_EN;
}
std::optional<sai_attr_id_t> SaiPortSerdesTraits::Attributes::
    AttributeRxCdrTdet1stOrdStepOvValWrapper ::operator()() {
  return SAI_PORT_SERDES_ATTR_EXT_RX_CDR_TDET_1ST_ORD_STEP_OV_VAL;
}
std::optional<sai_attr_id_t> SaiPortSerdesTraits::Attributes::
    AttributeRxCdrTdet2ndOrdStepOvValWrapper ::operator()() {
  return SAI_PORT_SERDES_ATTR_EXT_RX_CDR_TDET_2ND_ORD_STEP_OV_VAL;
}
std::optional<sai_attr_id_t> SaiPortSerdesTraits::Attributes::
    AttributeRxCdrTdetFineStepOvValWrapper ::operator()() {
  return SAI_PORT_SERDES_ATTR_EXT_RX_CDR_TDET_FINE_STEP_OV_VAL;
}

std::optional<sai_attr_id_t>
SaiPortSerdesTraits::Attributes::AttributeRxLdoBypassWrapper::operator()() {
  return SAI_PORT_SERDES_ATTR_EXT_RX_LDO_BYPASS;
}

std::optional<sai_attr_id_t>
SaiPortSerdesTraits::Attributes::AttributeRxDiffEncoderEnWrapper::operator()() {
  return SAI_PORT_SERDES_ATTR_EXT_RX_DIFF_ENCODER_EN;
}

std::optional<sai_attr_id_t> SaiPortSerdesTraits::Attributes::
    AttributeRxInstgEnableScanWrapper::operator()() {
  return SAI_PORT_SERDES_ATTR_EXT_RX_INSTG_ENABLE_SCAN;
}

std::optional<sai_attr_id_t> SaiPortSerdesTraits::Attributes::
    AttributeRxFfeLengthBitmapWrapper::operator()() {
  return SAI_PORT_SERDES_ATTR_EXT_RX_FFE_LENGTH_BITMAP;
}

std::optional<sai_attr_id_t> SaiPortSerdesTraits::Attributes::
    AttributeRxFfeLmsDynamicGatingEnWrapper::operator()() {
  return SAI_PORT_SERDES_ATTR_EXT_RX_FFE_LMS_DYNAMIC_GATING_EN;
}

std::optional<sai_attr_id_t>
SaiPortTraits::Attributes::AttributeCondEntropyRehashEnable::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiPortTraits::Attributes::AttributeCondEntropyRehashPeriodUS::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiPortTraits::Attributes::AttributeCondEntropyRehashSeed::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiSwitchTraits::Attributes::AttributeMaxSystemPortId::operator()() {
  return SAI_SWITCH_ATTR_MAX_SYSTEM_PORT_ID;
}

std::optional<sai_attr_id_t>
SaiSwitchTraits::Attributes::AttributeMaxLocalSystemPortId::operator()() {
  return SAI_SWITCH_ATTR_MAX_LOCAL_SYSTEM_PORT_ID;
}

std::optional<sai_attr_id_t>
SaiSwitchTraits::Attributes::AttributeMaxSystemPorts::operator()() {
  return SAI_SWITCH_ATTR_MAX_SYSTEM_PORTS;
}

std::optional<sai_attr_id_t>
SaiSwitchTraits::Attributes::AttributeMaxVoqs::operator()() {
  return SAI_SWITCH_ATTR_MAX_VOQS;
}

std::optional<sai_attr_id_t>
SaiPortTraits::Attributes::AttributeShelEnable::operator()() {
  return std::nullopt;
}

const std::vector<sai_stat_id_t>&
SaiPortTraits::macTxDataQueueMinWatermarkStats() {
  static const std::vector<sai_stat_id_t> stats;
  return stats;
}

const std::vector<sai_stat_id_t>&
SaiPortTraits::macTxDataQueueMaxWatermarkStats() {
  static const std::vector<sai_stat_id_t> stats;
  return stats;
}

const std::vector<sai_stat_id_t>& SaiPortTraits::fabricControlRxPacketStats() {
  static const std::vector<sai_stat_id_t> stats;
  return stats;
}

const std::vector<sai_stat_id_t>& SaiPortTraits::fabricControlTxPacketStats() {
  static const std::vector<sai_stat_id_t> stats;
  return stats;
}

const std::vector<sai_stat_id_t>& SaiPortTraits::pfcXoffTotalDurationStats() {
  static const std::vector<sai_stat_id_t> stats;
  return stats;
}

std::optional<sai_attr_id_t>
SaiSystemPortTraits::Attributes::AttributeShelPktDstEnable::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiSystemPortTraits::Attributes::AttributeTcRateLimitExclude::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiSwitchTraits::Attributes::AttributeFirmwareCoreTouse::operator()() {
  return SAI_SWITCH_ATTR_FIRMWARE_CORE_TO_USE;
}

std::optional<sai_attr_id_t>
SaiSwitchTraits::Attributes::AttributeFirmwareLogFile::operator()() {
  return SAI_SWITCH_ATTR_FIRMWARE_LOG_PATH_NAME;
}

std::optional<sai_attr_id_t>
SaiSwitchTraits::Attributes::AttributeMaxSwitchId::operator()() {
  return SAI_SWITCH_ATTR_MAX_SWITCH_ID;
}

std::optional<sai_attr_id_t>
SaiSwitchTraits::Attributes::AttributeArsAvailableFlows::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiSystemPortTraits::Attributes::AttributePushQueueEnable::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiSwitchTraits::Attributes::AttributeSdkRegDumpLogPath::operator()() {
  return SAI_SWITCH_ATTR_SDK_DUMP_LOG_PATH_NAME;
}

std::optional<sai_attr_id_t>
SaiSwitchTraits::Attributes::AttributeFirmwareObjectList::operator()() {
  return SAI_SWITCH_ATTR_FIRMWARE_OBJECTS;
}

std::optional<sai_attr_id_t>
SaiSwitchTraits::Attributes::AttributeTcRateLimitList::operator()() {
  return SAI_SWITCH_ATTR_TC_RATE_LIMIT_LIST;
}

std::optional<sai_attr_id_t>
SaiSwitchTraits::Attributes::AttributeTechSupportType::operator()() {
  return SAI_SWITCH_ATTR_TECH_SUPPORT_TYPE;
}

std::optional<sai_attr_id_t> SaiSwitchTraits::Attributes::
    AttributePfcTcDldTimerGranularityInterval::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiSwitchTraits::Attributes::AttributeNumberOfPipes::operator()() {
  return SAI_SWITCH_ATTR_NUMBER_OF_PIPES;
}

std::optional<sai_attr_id_t>
SaiSwitchTraits::Attributes::AttributePipelineObjectList::operator()() {
  return SAI_SWITCH_ATTR_PIPELINE_OBJECTS;
}

std::optional<sai_attr_id_t>
SaiSwitchTraits::Attributes::AttributeDisableSllAndHllTimeout::operator()() {
  return SAI_SWITCH_ATTR_DISABLE_SLL_AND_HLL_TIMEOUT;
}

std::optional<sai_attr_id_t>
SaiSwitchTraits::Attributes::AttributeAsicRevision::operator()() {
  return SAI_SWITCH_ATTR_ASIC_REVISION;
}

std::optional<sai_attr_id_t> SaiSwitchTraits::Attributes::
    AttributeCreditRequestProfileSchedulerMode::operator()() {
  return SAI_SWITCH_ATTR_CREDIT_REQUEST_PROFILE_SCHEDULER_MODE;
}

std::optional<sai_attr_id_t> SaiSwitchTraits::Attributes::
    AttributeModuleIdToCreditRequestProfileParamList::operator()() {
  return SAI_SWITCH_ATTR_MODULE_ID_TO_CREDIT_REQUEST_PROFILE_PARAM_LIST;
}

std::optional<sai_attr_id_t> SaiSwitchTraits::Attributes::
    AttributeTriggerSimulatedEccCorrectableError::operator()() {
  return SAI_SWITCH_ATTR_TRIGGER_SIMULATED_ECC_CORRECTABLE_ERROR;
}

std::optional<sai_attr_id_t> SaiSwitchTraits::Attributes::
    AttributeTriggerSimulatedEccUnCorrectableError::operator()() {
  return SAI_SWITCH_ATTR_TRIGGER_SIMULATED_ECC_UNCORRECTABLE_ERROR;
}
std::optional<sai_attr_id_t>
SaiSwitchTraits::Attributes::AttributeDefaultCpuEgressBufferPool::operator()() {
  return SAI_SWITCH_ATTR_DEFAULT_CPU_EGRESS_BUFFER_POOL;
}

std::optional<sai_attr_id_t>
SaiSwitchTraits::Attributes::AttributeModuleIdFabricPortList::operator()() {
  return SAI_SWITCH_ATTR_MODULE_ID_FABRIC_PORT_LIST;
}

std::optional<sai_attr_id_t>
SaiSwitchTraits::Attributes::AttributePfcMonitorEnable::operator()() {
  return SAI_SWITCH_ATTR_PFC_MONITOR_ENABLE;
}

std::optional<sai_attr_id_t> SaiSwitchTraits::Attributes::
    AttributeCablePropagationDelayMeasurement::operator()() {
  return SAI_SWITCH_ATTR_CABLE_PROPAGATION_DELAY_MEASUREMENT;
}
#if SAI_API_VERSION >= SAI_VERSION(1, 16, 0)
std::optional<sai_attr_id_t>
SaiArsProfileTraits::Attributes::AttributeArsMaxGroups::operator()() {
  return SAI_ARS_PROFILE_ATTR_EXTENSION_ECMP_ARS_MAX_GROUPS;
}

std::optional<sai_attr_id_t> SaiArsProfileTraits::Attributes::
    AttributeArsAlternateMembersRouteMetaData::operator()() {
  return SAI_ARS_PROFILE_ATTR_ROUTE_ARS_ALTERNATE_MEMBERS_META_DATA;
}

std::optional<sai_attr_id_t>
SaiArsProfileTraits::Attributes::AttributeArsRouteMetaDataMask::operator()() {
  return SAI_ARS_PROFILE_ATTR_ROUTE_ARS_META_DATA_MASK;
}

std::optional<sai_attr_id_t> SaiArsProfileTraits::Attributes::
    AttributeArsPrimaryMembersRouteMetaData::operator()() {
  return SAI_ARS_PROFILE_ATTR_ROUTE_ARS_PRIMARY_MEMBERS_META_DATA;
}

std::optional<sai_attr_id_t>
SaiArsProfileTraits::Attributes::AttributeArsBaseIndex::operator()() {
  return SAI_ARS_PROFILE_ATTR_EXTENSION_ECMP_ARS_BASE_INDEX;
}

std::optional<sai_attr_id_t> SaiNextHopGroupTraits::Attributes::
    AttributeArsNextHopGroupMetaData::operator()() {
  return SAI_NEXT_HOP_GROUP_ATTR_ARS_NEXT_HOP_GROUP_META_DATA;
}

std::optional<sai_attr_id_t>
SaiAclEntryTraits::Attributes::AttributeActionL3SwitchCancel::operator()() {
  return SAI_ACL_ENTRY_ATTR_ACTION_L3_SWITCH_CANCEL;
}
#endif

} // namespace facebook::fboss
