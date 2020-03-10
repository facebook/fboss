/*
 *  Copyright (c) 2018-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include "fboss/lib/PhysicalMemory.h"
#include "fboss/lib/fpga/facebook/FbDomFpga.h"

namespace facebook::fboss {
/**
 * FPGA device in Minipack platform.
 * NOTE: In FPGA, PIM number always starts from 1
 */
class MinipackFpga {
 public:
  static constexpr auto kNumberPim = 8;
  static constexpr auto kNumPortsPerPim = 16;

  MinipackFpga();
  static std::shared_ptr<MinipackFpga> getInstance();

  /**
   * This function should be called before any read/write() to call any hardware
   * related functions to make FPGA ready.
   * Right now, it doesn't require any input parameter, but if in the future
   * we need to support different HW settings, like 4DD, we can leaverage more
   * input parameters to set up FPGA.
   */
  void initHW();

  /**
   * FPGA PCIe Register has been upgraded to 32bits data width on 32 bits
   * address.
   */
  uint32_t readSmb(uint32_t offset);
  uint32_t readPim(uint8_t pim, uint32_t offset);

  void writeSmb(uint32_t offset, uint32_t value);
  void writePim(uint8_t pim, uint32_t offset, uint32_t value);

  bool isQsfpPresent(uint8_t pim, int qsfp);
  std::array<bool, kNumPortsPerPim> scanQsfpPresence(uint8_t pim);
  void ensureQsfpOutOfReset(uint8_t pim, int qsfp);

  /* Trigger the QSFP hard reset of a given port on a given line card (PIM)
   * by toggling the FPGA bit. This function is for minipack line cards:
   * 16q and 4dd
   */
  void triggerQsfpHardReset(uint8_t pim, int qsfp);

  void
  setFrontPanelLedColor(uint8_t pim, int qsfp, FbDomFpga::LedColor ledColor);

  FbDomFpga* getDomFpga(uint8_t pim);

  FbDomFpga::PimType getPimType(uint8_t pim);

 private:
  static constexpr uint32_t kFacebookFpgaVendorID = 0x1d9b;

  using MinipackPhysicalMemory32 = PhysicalMemory32<PhysicalMemory>;

  std::unique_ptr<MinipackPhysicalMemory32> phyMem32_;

  std::array<std::unique_ptr<FbDomFpga>, kNumberPim> pimFpgas_;

  bool isHwInitialized_{false};
};

} // namespace facebook::fboss
