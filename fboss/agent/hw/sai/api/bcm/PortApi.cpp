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

#if defined(SAI_VERSION_8_2_0_0_ODP) || defined(SAI_VERSION_9_0_EA_ODP)
std::optional<sai_attr_id_t>
SaiPortTraits::Attributes::AttributeSerdesLaneList::operator()() {
  return SAI_PORT_ATTR_SERDES_LANE_LIST;
}
std::optional<sai_attr_id_t>
SaiPortTraits::Attributes::AttributeDiagModeEnable::operator()() {
  return SAI_PORT_ATTR_DIAGNOSTICS_MODE_ENABLE;
}
#endif

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
