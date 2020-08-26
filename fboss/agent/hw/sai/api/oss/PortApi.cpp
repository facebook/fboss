// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/api/PortApi.h"

namespace facebook::fboss {

std::optional<sai_attr_id_t> AttributeRxCtleCode::operator()() {
  return std::nullopt;
}
std::optional<sai_attr_id_t> AttributeRxDspMode::operator()() {
  return std::nullopt;
}
std::optional<sai_attr_id_t> AttributeRxAfeTrim::operator()() {
  return std::nullopt;
}
std::optional<sai_attr_id_t> AttributeRxAcCouplingBypass::operator()() {
  return std::nullopt;
}
} // namespace facebook::fboss
