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

#include "fboss/agent/platforms/common/PlatformMapping.h"
#include "fboss/lib/phy/ExternalPhy.h"
#include "fboss/lib/phy/PhyManager.h"
#include "fboss/qsfp_service/test/hw_test/phy/HwPhyEnsemble.h"

namespace facebook::fboss {

TEST_F(HwTest, CheckDefaultFirmwareVersion) {
  auto chips = getHwPhyEnsemble()->getPlatformMapping()->getChips();
  for (auto chip : chips) {
    if (chip.second.get_type() != phy::DataPlanePhyChipType::XPHY) {
      continue;
    }
    auto xphy = getHwPhyEnsemble()->getPhyManager()->getExternalPhy(
        GlobalXphyID(chip.second.get_physicalID()));
    EXPECT_EQ(xphy->fwVersion(), getHwPhyEnsemble()->getInitInfo().fwVersion);
  }
}
} // namespace facebook::fboss
