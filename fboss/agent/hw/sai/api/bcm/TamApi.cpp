// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/api/TamApi.h"

#if !defined(SAI_VERSION_5_1_0_3_ODP)

extern "C" {
#include <experimental/saitamextensions.h>
}
#endif

namespace facebook::fboss {

std::optional<sai_attr_id_t>
SaiTamEventTraits::Attributes::AttributeSwitchEventType::operator()() {
#if !defined(SAI_VERSION_5_1_0_3_ODP)
  return SAI_TAM_EVENT_ATTR_SWITCH_EVENT_TYPE;
#else
  return std::nullopt;
#endif
}

std::optional<sai_attr_id_t>
SaiTamEventTraits::Attributes::AttributeEventId::operator()() {
#if !defined(SAI_VERSION_5_1_0_3_ODP)
  return SAI_TAM_EVENT_ATTR_EVENT_ID;
#else
  return std::nullopt;
#endif
}

} // namespace facebook::fboss
