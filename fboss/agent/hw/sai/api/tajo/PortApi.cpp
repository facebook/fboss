// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/api/PortApi.h"

extern "C" {
#include <sai.h>

#include <experimental/sai_attr_ext.h>
}

namespace facebook::fboss {

std::optional<sai_attr_id_t> AttributeRxCtleCode::operator()() {
  return SAI_PORT_SERDES_ATTR_EXT_RX_CTLE_CODE;
}
std::optional<sai_attr_id_t> AttributeRxDspMode::operator()() {
  return SAI_PORT_SERDES_ATTR_EXT_RX_DSP_MODE;
}
std::optional<sai_attr_id_t> AttributeRxAfeTrim::operator()() {
  return SAI_PORT_SERDES_ATTR_EXT_RX_AFE_TRIM;
}
std::optional<sai_attr_id_t> AttributeRxAcCouplingBypass::operator()() {
  return SAI_PORT_SERDES_ATTR_EXT_RX_AC_COUPLING_BYPASS;
}
} // namespace facebook::fboss
