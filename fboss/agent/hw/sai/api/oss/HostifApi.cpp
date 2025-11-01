// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/api/HostifApi.h"

namespace facebook::fboss {

std::optional<sai_attr_id_t>
SaiTxPacketTraits::Attributes::AttributePacketType::operator()() {
  return std::nullopt;
}

} // namespace facebook::fboss
