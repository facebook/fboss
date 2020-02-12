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

#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/bcm/BcmAPI.h"
#include "fboss/agent/hw/bcm/BcmUnit.h"
#include "fboss/agent/platforms/wedge/Wedge100Port.h"
#include "fboss/agent/platforms/wedge/WedgePortMapping.h"
#include "fboss/lib/usb/Wedge100I2CBus.h"
#include "fboss/qsfp_service/platforms/wedge/WedgeI2CBusLock.h"

#include <folly/Memory.h>
#include <folly/Range.h>
#include <folly/logging/xlog.h>

#include <chrono>

namespace {
constexpr auto kMaxSetLedTime = std::chrono::seconds(1);

/* Auto-generated using the Broadcom ledasm tool */
// clang-format off
const auto kWedge100LedCode = folly::make_array<unsigned char>(
    0x02, 0x3F, 0x12, 0xC0, 0xF8, 0x15, 0x67, 0x0D, 0x90, 0x75, 0x02, 0x3A,
    0xC0, 0x21, 0x87, 0x99, 0x21, 0x87, 0x99, 0x21, 0x87, 0x57, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00
);
// clang-format off
} // namespace

namespace facebook::fboss {

std::unique_ptr<BaseWedgeI2CBus> Wedge100Platform::getI2CBus() {
  return std::make_unique<Wedge100I2CBus>();
}

std::unique_ptr<WedgePortMapping> Wedge100Platform::createPortMapping() {
  WedgePortMapping::PortTransceiverMap ports = {
      {PortID(118), TransceiverID(0)},  {PortID(122), TransceiverID(1)},
      {PortID(126), TransceiverID(2)},  {PortID(130), TransceiverID(3)},
      {PortID(1), TransceiverID(4)},    {PortID(5), TransceiverID(5)},
      {PortID(9), TransceiverID(6)},    {PortID(13), TransceiverID(7)},
      {PortID(17), TransceiverID(8)},   {PortID(21), TransceiverID(9)},
      {PortID(25), TransceiverID(10)},  {PortID(29), TransceiverID(11)},
      {PortID(34), TransceiverID(12)},  {PortID(38), TransceiverID(13)},
      {PortID(42), TransceiverID(14)},  {PortID(46), TransceiverID(15)},
      {PortID(50), TransceiverID(16)},  {PortID(54), TransceiverID(17)},
      {PortID(58), TransceiverID(18)},  {PortID(62), TransceiverID(19)},
      {PortID(68), TransceiverID(20)},  {PortID(72), TransceiverID(21)},
      {PortID(76), TransceiverID(22)},  {PortID(80), TransceiverID(23)},
      {PortID(84), TransceiverID(24)},  {PortID(88), TransceiverID(25)},
      {PortID(92), TransceiverID(26)},  {PortID(96), TransceiverID(27)},
      {PortID(102), TransceiverID(28)}, {PortID(106), TransceiverID(29)},
      {PortID(110), TransceiverID(30)}, {PortID(114), TransceiverID(31)}};
  return WedgePortMapping::create<
      WedgePortMappingT<Wedge100Platform, Wedge100Port>>(this, ports);
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

folly::ByteRange Wedge100Platform::defaultLed0Code() {
  return folly::ByteRange(kWedge100LedCode.data(), sizeof(kWedge100LedCode));
}

folly::ByteRange Wedge100Platform::defaultLed1Code() {
  return defaultLed0Code();
}
void Wedge100Platform::setPciPreemphasis(int unit) const {
  /* This sets the PCIe pre-emphasis values on the tomahawk chip that
   * is needed on the Wedge100 board. It is unfortunate that this
   * includes bcm sdk calls, but the logic is only specific to the
   * wedge100 board so I think the logic still makes sense to be in
   * the platform.
   *
   * These values were determined by the HW team during testing.
   */
  auto phy = 0x1c5;
  auto reg1 = 0x1f;
  auto reg2 = 0x14;
  auto unitObj = BcmAPI::getUnit(unit);

  unitObj->rawRegisterWrite(phy, reg1, 0x4000);
  unitObj->rawRegisterWrite(phy, reg2, 0x6620);
  unitObj->rawRegisterWrite(phy, reg1, 0x4010);
  unitObj->rawRegisterWrite(phy, reg2, 0x6620);
  unitObj->rawRegisterWrite(phy, reg1, 0x4020);
  unitObj->rawRegisterWrite(phy, reg2, 0x6620);
  unitObj->rawRegisterWrite(phy, reg1, 0x4030);
  unitObj->rawRegisterWrite(phy, reg2, 0x6620);
}

} // namespace facebook::fboss
