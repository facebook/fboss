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
#include "fboss/agent/platforms/common/PlatformMode.h"
#include "fboss/lib/phy/ExternalPhy.h"
#include "fboss/lib/phy/PhyManager.h"
#include "fboss/qsfp_service/test/hw_test/phy/HwPhyEnsemble.h"

namespace facebook::fboss {

TEST_F(HwTest, CheckDefaultFirmwareVersion) {
  auto chips = getHwPhyEnsemble()->getPlatformMapping()->getChips();
  if (!chips.size()) {
    return;
  }

  phy::PhyFwVersion desiredFw;
  auto platformMode = getHwPhyEnsemble()->getWedgeManager()->getPlatformMode();
  switch (platformMode) {
    case PlatformMode::ELBERT:
      desiredFw.version_ref() = 91;
      desiredFw.versionStr_ref() = "91.1";
      desiredFw.minorVersion_ref() = 1;
      break;
    default:
      throw FbossError("Fill in desired f/w version for: ", platformMode);
  }
  for (auto chip : chips) {
    if (chip.second.get_type() != phy::DataPlanePhyChipType::XPHY) {
      continue;
    }
    auto xphy = getHwPhyEnsemble()->getPhyManager()->getExternalPhy(
        GlobalXphyID(chip.second.get_physicalID()));
    EXPECT_EQ(xphy->fwVersion(), desiredFw);
  }
}
} // namespace facebook::fboss
