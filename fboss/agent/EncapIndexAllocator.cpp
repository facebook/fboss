// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/EncapIndexAllocator.h"

#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"

namespace facebook::fboss {
class HwAsic;
class SwitchState;

int64_t EncapIndexAllocator::getNextAvailableEncapIdx(
    const std::shared_ptr<SwitchState>& /*state*/,
    const HwAsic& asic) {
  if (!asic.isSupported(HwAsic::Feature::RESERVED_ENCAP_INDEX_RANGE)) {
    throw FbossError(
        "Encap index allocation not supported on: ", asic.getAsicTypeStr());
  }
  auto start = *asic.getReservedEncapIndexRange().minimum() +
      kEncapIdxReservedForLoopbacks;
  // TODO - figure out a available encap index based on looking
  // at neighbor tables in switch state
  return start;
}
} // namespace facebook::fboss
