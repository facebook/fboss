// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/test/HwTestThriftHandler.h"

namespace facebook::fboss::utility {

void HwTestThriftHandler::updateFlowletStats() {
  auto bcmSwitch = static_cast<const BcmSwitch*>(hwSwitch_);
  bcmSwitch->getHwFlowletStats();
  return;
}

} // namespace facebook::fboss::utility
