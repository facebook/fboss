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
#include "fboss/qsfp_service/test/hw_test/HwQsfpEnsemble.h"

#include <folly/logging/xlog.h>

namespace facebook::fboss {

TEST_F(HwTest, CheckDefaultXphyFirmwareVersion) {
  auto platformMode = getHwQsfpEnsemble()->getWedgeManager()->getPlatformMode();

  phy::PhyFwVersion desiredFw;
  switch (platformMode) {
    case PlatformMode::WEDGE:
    case PlatformMode::WEDGE100:
    case PlatformMode::GALAXY_LC:
    case PlatformMode::GALAXY_FC:
    case PlatformMode::FAKE_WEDGE:
    case PlatformMode::FAKE_WEDGE40:
    case PlatformMode::WEDGE400C:
    case PlatformMode::WEDGE400C_SIM:
    case PlatformMode::WEDGE400:
      throw FbossError(" No xphys to check FW version on");
    case PlatformMode::ELBERT:
      desiredFw.version_ref() = 91;
      desiredFw.versionStr_ref() = "91.1";
      desiredFw.minorVersion_ref() = 1;
      break;
    case PlatformMode::FUJI:
      desiredFw.version_ref() = 0xD006;
      desiredFw.crc_ref() = 0x77265d92;
      break;
    case PlatformMode::MINIPACK:
    case PlatformMode::CLOUDRIPPER:
    case PlatformMode::YAMP:
      throw FbossError(
          "Fill in desired f/w version for: ", toString(platformMode));
  }

  auto chips = getHwQsfpEnsemble()->getPlatformMapping()->getChips();
  if (!chips.size()) {
    return;
  }
  for (auto chip : chips) {
    if (chip.second.get_type() != phy::DataPlanePhyChipType::XPHY) {
      continue;
    }
    auto xphy = getHwQsfpEnsemble()->getPhyManager()->getExternalPhy(
        GlobalXphyID(chip.second.get_physicalID()));
    EXPECT_EQ(xphy->fwVersion().get_version(), desiredFw.get_version());
    EXPECT_EQ(xphy->fwVersion().get_crc(), desiredFw.get_crc());
  }
}
} // namespace facebook::fboss
