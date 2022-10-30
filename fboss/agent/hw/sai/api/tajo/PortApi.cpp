// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/api/PortApi.h"

extern "C" {
#include <sai.h>

#include <experimental/sai_attr_ext.h>
}

namespace facebook::fboss {

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

#if defined(TAJO_SDK_VERSION_1_56_1) || defined(TAJO_SDK_VERSION_1_58_1)
std::optional<sai_attr_id_t>
SaiPortSerdesTraits::Attributes::AttributeRxChannelReachWrapper::operator()() {
  return SAI_PORT_SERDES_ATTR_EXT_RX_CHANNEL_REACH;
}
std::optional<sai_attr_id_t>
SaiPortSerdesTraits::Attributes::AttributeRxDiffEncoderEnWrapper::operator()() {
  return SAI_PORT_SERDES_ATTR_EXT_RX_DIFF_ENCODER_EN;
}

std::optional<sai_attr_id_t> SaiPortSerdesTraits::Attributes::
    AttributeRxFbfCoefInitValWrapper::operator()() {
  return SAI_PORT_SERDES_ATTR_EXT_RX_FBF_COEF_INIT_VAL;
}

std::optional<sai_attr_id_t>
SaiPortSerdesTraits::Attributes::AttributeRxFbfLmsEnableWrapper::operator()() {
  return SAI_PORT_SERDES_ATTR_EXT_RX_FBF_LMS_ENABLE;
}

std::optional<sai_attr_id_t> SaiPortSerdesTraits::Attributes::
    AttributeRxInstgScanOptimizeWrapper::operator()() {
  return SAI_PORT_SERDES_ATTR_EXT_RX_INSTG_SCAN_OPTIMIZE;
}

std::optional<sai_attr_id_t> SaiPortSerdesTraits::Attributes::
    AttributeRxInstgTableEndRowWrapper::operator()() {
  return SAI_PORT_SERDES_ATTR_EXT_RX_INSTG_TABLE_END_ROW;
}

std::optional<sai_attr_id_t> SaiPortSerdesTraits::Attributes::
    AttributeRxInstgTableStartRowWrapper::operator()() {
  return SAI_PORT_SERDES_ATTR_EXT_RX_INSTG_TABLE_START_ROW;
}

std::optional<sai_attr_id_t> SaiPortSerdesTraits::Attributes::
    AttributeRxParityEncoderEnWrapper::operator()() {
  return SAI_PORT_SERDES_ATTR_EXT_RX_PARITY_ENCODER_EN;
}

std::optional<sai_attr_id_t>
SaiPortSerdesTraits::Attributes::AttributeRxThpEnWrapper::operator()() {
  return SAI_PORT_SERDES_ATTR_EXT_RX_THP_EN;
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

std::optional<sai_attr_id_t> SaiPortSerdesTraits::Attributes::
    AttributeTxParityEncoderEnWrapper::operator()() {
  return SAI_PORT_SERDES_ATTR_EXT_TX_PARITY_ENCODER_EN;
}

std::optional<sai_attr_id_t>
SaiPortSerdesTraits::Attributes::AttributeTxThpEnWrapper::operator()() {
  return SAI_PORT_SERDES_ATTR_EXT_TX_THP_EN;
}
#endif
} // namespace facebook::fboss
