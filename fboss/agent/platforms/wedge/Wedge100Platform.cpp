/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/platforms/wedge/Wedge100Platform.h"

#include "fboss/agent/platforms/wedge/WedgePortMapping.h"
#include "fboss/agent/platforms/wedge/Wedge100Port.h"
#include "fboss/agent/FbossError.h"
#include "fboss/lib/usb/Wedge100I2CBus.h"
#include "fboss/qsfp_service/platforms/wedge/WedgeI2CBusLock.h"

#include <folly/Memory.h>

namespace facebook { namespace fboss {

std::unique_ptr<BaseWedgeI2CBus> Wedge100Platform::getI2CBus() {
  return std::make_unique<Wedge100I2CBus>();
}

std::unique_ptr<WedgePortMapping> Wedge100Platform::createPortMapping() {
  // FC == Falcon Core
  // Wedge100 is based on Tomahawk, which divides MMU buffer into 4
  // sub buffers called XPEs. FC are then mapped, to each XPE like so
  // Traffic egressing on FC0-15 use XPE 0, 2
  // Traffic egressing on FC16-13 use XPE 1, 3
  // Port to FC mappings are fixed for TH and are used as input to
  // creating bcm.conf files.
  WedgePortMapping::Port2TransceiverAndXPEs ports = {
    {PortID(118), TransceiverAndXPEs(TransceiverID(0), {1, 3})}, // FC28
    {PortID(122), TransceiverAndXPEs(TransceiverID(1), {1, 3})}, // FC29
    {PortID(126), TransceiverAndXPEs(TransceiverID(2), {1, 3})}, // FC30
    {PortID(130), TransceiverAndXPEs(TransceiverID(3), {1, 3})}, // FC31
    {PortID(1), TransceiverAndXPEs(TransceiverID(4), {0, 2})}, // FC0
    {PortID(5), TransceiverAndXPEs(TransceiverID(5), {0, 2})}, // FC1
    {PortID(9), TransceiverAndXPEs(TransceiverID(6), {0, 2})}, // FC2
    {PortID(13), TransceiverAndXPEs(TransceiverID(7), {0, 2})},// FC3
    {PortID(17), TransceiverAndXPEs(TransceiverID(8), {0, 2})}, // FC4
    {PortID(21), TransceiverAndXPEs(TransceiverID(9), {0, 2})}, // FC5
    {PortID(25), TransceiverAndXPEs(TransceiverID(10), {0, 2})}, // FC6
    {PortID(29), TransceiverAndXPEs(TransceiverID(11), {0, 2})}, // FC7
    {PortID(34), TransceiverAndXPEs(TransceiverID(12), {0, 2})}, // FC8
    {PortID(38), TransceiverAndXPEs(TransceiverID(13), {0, 2})}, // FC9
    {PortID(42), TransceiverAndXPEs(TransceiverID(14), {0, 2})}, // FC10
    {PortID(46), TransceiverAndXPEs(TransceiverID(15), {0, 2})}, // FC11
    {PortID(50), TransceiverAndXPEs(TransceiverID(16), {0, 2})}, // FC12
    {PortID(54), TransceiverAndXPEs(TransceiverID(17), {0, 2})}, // FC13
    {PortID(58), TransceiverAndXPEs(TransceiverID(18), {0, 2})}, // FC14
    {PortID(62), TransceiverAndXPEs(TransceiverID(19), {0, 2})}, // FC15
    {PortID(68), TransceiverAndXPEs(TransceiverID(20), {1, 3})}, // FC16
    {PortID(72), TransceiverAndXPEs(TransceiverID(21), {1, 3})}, // FC17
    {PortID(76), TransceiverAndXPEs(TransceiverID(22), {1, 3})}, // FC18
    {PortID(80), TransceiverAndXPEs(TransceiverID(23), {1, 3})}, // FC19
    {PortID(84), TransceiverAndXPEs(TransceiverID(24), {1, 3})}, // FC20
    {PortID(88), TransceiverAndXPEs(TransceiverID(25), {1, 3})}, // FC21
    {PortID(92), TransceiverAndXPEs(TransceiverID(26), {1, 3})}, // FC22
    {PortID(96), TransceiverAndXPEs(TransceiverID(27), {1, 3})}, // FC23
    {PortID(102), TransceiverAndXPEs(TransceiverID(28), {1, 3})}, // FC24
    {PortID(106), TransceiverAndXPEs(TransceiverID(29), {1, 3})}, // FC25
    {PortID(110), TransceiverAndXPEs(TransceiverID(30), {1, 3})}, // FC26
    {PortID(114), TransceiverAndXPEs(TransceiverID(31), {1, 3})}, // FC27
  };
  return WedgePortMapping::create<WedgePortMappingT<Wedge100Port>>(this, ports);
}

void Wedge100Platform::enableLedMode() {
  uint8_t mode = TWELVE_BIT_MODE;
  try {
    WedgeI2CBusLock(getI2CBus()).write(ADDR_SYSCPLD, LED_MODE_REG, 1, &mode);
  } catch (const std::exception& ex) {
    LOG(ERROR) << __func__ << ": failed to change LED mode: "
               << folly::exceptionStr(ex);
  }
}

void Wedge100Platform::onHwInitialized(SwSwitch* sw) {
  WedgePlatform::onHwInitialized(sw);
  enableLedMode();
}

void Wedge100Platform::onUnitAttach(int unit) {
  setPciPreemphasis(unit);
}

}} // facebook::fboss
