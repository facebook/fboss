// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/lib/fpga/HwMemoryRegion.h"

namespace facebook::fboss {

/*
  Maps a single regist of memory. Helper class for things like led or qsfp
  status registers
*/
template <typename IO>
class HwMemoryRegister : public HwMemoryRegion<IO> {
 public:
  HwMemoryRegister(const std::string& name, FpgaDevice* device, uint32_t start)
      : HwMemoryRegion<IO>(name, device, start, 4) {}

  uint32_t readRegister() const {
    return read(0);
  }

  void writeRegister(uint32_t value) {
    write(0, value);
  }

 private:
  using FpgaMemoryRegion::read;
  using FpgaMemoryRegion::write;
};

using FpgaMemoryRegister = HwMemoryRegister<FpgaDevice>;

} // namespace facebook::fboss
