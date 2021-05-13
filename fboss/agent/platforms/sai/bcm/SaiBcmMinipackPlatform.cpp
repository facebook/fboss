/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/sai/SaiBcmMinipackPlatform.h"

#include "fboss/agent/hw/switch_asics/Tomahawk3Asic.h"
#include "fboss/agent/platforms/common/minipack/MinipackPlatformMapping.h"

#include "fboss/lib/phy/facebook/bcm/minipack/MinipackPhyInterfaceHandler.h"

#include <cstdio>
#include <cstring>
namespace facebook::fboss {

SaiBcmMinipackPlatform::SaiBcmMinipackPlatform(
    std::unique_ptr<PlatformProductInfo> productInfo)
    : SaiBcmPlatform(
          std::move(productInfo),
          std::make_unique<MinipackPlatformMapping>(
              ExternalPhyVersion::MILN5_2)) {
  asic_ = std::make_unique<Tomahawk3Asic>();
  phyInterfaceHandler_ =
      std::make_unique<MinipackPhyInterfaceHandler>(getPlatformMapping());
}

} // namespace facebook::fboss
