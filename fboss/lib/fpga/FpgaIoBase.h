// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/lib/PhysicalMemory.h"

namespace facebook::fboss {

// TODO(daiweix): refactor these codes with MultiPimFpga classes in D23219064
class FpgaIoBase {
 public:
  using PhyMem32 = PhysicalMemory32<PhysicalMemory>;
  FpgaIoBase(std::unique_ptr<PhyMem32> phyMem) : phyMem32_(move(phyMem)) {}
  FpgaIoBase(uint32_t baseAddr, uint32_t fpgaSize)
      : phyMem32_(std::make_unique<PhyMem32>(baseAddr, fpgaSize, false)) {}

  virtual ~FpgaIoBase() {}

  /**
   * This function should be called before any read/write() to call any hardware
   * related functions to make FPGA ready.
   * Right now, it doesn't require any input parameter, but if in the future
   * we need to support different HW settings, like 4DD, we can leaverage more
   * input parameters to set up FPGA.
   */
  virtual void initHW() {
    phyMem32_->mmap();
  }

  /**
   * FPGA PCIe Register has been upgraded to 32bits data width on 32 bits
   * address.
   */
  virtual uint32_t read(uint32_t offset) const = 0;
  virtual void write(uint32_t offset, uint32_t value) = 0;

 protected:
  std::unique_ptr<PhyMem32> phyMem32_;
};

} // namespace facebook::fboss
