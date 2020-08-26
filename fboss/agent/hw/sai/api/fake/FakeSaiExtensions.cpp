// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/api/PortApi.h"
#include "fboss/agent/hw/sai/api/SwitchApi.h"

extern "C" {
#include "fboss/agent/hw/sai/api/fake/saifakeextensions.h"
}

namespace facebook::fboss {
std::optional<sai_attr_id_t>
SaiPortSerdesTraits::Attributes::AttributeRxCtleCode::operator()() {
  return SAI_PORT_SERDES_ATTR_EXT_FAKE_RX_CTLE_CODE;
}
std::optional<sai_attr_id_t>
SaiPortSerdesTraits::Attributes::AttributeRxDspMode::operator()() {
  return SAI_PORT_SERDES_ATTR_EXT_FAKE_RX_DSCP_MODE;
}
std::optional<sai_attr_id_t>
SaiPortSerdesTraits::Attributes::AttributeRxAfeTrim::operator()() {
  return SAI_PORT_SERDES_ATTR_EXT_FAKE_RX_AFE_TRIM;
}
std::optional<sai_attr_id_t>
SaiPortSerdesTraits::Attributes::AttributeRxAcCouplingBypass::operator()() {
  return SAI_PORT_SERDES_ATTR_EXT_FAKE_RX_AC_COUPLING_BYPASS;
}

std::optional<sai_attr_id_t> SaiSwitchTraits::Attributes::AttributeLed::
operator()() {
  return SAI_SWITCH_ATTR_EXT_FAKE_LED;
}
std::optional<sai_attr_id_t> SaiSwitchTraits::Attributes::AttributeLedReset::
operator()() {
  return SAI_SWITCH_ATTR_EXT_FAKE_LED_RESET;
}

} // namespace facebook::fboss
