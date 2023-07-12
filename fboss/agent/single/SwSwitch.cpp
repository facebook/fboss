// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/single/MonolithicHwSwitchHandler.h"

namespace facebook::fboss {

HwSwitch* SwSwitch::getHw_DEPRECATED() const {
  const auto* hwSwitchHandler =
      dynamic_cast<const MonolinithicHwSwitchHandler*>(getHwSwitchHandler());
  CHECK(hwSwitchHandler);
  return hwSwitchHandler->getHwSwitch();
}

const Platform* SwSwitch::getPlatform_DEPRECATED() const {
  const auto* hwSwitchHandler =
      dynamic_cast<const MonolinithicHwSwitchHandler*>(getHwSwitchHandler());
  CHECK(hwSwitchHandler);
  return hwSwitchHandler->getPlatform();
}

Platform* SwSwitch::getPlatform_DEPRECATED() {
  auto* hwSwitchHandler =
      dynamic_cast<const MonolinithicHwSwitchHandler*>(getHwSwitchHandler());
  CHECK(hwSwitchHandler);
  return hwSwitchHandler->getPlatform();
}

} // namespace facebook::fboss
