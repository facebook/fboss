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

#include <stdint.h>
#include "fboss/lib/usb/BaseWedgeI2CBus.h"

namespace facebook::fboss {
/* This class represents the PCA9541 device on platforms like Galaxy. This is
 * 2 to 1 upstream to downstream mapping device accessible through I2C. In
 * Galaxy it allows either Cp2112 or the BMC to access the QSFP CPLD on the
 * I2C bus. This class provides function to allow a master to access the
 * downstream device. Thsi class needs platform specific i2c object to do the
 * I2C read and write operation
 */

class PCA9541 {
 public:
  explicit PCA9541(BaseWedgeI2CBus* platformI2cBus, uint8_t i2cDeviceAddress)
      : platformI2cBus_(platformI2cBus), i2cDeviceAddress_(i2cDeviceAddress) {}

  ~PCA9541() {}

  // This function checks if the CP2112 has the I2C bus access to the QSFP
  // CPLD. The QSFP CPLD sits behind the MUX PCA9541 which is 2 upstream to
  // 1 downstream mux. The bus access can be granted by PCA9541 to BMC or
  // to CP2112.
  bool isBusControlOk();

  // This function provides the I2C bus control to CP2112 for accessing the QSFP
  // CPLD by writing to the PCA9541 device. The PCA9541 can provide the I2C bus
  // access towards QSFP CPLD to one of the two masters - BMC or CP2112. This
  // function checks all conditions and makes QSFP CPLD acessible to CP2112
  // by writing to the PCA control register.
  // Sometime the write operation can fail due to contention so this function
  // has a retry logic also
  bool getBusControl();

 private:
  // Stores the pointer to the platform specific I2C bus object so that we can
  // use the I2C read and write() API from there later
  BaseWedgeI2CBus* platformI2cBus_;

  // I2C device address for this PCA9541 device to be accessed from controller
  uint8_t i2cDeviceAddress_;
};

} // namespace facebook::fboss
