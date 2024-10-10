// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/api/PortApi.h"

extern "C" {
#include <sai.h>

#include <experimental/sai_attr_ext.h>
}

#if defined(TAJO_SDK_GTE_24_4_90)
#define RETURN_SUPPORTED_ATTR(attr) return (attr);
#else
#define RETURN_SUPPORTED_ATTR(attr) return std::nullopt;
#endif

namespace facebook::fboss {
std::optional<sai_attr_id_t>
SaiPortSerdesTraits::Attributes::AttributeTxLutModeIdWrapper::operator()() {
  return SAI_PORT_SERDES_ATTR_EXT_TX_LUT_MODE;
}
std::optional<sai_attr_id_t>
SaiPortSerdesTraits::Attributes::AttributeRxCtleCodeIdWrapper::operator()() {
  return SAI_PORT_SERDES_ATTR_EXT_RX_CTLE_CODE;
}
std::optional<sai_attr_id_t>
SaiPortSerdesTraits::Attributes::AttributeRxDspModeIdWrapper::operator()() {
  return SAI_PORT_SERDES_ATTR_EXT_RX_DSP_MODE;
}
std::optional<sai_attr_id_t>
SaiPortSerdesTraits::Attributes::AttributeRxAfeTrimIdWrapper::operator()() {
  return SAI_PORT_SERDES_ATTR_EXT_RX_AFE_TRIM;
}
std::optional<sai_attr_id_t> SaiPortSerdesTraits::Attributes::
    AttributeRxAcCouplingBypassIdWrapper::operator()() {
  return SAI_PORT_SERDES_ATTR_EXT_RX_AC_COUPLING_BYPASS;
}
std::optional<sai_attr_id_t>
SaiPortTraits::Attributes::AttributeSystemPortId::operator()() {
  return SAI_PORT_ATTR_EXT_SYSTEM_PORT_ID;
}
std::optional<sai_attr_id_t> SaiPortSerdesTraits::Attributes::
    AttributeRxAfeAdaptiveEnableWrapper::operator()() {
  return SAI_PORT_SERDES_ATTR_EXT_RX_AFE_ADAPTIVE_ENABLE;
}
std::optional<sai_attr_id_t>
SaiPortTraits::Attributes::AttributeSerdesLaneList::operator()() {
  return std::nullopt;
}
std::optional<sai_attr_id_t>
SaiPortTraits::Attributes::AttributeDiagModeEnable::operator()() {
  return std::nullopt;
}
std::optional<sai_attr_id_t>
SaiPortTraits::Attributes::AttributeRxLaneSquelchEnable::operator()() {
  return std::nullopt;
}
std::optional<sai_attr_id_t>
SaiPortTraits::Attributes::AttributeFdrEnable::operator()() {
  return std::nullopt;
}
#if SAI_API_VERSION >= SAI_VERSION(1, 10, 3)
std::optional<sai_attr_id_t>
SaiPortTraits::Attributes::AttributeCrcErrorDetect::operator()() {
  return std::nullopt;
}
#endif
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
SaiPortSerdesTraits::Attributes::AttributeTxDiffEncoderEnWrapper::operator()() {
  RETURN_SUPPORTED_ATTR(SAI_PORT_SERDES_ATTR_EXT_TX_DIFF_ENCODER_EN);
}

std::optional<sai_attr_id_t>
SaiPortSerdesTraits::Attributes::AttributeTxDigGainWrapper::operator()() {
  RETURN_SUPPORTED_ATTR(SAI_PORT_SERDES_ATTR_EXT_TX_DIG_GAIN);
}

std::optional<sai_attr_id_t>
SaiPortSerdesTraits::Attributes::AttributeTxFfeCoeff0Wrapper::operator()() {
  RETURN_SUPPORTED_ATTR(SAI_PORT_SERDES_ATTR_EXT_TX_FFE_COEFF_0);
}

std::optional<sai_attr_id_t>
SaiPortSerdesTraits::Attributes::AttributeTxFfeCoeff1Wrapper::operator()() {
  RETURN_SUPPORTED_ATTR(SAI_PORT_SERDES_ATTR_EXT_TX_FFE_COEFF_1);
}

std::optional<sai_attr_id_t>
SaiPortSerdesTraits::Attributes::AttributeTxFfeCoeff2Wrapper::operator()() {
  RETURN_SUPPORTED_ATTR(SAI_PORT_SERDES_ATTR_EXT_TX_FFE_COEFF_2);
}

std::optional<sai_attr_id_t>
SaiPortSerdesTraits::Attributes::AttributeTxFfeCoeff3Wrapper::operator()() {
  RETURN_SUPPORTED_ATTR(SAI_PORT_SERDES_ATTR_EXT_TX_FFE_COEFF_3);
}

std::optional<sai_attr_id_t>
SaiPortSerdesTraits::Attributes::AttributeTxFfeCoeff4Wrapper::operator()() {
  RETURN_SUPPORTED_ATTR(SAI_PORT_SERDES_ATTR_EXT_TX_FFE_COEFF_4);
}

std::optional<sai_attr_id_t>
SaiPortSerdesTraits::Attributes::AttributeTxDriverSwingWrapper::operator()() {
  RETURN_SUPPORTED_ATTR(SAI_PORT_SERDES_ATTR_EXT_TX_DRIVER_SWING);
}

std::optional<sai_attr_id_t> SaiPortSerdesTraits::Attributes::
    AttributeRxInstgBoost1StartWrapper::operator()() {
  RETURN_SUPPORTED_ATTR(SAI_PORT_SERDES_ATTR_EXT_RX_INSTG_BOOST1_STRT);
}

std::optional<sai_attr_id_t> SaiPortSerdesTraits::Attributes::
    AttributeRxInstgBoost1StepWrapper::operator()() {
  RETURN_SUPPORTED_ATTR(SAI_PORT_SERDES_ATTR_EXT_RX_INSTG_BOOST1_STEP);
}

std::optional<sai_attr_id_t> SaiPortSerdesTraits::Attributes::
    AttributeRxInstgBoost1StopWrapper::operator()() {
  RETURN_SUPPORTED_ATTR(SAI_PORT_SERDES_ATTR_EXT_RX_INSTG_BOOST1_STOP);
}

std::optional<sai_attr_id_t> SaiPortSerdesTraits::Attributes::
    AttributeRxInstgBoost2OrHrStartWrapper::operator()() {
  RETURN_SUPPORTED_ATTR(SAI_PORT_SERDES_ATTR_EXT_RX_INSTG_BOOST2_OR_HR_STRT);
}

std::optional<sai_attr_id_t> SaiPortSerdesTraits::Attributes::
    AttributeRxInstgBoost2OrHrStepWrapper::operator()() {
  RETURN_SUPPORTED_ATTR(SAI_PORT_SERDES_ATTR_EXT_RX_INSTG_BOOST2_OR_HR_STEP);
}

std::optional<sai_attr_id_t> SaiPortSerdesTraits::Attributes::
    AttributeRxInstgBoost2OrHrStopWrapper::operator()() {
  RETURN_SUPPORTED_ATTR(SAI_PORT_SERDES_ATTR_EXT_RX_INSTG_BOOST2_OR_HR_STOP);
}

std::optional<sai_attr_id_t> SaiPortSerdesTraits::Attributes::
    AttributeRxInstgC1Start1p7Wrapper::operator()() {
  RETURN_SUPPORTED_ATTR(SAI_PORT_SERDES_ATTR_EXT_RX_INSTG_C1_START_1P7);
}

std::optional<sai_attr_id_t> SaiPortSerdesTraits::Attributes::
    AttributeRxInstgC1Step1p7Wrapper::operator()() {
  RETURN_SUPPORTED_ATTR(SAI_PORT_SERDES_ATTR_EXT_RX_INSTG_C1_STEP_1P7);
}

std::optional<sai_attr_id_t> SaiPortSerdesTraits::Attributes::
    AttributeRxInstgC1Stop1p7Wrapper::operator()() {
  RETURN_SUPPORTED_ATTR(SAI_PORT_SERDES_ATTR_EXT_RX_INSTG_C1_STOP_1P7);
}

std::optional<sai_attr_id_t> SaiPortSerdesTraits::Attributes::
    AttributeRxInstgDfeStart1p7Wrapper::operator()() {
  RETURN_SUPPORTED_ATTR(SAI_PORT_SERDES_ATTR_EXT_RX_INSTG_DFE_START_1P7);
}

std::optional<sai_attr_id_t> SaiPortSerdesTraits::Attributes::
    AttributeRxInstgDfeStep1p7Wrapper::operator()() {
  RETURN_SUPPORTED_ATTR(SAI_PORT_SERDES_ATTR_EXT_RX_INSTG_DFE_STEP_1P7);
}

std::optional<sai_attr_id_t> SaiPortSerdesTraits::Attributes::
    AttributeRxInstgDfeStop1p7Wrapper::operator()() {
  RETURN_SUPPORTED_ATTR(SAI_PORT_SERDES_ATTR_EXT_RX_INSTG_DFE_STOP_1P7);
}

std::optional<sai_attr_id_t> SaiPortSerdesTraits::Attributes::
    AttributeRxEnableScanSelectionWrapper ::operator()() {
  RETURN_SUPPORTED_ATTR(SAI_PORT_SERDES_ATTR_EXT_RX_INSTG_ENABLE_SCAN);
}
std::optional<sai_attr_id_t> SaiPortSerdesTraits::Attributes::
    AttributeRxInstgScanUseSrSettingsWrapper ::operator()() {
  RETURN_SUPPORTED_ATTR(SAI_PORT_SERDES_ATTR_EXT_RX_INSTG_SCAN_USE_SR_SETTINGS);
}
std::optional<sai_attr_id_t>
SaiPortSerdesTraits::Attributes::AttributeRxCdrCfgOvEnWrapper ::operator()() {
  RETURN_SUPPORTED_ATTR(SAI_PORT_SERDES_ATTR_EXT_RX_CDR_CFG_OV_EN);
}
std::optional<sai_attr_id_t> SaiPortSerdesTraits::Attributes::
    AttributeRxCdrTdet1stOrdStepOvValWrapper ::operator()() {
  RETURN_SUPPORTED_ATTR(
      SAI_PORT_SERDES_ATTR_EXT_RX_CDR_TDET_1ST_ORD_STEP_OV_VAL);
}
std::optional<sai_attr_id_t> SaiPortSerdesTraits::Attributes::
    AttributeRxCdrTdet2ndOrdStepOvValWrapper ::operator()() {
  RETURN_SUPPORTED_ATTR(
      SAI_PORT_SERDES_ATTR_EXT_RX_CDR_TDET_2ND_ORD_STEP_OV_VAL);
}
std::optional<sai_attr_id_t> SaiPortSerdesTraits::Attributes::
    AttributeRxCdrTdetFineStepOvValWrapper ::operator()() {
  RETURN_SUPPORTED_ATTR(SAI_PORT_SERDES_ATTR_EXT_RX_CDR_TDET_FINE_STEP_OV_VAL);
}

} // namespace facebook::fboss
