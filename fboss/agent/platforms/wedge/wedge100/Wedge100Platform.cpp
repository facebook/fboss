/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/platforms/wedge/wedge100/Wedge100Platform.h"

#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/bcm/BcmAPI.h"
#include "fboss/agent/hw/bcm/BcmUnit.h"
#include "fboss/agent/platforms/common/utils/Wedge100LedUtils.h"
#include "fboss/agent/platforms/common/wedge100/Wedge100PlatformMapping.h"
#include "fboss/agent/platforms/wedge/WedgePortMapping.h"
#include "fboss/agent/platforms/wedge/wedge100/Wedge100Port.h"
#include "fboss/lib/platforms/PlatformProductInfo.h"
#include "fboss/lib/usb/Wedge100I2CBus.h"
#include "fboss/qsfp_service/platforms/wedge/WedgeI2CBusLock.h"

#include <folly/Memory.h>
#include <folly/Range.h>
#include <folly/logging/xlog.h>

#include <chrono>

namespace facebook::fboss {

Wedge100Platform::Wedge100Platform(
    std::unique_ptr<PlatformProductInfo> productInfo,
    folly::MacAddress localMac)
    : WedgeTomahawkPlatform(
          std::move(productInfo),
          std::make_unique<Wedge100PlatformMapping>(),
          localMac) {}

std::unique_ptr<BaseWedgeI2CBus> Wedge100Platform::getI2CBus() {
  return std::make_unique<Wedge100I2CBus>();
}

std::unique_ptr<WedgePortMapping> Wedge100Platform::createPortMapping() {
  return WedgePortMapping::createFromConfig<
      WedgePortMappingT<Wedge100Platform, Wedge100Port>>(this);
}

void Wedge100Platform::enableLedMode() {
  Wedge100LedUtils::enableLedMode();
}

void Wedge100Platform::onHwInitialized(HwSwitchCallback* sw) {
  WedgePlatform::onHwInitialized(sw);
  enableLedMode();
}

void Wedge100Platform::onUnitAttach(int unit) {
  setPciPreemphasis(unit);
}

folly::ByteRange Wedge100Platform::defaultLed0Code() {
  return Wedge100LedUtils::defaultLedCode();
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
