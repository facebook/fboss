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

#include "fboss/lib/fpga/MultiPimPlatformPimContainer.h"
#include "fboss/lib/fpga/MultiPimPlatformSystemContainer.h"
#include "fboss/lib/phy/PhyManager.h"
#include "fboss/qsfp_service/test/hw_test/HwQsfpEnsemble.h"

namespace facebook::fboss {

class HwPimTest : public HwTest {
  std::vector<qsfp_production_features::QsfpProductionFeature>
  getProductionFeatures() const override {
    return {qsfp_production_features::QsfpProductionFeature::PIM};
  }
};

TEST_F(HwPimTest, CheckPimPresent) {
  auto phyManager = getHwQsfpEnsemble()->getPhyManager();
  EXPECT_EQ(
      phyManager->getNumOfSlot(),
      phyManager->getSystemContainer()->getNumPims());

  auto lastPimState = getHwQsfpEnsemble()->getWedgeManager()->getLastPimState();
  EXPECT_EQ(lastPimState.size(), phyManager->getNumOfSlot());
  for (auto& [pim, pimState] : lastPimState) {
    EXPECT_TRUE(pimState.errors()->empty());
  }
}
} // namespace facebook::fboss
