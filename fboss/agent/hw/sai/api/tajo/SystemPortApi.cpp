// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/api/SystemPortApi.h"

namespace facebook::fboss {

std::optional<sai_attr_id_t>
SaiSystemPortTraits::Attributes::AttributeShelPktDstEnable::operator()() {
  return std::nullopt;
}

} // namespace facebook::fboss
