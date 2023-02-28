// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/api/TamApi.h"

#if !defined(SAI_VERSION_7_2_0_0_ODP) && !defined(SAI_VERSION_8_2_0_0_ODP) && \
    !defined(SAI_VERSION_8_2_0_0_DNX_ODP) &&                                  \
    !defined(SAI_VERSION_8_2_0_0_SIM_ODP) &&                                  \
    !defined(SAI_VERSION_9_0_EA_ODP) &&                                       \
    !defined(SAI_VERSION_9_0_EA_DNX_ODP) &&                                   \
    !defined(SAI_VERSION_9_0_EA_SIM_ODP)

extern "C" {
#ifndef IS_OSS_BRCM_SAI
#include <experimental/saitamextensions.h>
#else
#include <saitamextensions.h>
#endif
}
#endif

namespace facebook::fboss {

std::optional<sai_attr_id_t>
SaiTamEventTraits::Attributes::AttributeSwitchEventType::operator()() {
#if !defined(SAI_VERSION_7_2_0_0_ODP) && !defined(SAI_VERSION_8_2_0_0_ODP) && \
    !defined(SAI_VERSION_8_2_0_0_DNX_ODP) &&                                  \
    !defined(SAI_VERSION_8_2_0_0_SIM_ODP) &&                                  \
    !defined(SAI_VERSION_9_0_EA_ODP) &&                                       \
    !defined(SAI_VERSION_9_0_EA_DNX_ODP) &&                                   \
    !defined(SAI_VERSION_9_0_EA_SIM_ODP)
  return SAI_TAM_EVENT_ATTR_SWITCH_EVENT_TYPE;
#else
  return std::nullopt;
#endif
}

std::optional<sai_attr_id_t>
SaiTamEventTraits::Attributes::AttributeEventId::operator()() {
#if !defined(SAI_VERSION_7_2_0_0_ODP) && !defined(SAI_VERSION_8_2_0_0_ODP) && \
    !defined(SAI_VERSION_8_2_0_0_DNX_ODP) &&                                  \
    !defined(SAI_VERSION_8_2_0_0_SIM_ODP) &&                                  \
    !defined(SAI_VERSION_9_0_EA_ODP) &&                                       \
    !defined(SAI_VERSION_9_0_EA_DNX_ODP) &&                                   \
    !defined(SAI_VERSION_9_0_EA_SIM_ODP)
  return SAI_TAM_EVENT_ATTR_EVENT_ID;
#else
  return std::nullopt;
#endif
}

} // namespace facebook::fboss
