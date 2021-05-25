/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/qsfp_service/test/hw_test/HwTest.h"

#include "fboss/agent/FbossError.h"
#include "fboss/agent/platforms/common/PlatformProductInfo.h"
#include "fboss/lib/fpga/MultiPimPlatformSystemContainer.h"
#include "fboss/lib/phy/PhyManager.h"
#include "fboss/qsfp_service/test/hw_test/phy/HwPhyEnsemble.h"

namespace facebook::fboss {

void HwTest::SetUp() {
  ensemble_ = std::make_unique<HwPhyEnsemble>();
  ensemble_->init();
}

void HwTest::TearDown() {
  ensemble_.reset();
}

MultiPimPlatformPimContainer* HwTest::getPimContainer(int pimID) {
  return ensemble_->getPhyManager()->getSystemContainer()->getPimContainer(
      pimID);
}
} // namespace facebook::fboss
