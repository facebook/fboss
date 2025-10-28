// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/api/PortApi.h"

extern "C" {
#include <sai.h>

#ifndef IS_OSS_BRCM_SAI
#include <experimental/saiportextensions.h>
#else
#include <saiportextensions.h>
#endif
}

namespace facebook::fboss {

std::optional<sai_attr_id_t>
SaiPortTraits::Attributes::AttributeSerdesLaneList::operator()() {
#if defined(BRCM_SAI_SDK_XGS)
  return SAI_PORT_ATTR_SERDES_LANE_LIST;
#else
  return std::nullopt;
#endif
}

std::optional<sai_attr_id_t>
SaiPortTraits::Attributes::AttributeDiagModeEnable::operator()() {
#if defined(BRCM_SAI_SDK_XGS)
  return SAI_PORT_ATTR_DIAGNOSTICS_MODE_ENABLE;
#else
  return std::nullopt;
#endif
}

std::optional<sai_attr_id_t>
SaiPortTraits::Attributes::AttributeFdrEnable::operator()() {
#if defined(BRCM_SAI_SDK_GTE_10_0) || defined(BRCM_SAI_SDK_DNX_GTE_11_0)
  return SAI_PORT_ATTR_FDR_ENABLE;
#else
  return std::nullopt;
#endif
}

std::optional<sai_attr_id_t>
SaiPortTraits::Attributes::AttributeRxLaneSquelchEnable::operator()() {
#if defined(BRCM_SAI_SDK_GTE_9_2) && !defined(SAI_VERSION_10_0_EA_DNX_SIM_ODP)
  return SAI_PORT_ATTR_RX_LANE_SQUELCH_ENABLE;
#else
  return std::nullopt;
#endif
}

std::optional<sai_attr_id_t>
SaiPortSerdesTraits::Attributes::AttributeRVgaWrapper::operator()() {
#if defined(BRCM_SAI_SDK_GTE_13_0)
  return SAI_PORT_SERDES_ATTR_PHY_DIAG_RVG;
#else
  return std::nullopt;
#endif
}

std::optional<sai_attr_id_t>
SaiPortSerdesTraits::Attributes::AttributeDcoWrapper::operator()() {
#if defined(BRCM_SAI_SDK_GTE_13_0)
  return SAI_PORT_SERDES_ATTR_PHY_DIAG_DCO;
#else
  return std::nullopt;
#endif
}

std::optional<sai_attr_id_t>
SaiPortSerdesTraits::Attributes::AttributeFltMWrapper::operator()() {
#if defined(BRCM_SAI_SDK_GTE_13_0)
  return SAI_PORT_SERDES_ATTR_PHY_DIAG_FLT_M;
#else
  return std::nullopt;
#endif
}

std::optional<sai_attr_id_t>
SaiPortSerdesTraits::Attributes::AttributeFltSWrapper::operator()() {
#if defined(BRCM_SAI_SDK_GTE_13_0)
  return SAI_PORT_SERDES_ATTR_PHY_DIAG_FLT_S;
#else
  return std::nullopt;
#endif
}

std::optional<sai_attr_id_t>
SaiPortSerdesTraits::Attributes::AttributeRxPfWrapper::operator()() {
#if defined(BRCM_SAI_SDK_GTE_13_0)
  return SAI_PORT_SERDES_ATTR_PHY_DIAG_RX_PF;
#else
  return std::nullopt;
#endif
}

std::optional<sai_attr_id_t>
SaiPortSerdesTraits::Attributes::AttributeRxEqP2Wrapper::operator()() {
#if defined(BRCM_SAI_SDK_GTE_13_0)
  return SAI_PORT_SERDES_ATTR_PHY_DIAG_RX_EQ_P2;
#else
  return std::nullopt;
#endif
}

std::optional<sai_attr_id_t>
SaiPortSerdesTraits::Attributes::AttributeRxEqP1Wrapper::operator()() {
#if defined(BRCM_SAI_SDK_GTE_13_0)
  return SAI_PORT_SERDES_ATTR_PHY_DIAG_RX_EQ_P1;
#else
  return std::nullopt;
#endif
}

std::optional<sai_attr_id_t>
SaiPortSerdesTraits::Attributes::AttributeRxEqMWrapper::operator()() {
#if defined(BRCM_SAI_SDK_GTE_13_0)
  return SAI_PORT_SERDES_ATTR_PHY_DIAG_RX_EQ_M;
#else
  return std::nullopt;
#endif
}

std::optional<sai_attr_id_t>
SaiPortSerdesTraits::Attributes::AttributeRxEq1Wrapper::operator()() {
#if defined(BRCM_SAI_SDK_GTE_13_0)
  return SAI_PORT_SERDES_ATTR_PHY_DIAG_RX_EQ_1;
#else
  return std::nullopt;
#endif
}

std::optional<sai_attr_id_t>
SaiPortSerdesTraits::Attributes::AttributeRxEq2Wrapper::operator()() {
#if defined(BRCM_SAI_SDK_GTE_13_0)
  return SAI_PORT_SERDES_ATTR_PHY_DIAG_RX_EQ_2;
#else
  return std::nullopt;
#endif
}

std::optional<sai_attr_id_t>
SaiPortSerdesTraits::Attributes::AttributeRxEq3Wrapper::operator()() {
#if defined(BRCM_SAI_SDK_GTE_13_0)
  return SAI_PORT_SERDES_ATTR_PHY_DIAG_RX_EQ_3;
#else
  return std::nullopt;
#endif
}

std::optional<sai_attr_id_t>
SaiPortSerdesTraits::Attributes::AttributeRxTap2Wrapper::operator()() {
#if defined(BRCM_SAI_SDK_GTE_13_0)
  return SAI_PORT_SERDES_ATTR_PHY_DIAG_RX_TAP_2;
#else
  return std::nullopt;
#endif
}

std::optional<sai_attr_id_t>
SaiPortSerdesTraits::Attributes::AttributeRxTap1Wrapper::operator()() {
#if defined(BRCM_SAI_SDK_GTE_13_0)
  return SAI_PORT_SERDES_ATTR_PHY_DIAG_RX_TAP_1;
#else
  return std::nullopt;
#endif
}

std::optional<sai_attr_id_t>
SaiPortSerdesTraits::Attributes::AttributeTpChn2Wrapper::operator()() {
#if defined(BRCM_SAI_SDK_GTE_13_0)
  return SAI_PORT_SERDES_ATTR_PHY_DIAG_TP_CHN_2;
#else
  return std::nullopt;
#endif
}

std::optional<sai_attr_id_t>
SaiPortSerdesTraits::Attributes::AttributeTpChn1Wrapper::operator()() {
#if defined(BRCM_SAI_SDK_GTE_13_0)
  return SAI_PORT_SERDES_ATTR_PHY_DIAG_TP_CHN_1;
#else
  return std::nullopt;
#endif
}

std::optional<sai_attr_id_t>
SaiPortSerdesTraits::Attributes::AttributeTpChn0Wrapper::operator()() {
#if defined(BRCM_SAI_SDK_GTE_13_0)
  return SAI_PORT_SERDES_ATTR_PHY_DIAG_TP_CHN_0;
#else
  return std::nullopt;
#endif
}

std::optional<sai_attr_id_t>
SaiPortTraits::Attributes::AttributeFabricSystemPort::operator()() {
#if defined(BRCM_SAI_SDK_DNX_GTE_13_0)
  return SAI_PORT_ATTR_FABRIC_SYSTEM_PORT;
#else
  return std::nullopt;
#endif
}

std::optional<sai_attr_id_t>
SaiPortSerdesTraits::Attributes::AttributeTxLutModeIdWrapper::operator()() {
  return std::nullopt;
}
std::optional<sai_attr_id_t>
SaiPortSerdesTraits::Attributes::AttributeRxCtleCodeIdWrapper::operator()() {
  return std::nullopt;
}
std::optional<sai_attr_id_t>
SaiPortSerdesTraits::Attributes::AttributeRxDspModeIdWrapper::operator()() {
  return std::nullopt;
}
std::optional<sai_attr_id_t>
SaiPortSerdesTraits::Attributes::AttributeRxAfeTrimIdWrapper::operator()() {
  return std::nullopt;
}
std::optional<sai_attr_id_t> SaiPortSerdesTraits::Attributes::
    AttributeRxAcCouplingBypassIdWrapper::operator()() {
  return std::nullopt;
}
std::optional<sai_attr_id_t>
SaiPortTraits::Attributes::AttributeSystemPortId::operator()() {
  return std::nullopt;
}
std::optional<sai_attr_id_t> SaiPortSerdesTraits::Attributes::
    AttributeRxAfeAdaptiveEnableWrapper::operator()() {
  return std::nullopt;
}
std::optional<sai_attr_id_t>
SaiPortTraits::Attributes::AttributeCrcErrorDetect::operator()() {
#if defined(BRCM_SAI_SDK_GTE_11_0)
  return SAI_PORT_ATTR_CRC_ERROR_TOKEN_DETECT;
#else
  return std::nullopt;
#endif
}

std::optional<sai_attr_id_t>
SaiPortTraits::Attributes::AttributeCablePropogationDelayNS::operator()() {
#if defined(BRCM_SAI_SDK_GTE_11_0) && defined(BRCM_SAI_SDK_DNX)
  return SAI_PORT_ATTR_CABLE_PROPAGATION_DELAY;
#else
  return std::nullopt;
#endif
}

std::optional<sai_attr_id_t>
SaiPortTraits::Attributes::AttributeFabricDataCellsFilterStatus::operator()() {
#if defined(BRCM_SAI_SDK_GTE_11_0) && defined(BRCM_SAI_SDK_DNX)
  return SAI_PORT_ATTR_FABRIC_DATA_CELL_FILTER_STATUS;
#else
  return std::nullopt;
#endif
}

std::optional<sai_attr_id_t>
SaiPortTraits::Attributes::AttributeReachabilityGroup::operator()() {
#if defined(BRCM_SAI_SDK_GTE_12_0) && defined(BRCM_SAI_SDK_DNX)
  return SAI_PORT_ATTR_REACHABILITY_GROUP;
#else
  return std::nullopt;
#endif
}

std::optional<sai_attr_id_t>
SaiPortTraits::Attributes::AttributeFecErrorDetectEnable::operator()() {
#if defined(BRCM_SAI_SDK_DNX_GTE_11_7)
  return SAI_PORT_ATTR_FEC_ERROR_DETECT_ENABLE;
#else
  return std::nullopt;
#endif
}

std::optional<sai_attr_id_t>
SaiPortTraits::Attributes::AttributePfcMonitorDirection::operator()() {
#if defined(BRCM_SAI_SDK_GTE_13_0) && !defined(BRCM_SAI_SDK_DNX)
  // Some ASICs can support PFC Monitoring only in one direction. This
  // attribute specifies the direction to enable PFC monitoring for,
  // either RX or TX.
  return SAI_PORT_ATTR_PFC_MONITOR_DIRECTION;
#else
  return std::nullopt;
#endif
}

std::optional<sai_attr_id_t>
SaiPortTraits::Attributes::AttributeAmIdles::operator()() {
#if (                                                                          \
    defined(SAI_VERSION_11_7_0_0_ODP) || defined(BRCM_SAI_SDK_XGS_GTE_13_0) || \
    defined(BRCM_SAI_SDK_DNX_GTE_12_0)) &&                                     \
    !defined(SAI_VERSION_14_0_EA_ODP) && !defined(BRCM_SAI_SDK_DNX_GTE_14_0)
  return SAI_PORT_ATTR_EXTENSION_AM_IDLES;
#else
  return std::nullopt;
#endif
}

std::optional<sai_attr_id_t>
SaiPortTraits::Attributes::AttributeResetQueueCreditBalance::operator()() {
#if defined(BRCM_SAI_SDK_DNX_GTE_13_0) && !defined(BRCM_SAI_SDK_DNX_GTE_14_0)
  // The initial credit in VoQ switch ports allows a lot of packets to egress
  // after TX is disabled on the port. This results in inaccuracies in some
  // tests which need TX to be disabled before specific sequence is done. This
  // new SAI attribute provides an option to reset the credit balance on a per
  // port basis which will stop TX on a port after ~400+ 256B packets.
  return SAI_PORT_ATTR_RESET_QUEUE_CREDIT_BALANCE;
#else
  return std::nullopt;
#endif
}

std::optional<sai_attr_id_t>
SaiPortTraits::Attributes::AttributePgDropStatus::operator()() {
#if defined(BRCM_SAI_SDK_GTE_13_0) && !defined(BRCM_SAI_SDK_DNX)
  return SAI_PORT_ATTR_PORT_PG_PKT_DROP_STATUS;
#else
  return std::nullopt;
#endif
}

std::optional<sai_attr_id_t>
SaiPortTraits::Attributes::AttributeStaticModuleId::operator()() {
#if defined(BRCM_SAI_SDK_DNX_GTE_13_0)
  return SAI_PORT_ATTR_STATIC_MODULE_ID;
#else
  return std::nullopt;
#endif
}

std::optional<sai_attr_id_t>
SaiPortSerdesTraits::Attributes::AttributeTxDiffEncoderEnWrapper::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiPortSerdesTraits::Attributes::AttributeTxDigGainWrapper::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiPortSerdesTraits::Attributes::AttributeTxFfeCoeff0Wrapper::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiPortSerdesTraits::Attributes::AttributeTxFfeCoeff1Wrapper::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiPortSerdesTraits::Attributes::AttributeTxFfeCoeff2Wrapper::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiPortSerdesTraits::Attributes::AttributeTxFfeCoeff3Wrapper::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiPortSerdesTraits::Attributes::AttributeTxFfeCoeff4Wrapper::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiPortSerdesTraits::Attributes::AttributeTxDriverSwingWrapper::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiPortSerdesTraits::Attributes::AttributeTxLdoBypassWrapper::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t> SaiPortSerdesTraits::Attributes::
    AttributeRxInstgBoost1StartWrapper::operator()() {
  return std::nullopt;
}
std::optional<sai_attr_id_t> SaiPortSerdesTraits::Attributes::
    AttributeRxInstgBoost1StepWrapper::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t> SaiPortSerdesTraits::Attributes::
    AttributeRxInstgBoost1StopWrapper::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t> SaiPortSerdesTraits::Attributes::
    AttributeRxInstgBoost2OrHrStartWrapper::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t> SaiPortSerdesTraits::Attributes::
    AttributeRxInstgBoost2OrHrStepWrapper::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t> SaiPortSerdesTraits::Attributes::
    AttributeRxInstgBoost2OrHrStopWrapper::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t> SaiPortSerdesTraits::Attributes::
    AttributeRxInstgC1Start1p7Wrapper::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t> SaiPortSerdesTraits::Attributes::
    AttributeRxInstgC1Step1p7Wrapper::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t> SaiPortSerdesTraits::Attributes::
    AttributeRxInstgC1Stop1p7Wrapper::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t> SaiPortSerdesTraits::Attributes::
    AttributeRxInstgDfeStart1p7Wrapper::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t> SaiPortSerdesTraits::Attributes::
    AttributeRxInstgDfeStep1p7Wrapper::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t> SaiPortSerdesTraits::Attributes::
    AttributeRxInstgDfeStop1p7Wrapper::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t> SaiPortSerdesTraits::Attributes::
    AttributeRxEnableScanSelectionWrapper ::operator()() {
  return std::nullopt;
}
std::optional<sai_attr_id_t> SaiPortSerdesTraits::Attributes::
    AttributeRxInstgScanUseSrSettingsWrapper ::operator()() {
  return std::nullopt;
}
std::optional<sai_attr_id_t>
SaiPortSerdesTraits::Attributes::AttributeRxCdrCfgOvEnWrapper ::operator()() {
  return std::nullopt;
}
std::optional<sai_attr_id_t> SaiPortSerdesTraits::Attributes::
    AttributeRxCdrTdet1stOrdStepOvValWrapper ::operator()() {
  return std::nullopt;
}
std::optional<sai_attr_id_t> SaiPortSerdesTraits::Attributes::
    AttributeRxCdrTdet2ndOrdStepOvValWrapper ::operator()() {
  return std::nullopt;
}
std::optional<sai_attr_id_t> SaiPortSerdesTraits::Attributes::
    AttributeRxCdrTdetFineStepOvValWrapper ::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiPortSerdesTraits::Attributes::AttributeRxLdoBypassWrapper::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiPortSerdesTraits::Attributes::AttributeRxDiffEncoderEnWrapper::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t> SaiPortSerdesTraits::Attributes::
    AttributeRxInstgEnableScanWrapper::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t> SaiPortSerdesTraits::Attributes::
    AttributeRxFfeLengthBitmapWrapper::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t> SaiPortSerdesTraits::Attributes::
    AttributeRxFfeLmsDynamicGatingEnWrapper::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t>
SaiPortTraits::Attributes::AttributeCondEntropyRehashEnable::operator()() {
#if defined(BRCM_SAI_SDK_DNX_GTE_11_0)
  return SAI_PORT_ATTR_COND_ENTROPY_REHASH_ENABLE;
#else
  return std::nullopt;
#endif
}

std::optional<sai_attr_id_t>
SaiPortTraits::Attributes::AttributeCondEntropyRehashPeriodUS::operator()() {
// TODO(zecheng): Update flag when new 12.0 release has the attribute
#if defined(BRCM_SAI_SDK_DNX_GTE_11_0) && !defined(BRCM_SAI_SDK_DNX_GTE_12_0)
  // TODO: Fix this properly as SAI_PORT_ATTR_COND_ENTROPY_REHASH_PERIOD_US
  // port extension attribute is removed and no longer supported.
  // This is just a workaround to integrate BRCM SAI 11.7 GA
  return std::nullopt;
#else
  return std::nullopt;
#endif
}

std::optional<sai_attr_id_t>
SaiPortTraits::Attributes::AttributeCondEntropyRehashSeed::operator()() {
// TODO(zecheng): Update flag when new 12.0 release has the attribute
#if defined(BRCM_SAI_SDK_DNX_GTE_11_0) && !defined(BRCM_SAI_SDK_DNX_GTE_12_0)
  return SAI_PORT_ATTR_COND_ENTROPY_REHASH_SEED;
#else
  return std::nullopt;
#endif
}

std::optional<sai_attr_id_t>
SaiPortTraits::Attributes::AttributeShelEnable::operator()() {
#if defined(BRCM_SAI_SDK_DNX_GTE_11_0)
  return SAI_PORT_ATTR_SHEL_ENABLE;
#else
  return std::nullopt;
#endif
}

std::optional<sai_attr_id_t>
SaiPortTraits::Attributes::AttributeArsLinkState::operator()() {
#if defined(BRCM_SAI_SDK_GTE_13_0) && !defined(BRCM_SAI_SDK_GTE_14_0) && \
    defined(BRCM_SAI_SDK_XGS)
  return SAI_PORT_ATTR_ARS_LINK_STATE;
#else
  return std::nullopt;
#endif
}

std::optional<sai_attr_id_t>
SaiPortTraits::Attributes::AttributeIsHyperPortMember::operator()() {
#if defined(BRCM_SAI_SDK_DNX_GTE_14_0)
  return SAI_PORT_ATTR_IS_HYPERPORT_MEMBER;
#else
  return std::nullopt;
#endif
}

std::optional<sai_attr_id_t>
SaiPortTraits::Attributes::AttributeHyperPortMemberList::operator()() {
#if defined(BRCM_SAI_SDK_DNX_GTE_14_0)
  return SAI_PORT_ATTR_HYPERPORT_MEMBER_LIST;
#else
  return std::nullopt;
#endif
}

const std::vector<sai_stat_id_t>&
SaiPortTraits::macTxDataQueueMinWatermarkStats() {
#if defined(BRCM_SAI_SDK_DNX_GTE_11_7) && !defined(BRCM_SAI_SDK_DNX_GTE_13_0)
  static const std::vector<sai_stat_id_t> stats{
      SAI_PORT_STAT_MAC_TX_DATA_QUEUE_MIN_WM};
#else
  static const std::vector<sai_stat_id_t> stats;
#endif
  return stats;
}

const std::vector<sai_stat_id_t>&
SaiPortTraits::macTxDataQueueMaxWatermarkStats() {
#if defined(BRCM_SAI_SDK_DNX_GTE_11_7) && !defined(BRCM_SAI_SDK_DNX_GTE_13_0)
  static const std::vector<sai_stat_id_t> stats{
      SAI_PORT_STAT_MAC_TX_DATA_QUEUE_MAX_WM};
#else
  static const std::vector<sai_stat_id_t> stats;
#endif
  return stats;
}

const std::vector<sai_stat_id_t>& SaiPortTraits::fabricControlRxPacketStats() {
#if defined(BRCM_SAI_SDK_DNX_GTE_13_0)
  static const std::vector<sai_stat_id_t> stats{
      SAI_PORT_STAT_FABRIC_CONTROL_RX_PKTS};
#else
  static const std::vector<sai_stat_id_t> stats;
#endif
  return stats;
}

const std::vector<sai_stat_id_t>& SaiPortTraits::fabricControlTxPacketStats() {
#if defined(BRCM_SAI_SDK_DNX_GTE_13_0)
  static const std::vector<sai_stat_id_t> stats{
      SAI_PORT_STAT_FABRIC_CONTROL_TX_PKTS};
#else
  static const std::vector<sai_stat_id_t> stats;
#endif
  return stats;
}

const std::vector<sai_stat_id_t>& SaiPortTraits::pfcXoffTotalDurationStats() {
#if defined(BRCM_SAI_SDK_GTE_13_0) && defined(BRCM_SAI_SDK_XGS)
  static const std::vector<sai_stat_id_t> stats{
      SAI_PORT_STAT_PFC_0_XOFF_TOTAL_DURATION,
      SAI_PORT_STAT_PFC_1_XOFF_TOTAL_DURATION,
      SAI_PORT_STAT_PFC_2_XOFF_TOTAL_DURATION,
      SAI_PORT_STAT_PFC_3_XOFF_TOTAL_DURATION,
      SAI_PORT_STAT_PFC_4_XOFF_TOTAL_DURATION,
      SAI_PORT_STAT_PFC_5_XOFF_TOTAL_DURATION,
      SAI_PORT_STAT_PFC_6_XOFF_TOTAL_DURATION,
      SAI_PORT_STAT_PFC_7_XOFF_TOTAL_DURATION};
#else
  static const std::vector<sai_stat_id_t> stats;
#endif
  return stats;
}
} // namespace facebook::fboss
