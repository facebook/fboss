/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once

#include "fboss/lib/fpga/FbFpgaRegisters.h"
#include "fboss/lib/fpga/HwMemoryRegion.h"
#include "fboss/mdio/Mdio.h"

namespace facebook::fboss {

enum class FbFpgaMdioVersion {
  UNKNOWN = -1,
  V0 = 0,
};

class FbFpgaMdio : public Mdio {
 public:
  // TODO: use a static factory method when we add more versions
  explicit FbFpgaMdio(
      FpgaMemoryRegion* io,
      uint32_t baseAddr,
      FbFpgaMdioVersion version = FbFpgaMdioVersion::V0);

  // read/write apis.
  phy::Cl45Data readCl45(
      phy::PhyAddress physAddr,
      phy::Cl45DeviceAddress devAddr,
      phy::Cl45RegisterAddress regAddr) override;

  void writeCl45(
      phy::PhyAddress physAddr,
      phy::Cl45DeviceAddress devAddr,
      phy::Cl45RegisterAddress regAddr,
      phy::Cl45Data data) override;

  void reset();

  void setClockDivisor(int div);

  void setFastMode(bool enable);

 private:
  void clearStatus();
  void waitUntilDone(uint32_t millis, MdioCommand command);
  void printSlpcError();

  template <typename Register>
  Register readReg();
  template <typename Register>
  void writeReg(Register value);

  FpgaMemoryRegion* io_{nullptr};
  const uint32_t baseAddr_;
  const FbFpgaMdioVersion version_;
};

} // namespace facebook::fboss
