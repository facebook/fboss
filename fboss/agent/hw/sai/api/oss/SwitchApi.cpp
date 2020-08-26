// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/api/SwitchApi.h"

namespace facebook::fboss {

std::optional<sai_attr_id_t> SaiSwitchTraits::Attributes::AttributeLed::
operator()() {
  return std::nullopt;
}
std::optional<sai_attr_id_t> SaiSwitchTraits::Attributes::AttributeLedReset::
operator()() {
  return std::nullopt;
}

} // namespace facebook::fboss
