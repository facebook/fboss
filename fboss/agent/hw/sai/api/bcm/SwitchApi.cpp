// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/api/SwitchApi.h"

extern "C" {
#include <sai.h>

#include <experimental/saiswitchextensions.h>
}

namespace facebook::fboss {
std::optional<sai_attr_id_t>
SaiSwitchTraits::Attributes::AttributeLedIdWrapper::operator()() {
  return SAI_SWITCH_ATTR_LED;
}
std::optional<sai_attr_id_t>
SaiSwitchTraits::Attributes::AttributeLedResetIdWrapper::operator()() {
  return SAI_SWITCH_ATTR_LED_PROCESSOR_RESET;
}

} // namespace facebook::fboss
