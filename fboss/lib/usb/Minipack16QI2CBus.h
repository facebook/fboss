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

#include "fboss/lib/fpga/FbFpgaI2c.h"
#include "fboss/lib/i2c/gen-cpp2/i2c_controller_stats_types.h"
#include "fboss/lib/i2c/minipack/MinipackBaseI2cBus.h"
#include "fboss/lib/usb/PCA9548MuxedBus.h"

namespace facebook::fboss {
class Minipack16QI2CBus : public MinipackBaseI2cBus {
 public:
  Minipack16QI2CBus();
  ~Minipack16QI2CBus() override;

  /* Consolidate the i2c transaction stats from all the pims using their
   * corresponding i2c controller. In case of Minipack16q there are 8 pims
   * and there are four FbFpgaI2cController corresponding to each pim. This
   * function consolidates the counters from all constollers and return the
   * vector of the i2c stats
   */
  std::vector<I2cControllerStats> getI2cControllerStats() override;

 private:
  std::array<std::array<std::unique_ptr<FbFpgaI2cController>, 4>, 8>
      i2cControllers_;
};

} // namespace facebook::fboss
