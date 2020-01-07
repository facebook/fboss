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

#include "fboss/lib/usb/PCA9548MuxedBus.h"

namespace facebook::fboss {
// Handle the intricacies of routing I2C requests to appropriate QSFPs,
// based on the hardware.

class Wedge100I2CBus : public PCA9548MuxedBus<32> {
 public:
  void scanPresence(std::map<int32_t, ModulePresence>& presences) override;

  /* Trigger the QSFP hard reset for a given QSFP module in the wedge100.
   * This function access the CPLD do trigger the hard reset of QSFP module.
   */
  void triggerQsfpHardReset(unsigned int module) override;

 protected:
  void initBus() override;

 private:
  MuxLayer createMuxes() override;
  void wireUpPorts(PortLeaves& leaves) override;

  std::unique_ptr<PCA9548> qsfpStatusMux_;
};

} // namespace facebook::fboss
