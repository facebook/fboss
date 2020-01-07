/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/lib/i2c/PCA9541.h"
#include <folly/logging/xlog.h>

namespace {

/* In some platforms like Galaxy the QSFP CPLD sits behind the I2C Mux PCA9541
 * which is two upstream to one downstream mux. This needs to be controlled to
 * get the QSFP CPLD access from controllers like CP2112 or the BMC. The PCA9541
 * control register 0x01 will be used for to allow the access from one of the
 * upstream controller to access the downstream device
 */
enum pca9541ControlReg { PCA9541_CONTROL_REG = 0x01 };
/* PCA9541 Control register bit definitions */
enum pca9541ControlRegBits {
  PCA9541_CONTROL_MYBUS = 0x0,
  PCA9541_CONTROL_NMYBUS,
  PCA9541_CONTROL_BUSON,
  PCA9541_CONTROL_NBUSON,
  PCA9541_CONTROL_BUSINIT
};

/* The I2C access to QSFP CPLD is controlled by PCA9541 device. The I2C access
 * can be obtained by writing the PCA9541 control register. If the bus access
 * can't be obtained then we need to retry. The retry logic uses these two
 * numbers for retries in case of failure
 */
constexpr uint8_t NBUS_SELECT_DELAY = 1;
constexpr uint8_t BUS_SELECT_RETRIES = 25;

} // namespace

namespace facebook::fboss {
/* This function checks if the current I2C controller has the I2C bus access to
 * the downstream device like QSFP CPLD in Galaxy. The downstream device sits
 * behind the MUX PCA9541 which is 2 upstream to 1 downstream mux. The bus
 * access can be granted by PCA9541 to the upstream I2C controllers like BMC,
 * CP2112 etc.
 */
bool PCA9541::isBusControlOk() {
  uint8_t readBuf;
  uint8_t busOn, nBusOn, myBus, nMyBus, busInit;
  bool hasBusControl, isBusOn;

  // Read control register of PCA9541
  platformI2cBus_->read(i2cDeviceAddress_, PCA9541_CONTROL_REG, 1, &readBuf);

  busInit = readBuf & (1 << PCA9541_CONTROL_BUSINIT);

  // If the bus initialization request bit is enabled then we need to abort
  // as the bus initialization is going through.
  // Return false in this case as the bus control is still not verified so
  // the caller can retry
  if (busInit) {
    XLOG(DBG0) << "The PCA9541 bus initialization is not complete";
    return false;
  }

  myBus = readBuf & (1 << PCA9541_CONTROL_MYBUS);
  nMyBus = readBuf & (1 << PCA9541_CONTROL_NMYBUS);
  busOn = readBuf & (1 << PCA9541_CONTROL_BUSON);
  nBusOn = readBuf & (1 << PCA9541_CONTROL_NBUSON);

  // As per the spec, if bitfields myBus and nMyBus have same value then
  // the requesting entity has control over the bus
  hasBusControl = ((myBus && nMyBus) || (!myBus && !nMyBus)) ? true : false;

  // As per the spec, if bitfields busOn and nBusOn has different values
  // then the bus is on
  isBusOn = ((busOn && !nBusOn) || (!busOn && nBusOn)) ? true : false;

  // If the bus is On and requesting entity has the control over the bus
  // then retrun true from here
  if (isBusOn && hasBusControl) {
    XLOG(DBG0) << "The current controller has control over PCA9541 I2C Bus";
    return true;
  }

  // Otherwise the requesting entity does not have the control over the bus so
  // return false
  XLOG(DBG0)
      << "The current controller does not have control of the PCA9541 bus, \
          control_reg[0x01]: "
      << readBuf;

  return false;
}

/* This function provides the I2C bus control to the requesting controller for
 * accessing the downstream device like the QSFP CPLD in case of Galaxy. The
 * PCA9541 can provide the I2C bus access towards downstream device to one of
 * the requesting masters like in case of Galaxy we have two masters - BMC or
 * CP2112. This function checks all conditions and makes downstream device
 * acessible to the requesting controller by writing to the PCA control
 * register. Sometime the write operation can fail due to contention so this
 * function has a retry logic also
 */
bool PCA9541::getBusControl() {
  uint8_t readBuf, writeBuf;
  uint8_t numRetries = 0;

  /* The PCA9541 has the state machine for the bus access. The following
   * maps presents the current value of control register and the next value
   * to be written there in order to get the downstream device access to the
   * requesting master
   */
  std::array<uint8_t, 16> actionMap = {
      0x4, // current(0x0): bus off, has control --> turn on bus
      0x4, // current(0x1): bus off, no control --> turn on bus, take control
      0x5, // current(0x2): bus off, no control --> turn on bus, take control
      0x5, // current(0x3): bus off, has control --> turn on bus
      0x4, // current(0x4): bus on, has control --> Nothing to be done
      0x4, // current(0x5): bus on, no control --> take control
      0x5, // current(0x6): bus on, no control --> take control
      0x7, // current(0x7): bus on, has control --> Nothing to be done
      0x8, // current(0x8): bus on, has control --> Nothing to be done
      0x0, // current(0x9): bus on, no control --> take control
      0x1, // current(0xa): bus on, no control --> take control
      0xb, // current(0xb): bus on, has control --> Nothing to be done
      0x0, // current(0xc): bus off, has control --> turn on bus
      0x0, // current(0xd): bus off, no control --> turn on bus, take control
      0x1, // current(0xe): bus off, no control --> turn on bus, take control
      0x1, // current(0xf): bus off, has control --> turn on bus
  };

  // If the current controller does not have the bus access then try to get it
  // by writing to PCA9541 cibtrol registers. Due to bus contention the control
  // register write may fail therefore the retries are done in a loop.
  while (!isBusControlOk() && numRetries < BUS_SELECT_RETRIES) {
    // Get the current value to PCA9541 control register
    platformI2cBus_->read(i2cDeviceAddress_, PCA9541_CONTROL_REG, 1, &readBuf);

    // We are interested in last 4 bnits only - myBus, nMyBus, busOn, nBusOn
    readBuf &= 0x0f;

    // Based on the current value find the next value to be written to the
    // control register in order to get the bus access
    writeBuf = actionMap[readBuf];

    // If the next value to be written to PCA9541 control register is same as
    // the current one then it means currently we have the access to the bus so
    // return true from here in this case
    if (writeBuf == readBuf) {
      XLOG(DBG0)
          << "The current controller has got control over PCA9541 bus now";
      return true;
    }

    XLOG(DBG0) << "PCA9541: current val " << readBuf << " write value "
               << writeBuf;

    platformI2cBus_->write(
        i2cDeviceAddress_, PCA9541_CONTROL_REG, 1, &writeBuf);

    std::this_thread::sleep_for(std::chrono::milliseconds(NBUS_SELECT_DELAY));

    numRetries++;

    XLOG(DBG0)
        << "The current controller could not obtain bus control for PCA9541 after retries: "
        << numRetries;
  }

  // If the bus control is obtained by current controller then return true
  // from here otherwise retry again till the retry limit is reached
  if (isBusControlOk()) {
    XLOG(DBG0) << "The current controller has got control over PCA9541 bus now";
    return true;
  }

  // We are here then it means the bus control was not obtained by the current
  // controller despite all retries. Retrun false from here.
  XLOG(DBG0) << "Controller didn't get bus control after retries "
             << numRetries;

  return false;
}

} // namespace facebook::fboss
