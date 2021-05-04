/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/qsfp_service/test/hw_test/phy/HwPhyEnsemble.h"

#include "fboss/agent/FbossError.h"
#include "fboss/lib/fpga/MultiPimPlatformSystemContainer.h"
#include "fboss/lib/phy/PhyManager.h"

namespace facebook::fboss {
HwPhyEnsemble::HwPhyEnsemble() {}

HwPhyEnsemble::~HwPhyEnsemble() {}

int8_t HwPhyEnsemble::getFirstAvailablePimID(
    MultiPimPlatformPimContainer::PimType pimType) {
  auto pimStartNum = phyManager_->getSystemContainer()->getPimStartNum();
  for (auto i = 0; i < phyManager_->getNumOfSlot(); ++i) {
    if (pimType ==
        phyManager_->getSystemContainer()->getPimType(i + pimStartNum)) {
      return i + pimStartNum;
    }
  }
  throw FbossError("Can't find pimType:", pimType, " pim in this platform");
}
} // namespace facebook::fboss
