// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/lib/fpga/HwMemoryRegion.h"

#include <folly/SharedMutex.h>

namespace facebook::fboss {

/*
  Maps a single regist of memory. Helper class for things like led or qsfp
  status registers
*/
template <typename IO>
class HwMemoryRegister : public HwMemoryRegion<IO> {
 public:
  HwMemoryRegister(const std::string& name, FpgaDevice* device, uint32_t start)
      : HwMemoryRegion<IO>(name, device, start, 4) {
    mutex_ = std::make_unique<folly::SharedMutex>();
  }

  uint32_t readRegister() const {
    std::shared_lock g(*mutex_);
    return read(0);
  }

  void writeRegister(uint32_t value) {
    std::unique_lock g(*mutex_);
    write(0, value);
  }

 private:
  std::unique_ptr<folly::SharedMutex> mutex_;
  using FpgaMemoryRegion::read;
  using FpgaMemoryRegion::write;
};

using FpgaMemoryRegister = HwMemoryRegister<FpgaDevice>;

} // namespace facebook::fboss
