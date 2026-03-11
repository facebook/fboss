// Copyright 2023-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/switch_asics/Chenab2Asic.h"

namespace facebook::fboss {

cfg::AsicType Chenab2Asic::getAsicType() const {
  return cfg::AsicType::ASIC_TYPE_CHENAB2;
}

uint64_t Chenab2Asic::getMMUSizeBytes() const {
  // Egress buffer pool size for Chenab2 Asics is 248MB
  return 1024 * 1024 * 248;
}

int Chenab2Asic::getMaxNumLogicalPorts() const {
  // 516 physical lanes + cpu
  return 517;
}

uint32_t Chenab2Asic::getPacketBufferUnitSize() const {
  return 256;
}

uint32_t Chenab2Asic::getMaxEcmpSize() const {
  return 512;
}

std::optional<uint32_t> Chenab2Asic::getMaxEcmpMembers() const {
  return 32000;
}

std::optional<uint32_t> Chenab2Asic::getMaxArsGroups() const {
  return 1024;
}

} // namespace facebook::fboss
