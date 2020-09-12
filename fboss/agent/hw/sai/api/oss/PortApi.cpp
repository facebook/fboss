// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/api/PortApi.h"

namespace facebook::fboss {

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
std::optional<sai_attr_id_t>
SaiPortSerdesTraits::Attributes::AttributeRxAcCouplingBypassIdWrapper::
operator()() {
  return std::nullopt;
}
} // namespace facebook::fboss
