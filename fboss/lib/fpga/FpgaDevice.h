// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/types.h"
#include "fboss/lib/PciDevice.h"
#include "fboss/lib/PhysicalMemory.h"

#include <folly/Format.h>
#include <folly/logging/xlog.h>

namespace facebook::fboss {

/**
 * CPU connects to first set of FPGAs through the PCI bus. In some multiple card
 * platform like Minipack, there are another set of FPGAs connecting to the
 * first set through internal bus and communication protocol.
 *
 * No matter how the FPGA is connected to the CPU, SW uses the same way to
 * access the FPGA function. That is to access FPGA memory through a virtual
 * memory address.
 *
 * How to map FPGA physical memory into virtual memory is different, depending
 * on how FPGA is connected to the CPU. For the FPGA directly connected to the
 * CPU, we retrieve the physical memory address and its size through BAR
 * register of such device. mmap() that physical memory into the process
 * virtual memory space. For the FPGA indirectly connected to the CPU, their
 * memory space is represented as a region within the FPGA memory space
 * directly connected to the CPU.
 *
 * FpgaDevice models how we see a FPGA device on the PCI bus.
 */
class FpgaDevice {
 public:
  using PhyMem = PhysicalMemory32<PhysicalMemory>;

  FpgaDevice(PciVendorId vendorId, PciDeviceId deviceId);

  // For platforms that can't use PCI device yet (Wedge400). This should be
  // removed when we support a more sophisticated way of selecting device (for
  // example using pci topology)
  FpgaDevice(uint32_t fpgaBar0, uint32_t fpgaBar0Size)
      : phyMem_(std::make_unique<PhyMem>(fpgaBar0, fpgaBar0Size, false)) {
    XLOG(DBG1) << folly::format(
        "Creating FPGA Device at address={:#x} size={:d}",
        fpgaBar0,
        fpgaBar0Size);
  }

  virtual ~FpgaDevice() {}

  /**
   * This function should be called before any read/write() to call any hardware
   * related functions to make FPGA ready.
   * Right now, it doesn't require any input parameter, but if in the future
   * we need to support different HW settings, like 4DD, we can leaverage more
   * input parameters to set up FPGA.
   */
  void mmap() {
    phyMem_->mmap();
  }

  virtual uint32_t read(uint32_t offset) const {
    CHECK(offset >= 0 && offset < phyMem_->getSize() - 3);
    uint32_t ret = phyMem_->read(offset);
    XLOG(DBG5) << folly::format("FPGA read {:#x}={:#x}", offset, ret);
    return ret;
  }

  virtual void write(uint32_t offset, uint32_t value) {
    CHECK(offset >= 0 && offset < phyMem_->getSize() - 3);
    XLOG(DBG5) << folly::format("FPGA write {:#x} to {:#x}", value, offset);
    phyMem_->write(offset, value);
  }

  uint32_t getBaseAddr() const {
    return phyMem_->getPhyAddress();
  }

  uint32_t getSize() const {
    return phyMem_->getSize();
  }

 private:
  std::unique_ptr<PhyMem> phyMem_;
}; // namespace facebook::fboss

} // namespace facebook::fboss
