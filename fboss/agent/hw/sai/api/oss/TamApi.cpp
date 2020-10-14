// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/api/TamApi.h"

namespace facebook::fboss {

std::optional<sai_attr_id_t>
SaiTamEventTraits::Attributes::AttributeSwitchEventType::operator()() {
  return std::nullopt;
}

std::optional<sai_attr_id_t> SaiTamEventTraits::Attributes::AttributeEventId::
operator()() {
  return std::nullopt;
}
} // namespace facebook::fboss
