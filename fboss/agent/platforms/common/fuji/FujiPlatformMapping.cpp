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
#include <folly/logging/xlog.h>
#include "fboss/agent/platforms/common/fuji/Fuji16QPimPlatformMapping.h"
#include "fboss/lib/fpga/facebook/fuji/FujiSystemContainer.h"

namespace facebook {
namespace fboss {
FujiPlatformMapping::FujiPlatformMapping() {
  auto fuji16Q = std::make_unique<Fuji16QPimPlatformMapping>();
  // TODO: Define 16O platform mapping
  auto fuji16O = std::make_unique<Fuji16QPimPlatformMapping>();
  for (uint8_t pimID = 2; pimID < 10; pimID++) {
    MultiPimPlatformPimContainer::PimType pimType;
    if (std::getenv("BCM_SIM_PATH")) {
      // Assume 16Q for TH4 bcmsim to avoid accessing undelrying pci device
      // in FujiSystemContainer::getInstance()
      pimType = MultiPimPlatformPimContainer::PimType::FUJI_16Q;
    } else {
      pimType = FujiSystemContainer::getInstance()->getPimType(pimID);
    }
    if (pimType == MultiPimPlatformPimContainer::PimType::FUJI_16O) {
      XLOG(INFO) << "Detected pim:" << static_cast<int>(pimID) << " is 16O";
      this->merge(fuji16O->getPimPlatformMapping(pimID));
    } else {
      XLOG(INFO) << "Detected pim:" << static_cast<int>(pimID) << " is 16Q";
      this->merge(fuji16Q->getPimPlatformMapping(pimID));
    }
  }
}
} // namespace fboss
} // namespace facebook
