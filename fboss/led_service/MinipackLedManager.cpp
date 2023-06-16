// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/led_service/MinipackLedManager.h"
#include "fboss/agent/EnumUtils.h"
#include "fboss/agent/platforms/common/minipack/MinipackPlatformMapping.h"
#include "fboss/lib/CommonFileUtils.h"
#include "fboss/lib/CommonPortUtils.h"
#include "fboss/lib/fpga/MinipackLed.h"
#include "fboss/lib/fpga/MinipackSystemContainer.h"

namespace facebook::fboss {

/*
 * MinipackLedManager ctor()
 *
 * MinipackLedManager constructor will just call based LedManager ctor() as of
 * now.
 */
MinipackLedManager::MinipackLedManager() : MinipackBaseLedManager() {
  // Platform mapping
  platformMapping_ = std::make_unique<MinipackPlatformMapping>(
      ExternalPhyVersion::MILN5_2, "");

  XLOG(INFO) << "Created Minipack LED Manager";
}

/*
 * setLedColor
 *
 * Set the LED color in HW for the LED on a given port. This function should
 * not depend on FSDB provided values from portDisplayMap_
 */
void MinipackLedManager::setLedColor(
    uint32_t portId,
    cfg::PortProfileID /* portProfile */,
    led::LedColor ledColor) {
  if (ledColor == portDisplayMap_[portId].currentLedColor) {
    // New LED color is same as current LED color
    return;
  }
  portDisplayMap_[portId].currentLedColor = ledColor;

  // MinipackLed::Color contains FPGA register write values so convert the LED
  // color to Minipack FPGA register values
  MinipackLed::Color color;
  if (ledColor == led::LedColor::BLUE) {
    color = MinipackLed::Color::BLUE;
  } else {
    color = MinipackLed::Color::OFF;
  }

  auto portName = portDisplayMap_[portId].portName;
  MinipackSystemContainer::getInstance()
      ->getPimContainer(getPimID(portName))
      ->getLedController(getTransceiverIndexInPim(portName))
      ->setColor(color);
}

} // namespace facebook::fboss
