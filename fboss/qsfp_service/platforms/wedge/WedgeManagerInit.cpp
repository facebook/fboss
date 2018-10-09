/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/qsfp_service/platforms/wedge/WedgeManagerInit.h"

#include "fboss/agent/platforms/wedge/WedgeProductInfo.h"
#include "fboss/qsfp_service/platforms/wedge/GalaxyManager.h"
#include "fboss/qsfp_service/platforms/wedge/Wedge40Manager.h"
#include "fboss/qsfp_service/platforms/wedge/Wedge100Manager.h"

namespace facebook { namespace fboss {

std::unique_ptr<TransceiverManager> createTransceiverManager() {
  auto productInfo = std::make_unique<WedgeProductInfo>(FLAGS_fruid_filepath);
  productInfo->initialize();
  auto mode = productInfo->getMode();

  if (mode == WedgePlatformMode::WEDGE100) {
    return std::make_unique<Wedge100Manager>();
  } else if (
      mode == WedgePlatformMode::GALAXY_LC ||
      mode == WedgePlatformMode::GALAXY_FC) {
    return std::make_unique<GalaxyManager>();
  } else if (mode == WedgePlatformMode::MINIPACK) {
    return createFBTransceiverManager(std::move(productInfo));
  } else if (mode == WedgePlatformMode::YAMP) {
    return createYampTransceiverManager();
  }
  return std::make_unique<Wedge40Manager>();
}
}} // facebook::fboss
