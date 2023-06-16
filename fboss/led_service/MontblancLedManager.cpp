// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/led_service/MontblancLedManager.h"
#include "fboss/agent/EnumUtils.h"
#include "fboss/agent/platforms/common/montblanc/MontblancPlatformMapping.h"
#include "fboss/lib/CommonFileUtils.h"
#include "fboss/lib/CommonPortUtils.h"
#include "fboss/lib/bsp/BspGenericSystemContainer.h"
#include "fboss/lib/bsp/montblanc/MontblancBspPlatformMapping.h"

namespace facebook::fboss {

/*
 * MontblancLedManager ctor()
 *
 * MontblancLedManager constructor will create the LedManager object for
 * Montblanc platform
 */
MontblancLedManager::MontblancLedManager() : BspLedManager() {
  bspSystemContainer_ =
      BspGenericSystemContainer<MontblancBspPlatformMapping>::getInstance()
          .get();

  platformMapping_ = std::make_unique<MontblancPlatformMapping>();

  XLOG(INFO) << "Created Montblanc BSP LED Manager";
}

} // namespace facebook::fboss
