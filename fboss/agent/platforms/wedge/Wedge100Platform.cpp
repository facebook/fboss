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
#include <folly/logging/xlog.h>

#include <chrono>

namespace {
constexpr auto kMaxSetLedTime = std::chrono::seconds(1);
}

namespace facebook { namespace fboss {

std::unique_ptr<BaseWedgeI2CBus> Wedge100Platform::getI2CBus() {
  return std::make_unique<Wedge100I2CBus>();
}

std::unique_ptr<WedgePortMapping> Wedge100Platform::createPortMapping() {
  WedgePortMapping::PortTransceiverMap ports = {
    {PortID(118), TransceiverID(0)},
    {PortID(122), TransceiverID(1)},
    {PortID(126), TransceiverID(2)},
    {PortID(130), TransceiverID(3)},
    {PortID(1), TransceiverID(4)},
    {PortID(5), TransceiverID(5)},
    {PortID(9), TransceiverID(6)},
    {PortID(13), TransceiverID(7)},
    {PortID(17), TransceiverID(8)},
    {PortID(21), TransceiverID(9)},
    {PortID(25), TransceiverID(10)},
    {PortID(29), TransceiverID(11)},
    {PortID(34), TransceiverID(12)},
    {PortID(38), TransceiverID(13)},
    {PortID(42), TransceiverID(14)},
    {PortID(46), TransceiverID(15)},
    {PortID(50), TransceiverID(16)},
    {PortID(54), TransceiverID(17)},
    {PortID(58), TransceiverID(18)},
    {PortID(62), TransceiverID(19)},
    {PortID(68), TransceiverID(20)},
    {PortID(72), TransceiverID(21)},
    {PortID(76), TransceiverID(22)},
    {PortID(80), TransceiverID(23)},
    {PortID(84), TransceiverID(24)},
    {PortID(88), TransceiverID(25)},
    {PortID(92), TransceiverID(26)},
    {PortID(96), TransceiverID(27)},
    {PortID(102), TransceiverID(28)},
    {PortID(106), TransceiverID(29)},
    {PortID(110), TransceiverID(30)},
    {PortID(114), TransceiverID(31)}
  };
  return WedgePortMapping::create<WedgePortMappingT<Wedge100Port>>(this, ports);
}

void Wedge100Platform::enableLedMode() {
  // TODO: adding retries adds tolerance in case the i2c bus is
  // busy. Long-term, we should think about having all i2c io go
  // through qsfp_service, though this feels a bit out of place there.
  auto expireTime = std::chrono::steady_clock::now() + kMaxSetLedTime;
  uint8_t mode = TWELVE_BIT_MODE;
  while (true) {
    try {
      WedgeI2CBusLock(getI2CBus()).write(ADDR_SYSCPLD, LED_MODE_REG, 1, &mode);
      XLOG(DBG0) << "Successfully set LED mode to '12-bit' mode";
      return;
    } catch (const std::exception& ex) {
      if (std::chrono::steady_clock::now() > expireTime) {
        XLOG(ERR) << __func__
                  << ": failed to change LED mode: " << folly::exceptionStr(ex);
        return;
      }
    }
    usleep(100);
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
