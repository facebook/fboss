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

#include <string>
#include <vector>
#include "fboss/mdio/Mdio.h"

namespace facebook::fboss {

class BspDeviceMdio : public Mdio {
 public:
  static constexpr int kDelayAfterMdioInit = 100000;
  static constexpr int kDelayAfterMdioTransaction = 50000;

  // There will be one BspDeviceMdio object for each controller
  BspDeviceMdio(int pim, int controller, const std::string& devName);

  virtual ~BspDeviceMdio() override;

  // Initialize the MDIO controller
  // 'forceReset' will force the MDIO to be reset first, and then out of reset.
  void init(bool forceReset) override;

  // CL45 MDIO read, blocking call
  phy::Cl45Data readCl45(
      phy::PhyAddress physAddr,
      phy::Cl45DeviceAddress devAddr,
      phy::Cl45RegisterAddress regAddr) override;

  // CL45 MDIO write, blocking call
  void writeCl45(
      phy::PhyAddress physAddr,
      phy::Cl45DeviceAddress devAddr,
      phy::Cl45RegisterAddress regAddr,
      phy::Cl45Data data) override;

 protected:
  int getFileHandle() {
    return fd_;
  }

 private:
  std::string devName_;
  int fd_;
  int pim_;
  int controller_;
  uint8_t request_{0}; // the request number

  phy::Cl45Data cl45Operation(
      unsigned int ioctlVal,
      phy::PhyAddress physAddr,
      phy::Cl45DeviceAddress devAddr,
      phy::Cl45RegisterAddress regAddr,
      phy::Cl45Data data);
};

using BspDeviceMdioController = MdioController<BspDeviceMdio>;
} // namespace facebook::fboss
