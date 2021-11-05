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
    case PlatformMode::DARWIN:
      throw FbossError("No xphys to check FW version on");
    case PlatformMode::ELBERT:
      desiredFw.version_ref() = 1;
      desiredFw.versionStr_ref() = "1.91";
      desiredFw.minorVersion_ref() = 91;
      break;
    case PlatformMode::FUJI:
      desiredFw.version_ref() = 0xD008;
      desiredFw.crc_ref() = 0x4dcf6a59;
      break;
    case PlatformMode::CLOUDRIPPER:
      desiredFw.version_ref() = 1;
      desiredFw.versionStr_ref() = "1.92";
      desiredFw.minorVersion_ref() = 92;
      break;
    case PlatformMode::MINIPACK:
      desiredFw.version_ref() = 0xD037;
      desiredFw.crc_ref() = 0xbf86d423;
      break;
    case PlatformMode::YAMP:
      desiredFw.version_ref() = 0x3894F5;
      desiredFw.versionStr_ref() = "2.18.2";
      desiredFw.crc_ref() = 0x5B4C;
      desiredFw.dateCode_ref() = 18423;
      break;
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
    const auto& actualFw = xphy->fwVersion();
    EXPECT_EQ(actualFw.version_ref(), desiredFw.version_ref());
    EXPECT_EQ(actualFw.versionStr_ref(), desiredFw.versionStr_ref());
    EXPECT_EQ(actualFw.crc_ref(), desiredFw.crc_ref());
    EXPECT_EQ(actualFw.minorVersion_ref(), desiredFw.minorVersion_ref());
    EXPECT_EQ(actualFw.dateCode_ref(), desiredFw.dateCode_ref());
  }
}
} // namespace facebook::fboss
