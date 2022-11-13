// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <memory>

namespace facebook::fboss {
class HwAsic;
class SwitchState;

class EncapIndexAllocator {
 public:
  static constexpr int kEncapIdxReservedForLoopbacks = 100;
  static int64_t getNextAvailableEncapIdx(
      const std::shared_ptr<SwitchState>& state,
      const HwAsic& asic);
};
} // namespace facebook::fboss
