// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/led_service/LedManagerInit.h"
#include "fboss/led_service/FujiLedManager.h"
#include "fboss/led_service/MinipackLedManager.h"
#include "fboss/led_service/MontblancLedManager.h"
#include "fboss/lib/platforms/PlatformProductInfo.h"

namespace facebook::fboss {

/*
 * createLedManager
 *
 * createLedManager will create the LED manager class for the given platform
 */
std::unique_ptr<LedManager> createLedManager() {
  auto productInfo =
      std::make_unique<PlatformProductInfo>(FLAGS_fruid_filepath);
  productInfo->initialize();
  auto mode = productInfo->getType();

  if (mode == PlatformType::PLATFORM_MONTBLANC) {
    return std::make_unique<MontblancLedManager>();
  } else if (mode == PlatformType::PLATFORM_FUJI) {
    return std::make_unique<FujiLedManager>();
  } else if (mode == PlatformType::PLATFORM_MINIPACK) {
    return std::make_unique<MinipackLedManager>();
  }
  return nullptr;
}

} // namespace facebook::fboss
