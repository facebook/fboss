/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/common/fuji/FujiPlatformMapping.h"
#include "fboss/agent/platforms/common/fuji/Fuji16QPimPlatformMapping.h"
#include "fboss/lib/fpga/facebook/fuji/FujiSystemContainer.h"

namespace facebook {
namespace fboss {
FujiPlatformMapping::FujiPlatformMapping(
    const std::string& platformMappingStr) {
  auto fuji16Q = platformMappingStr.empty()
      ? std::make_unique<Fuji16QPimPlatformMapping>()
      : std::make_unique<Fuji16QPimPlatformMapping>(platformMappingStr);
  if (std::getenv("BCM_SIM_PATH") || FLAGS_force_5pim_fuji) {
    for (uint8_t pimID = 2; pimID < 10; pimID++) {
      this->merge(fuji16Q->getPimPlatformMapping(pimID));
    }
  }
}
} // namespace fboss
} // namespace facebook
