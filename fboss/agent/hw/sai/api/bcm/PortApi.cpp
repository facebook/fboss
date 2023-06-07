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
#if defined(SAI_VERSION_8_2_0_0_ODP) ||                                        \
    defined(SAI_VERSION_8_2_0_0_SIM_ODP) || defined(SAI_VERSION_9_0_EA_ODP) || \
    defined(SAI_VERSION_9_0_EA_SIM_ODP)
  return SAI_PORT_ATTR_SERDES_LANE_LIST;
#else
  return std::nullopt;
#endif
}
std::optional<sai_attr_id_t>
SaiPortTraits::Attributes::AttributeDiagModeEnable::operator()() {
#if defined(SAI_VERSION_8_2_0_0_ODP) ||                                        \
    defined(SAI_VERSION_8_2_0_0_SIM_ODP) || defined(SAI_VERSION_9_0_EA_ODP) || \
    defined(SAI_VERSION_9_0_EA_SIM_ODP)
  return SAI_PORT_ATTR_DIAGNOSTICS_MODE_ENABLE;
#else
  return std::nullopt;
#endif
}

std::optional<sai_attr_id_t>
SaiPortTraits::Attributes::AttributeRxLaneSquelchEnable::operator()() {
#if defined(SAI_VERSION_9_0_EA_ODP) || defined(SAI_VERSION_9_0_EA_SIM_ODP) || \
    defined(SAI_VERSION_10_0_EA_DNX_ODP)
  return SAI_PORT_ATTR_RX_LANE_SQUELCH_ENABLE;
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

} // namespace facebook::fboss
