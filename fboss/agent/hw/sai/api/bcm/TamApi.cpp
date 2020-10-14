// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/api/TamApi.h"

extern "C" {
#include <experimental/saitamextensions.h>
}

namespace facebook::fboss {

std::optional<sai_attr_id_t>
SaiTamEventTraits::Attributes::AttributeSwitchEventType::operator()() {
  return SAI_TAM_EVENT_ATTR_SWITCH_EVENT_TYPE;
}

std::optional<sai_attr_id_t> SaiTamEventTraits::Attributes::AttributeEventId::
operator()() {
  return SAI_TAM_EVENT_ATTR_EVENT_ID;
}

} // namespace facebook::fboss
