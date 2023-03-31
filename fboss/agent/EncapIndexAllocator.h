// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <memory>

namespace facebook::fboss {
class HwAsic;
class SwitchState;
class StateDelta;

class EncapIndexAllocator {
 public:
  static constexpr int kEncapIdxReservedForLoopbacks = 100;
  static int64_t getNextAvailableEncapIdx(
      const std::shared_ptr<SwitchState>& state,
      const HwAsic& asic);
  static std::shared_ptr<SwitchState> updateEncapIndices(
      const StateDelta& delta,
      const HwAsic& asic);
};
} // namespace facebook::fboss
