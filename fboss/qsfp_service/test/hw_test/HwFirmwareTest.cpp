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
#include "fboss/lib/platforms/PlatformMode.h"
#include "fboss/qsfp_service/test/hw_test/HwQsfpEnsemble.h"

#include <folly/logging/xlog.h>

namespace facebook::fboss {

TEST_F(HwTest, CheckDefaultXphyFirmwareVersion) {
  auto platformType = getHwQsfpEnsemble()->getWedgeManager()->getPlatformType();

  phy::PhyFwVersion desiredFw;
  switch (platformType) {
    case PlatformType::PLATFORM_WEDGE:
    case PlatformType::PLATFORM_WEDGE100:
    case PlatformType::PLATFORM_GALAXY_LC:
    case PlatformType::PLATFORM_GALAXY_FC:
    case PlatformType::PLATFORM_FAKE_WEDGE:
    case PlatformType::PLATFORM_FAKE_WEDGE40:
    case PlatformType::PLATFORM_WEDGE400C:
    case PlatformType::PLATFORM_WEDGE400C_SIM:
    case PlatformType::PLATFORM_WEDGE400C_VOQ:
    case PlatformType::PLATFORM_WEDGE400C_FABRIC:
    case PlatformType::PLATFORM_WEDGE400C_GRANDTETON:
    case PlatformType::PLATFORM_WEDGE400:
    case PlatformType::PLATFORM_WEDGE400_GRANDTETON:
    case PlatformType::PLATFORM_DARWIN:
    case PlatformType::PLATFORM_LASSEN:
    case PlatformType::PLATFORM_MERU400BIU:
    case PlatformType::PLATFORM_MERU800BIA:
    case PlatformType::PLATFORM_MERU800BFA:
    case PlatformType::PLATFORM_MERU400BIA:
    case PlatformType::PLATFORM_MERU400BFU:
    case PlatformType::PLATFORM_MONTBLANC:
    case PlatformType::PLATFORM_MORGAN800CC:
      throw FbossError("No xphys to check FW version on");
    case PlatformType::PLATFORM_ELBERT:
      desiredFw.version() = 1;
      desiredFw.versionStr() = "1.93";
      desiredFw.minorVersion() = 93;
      break;
    case PlatformType::PLATFORM_FUJI:
      desiredFw.version() = 0xD008;
      desiredFw.crc() = 0x4dcf6a59;
      break;
    case PlatformType::PLATFORM_CLOUDRIPPER:
    case PlatformType::PLATFORM_CLOUDRIPPER_VOQ:
    case PlatformType::PLATFORM_CLOUDRIPPER_FABRIC:
      desiredFw.version() = 1;
      desiredFw.versionStr() = "1.92";
      desiredFw.minorVersion() = 92;
      break;
    case PlatformType::PLATFORM_MINIPACK:
      desiredFw.version() = 0xD037;
      desiredFw.crc() = 0xbf86d423;
      break;
    case PlatformType::PLATFORM_YAMP:
      desiredFw.version() = 0x3894F5;
      desiredFw.versionStr() = "2.18.2";
      desiredFw.crc() = 0x5B4C;
      desiredFw.dateCode() = 18423;
      break;
    case PlatformType::PLATFORM_SANDIA:
      desiredFw.version() = 0;
      desiredFw.versionStr() = "0.0";
      desiredFw.minorVersion() = 0;
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

    // There are some Sandia PIM's which does not have XPHY but the platform
    // mapping is generic so we need to first check if the external Phy is
    // present before attempting this test
    phy::ExternalPhy* xphy;
    try {
      xphy = getHwQsfpEnsemble()->getPhyManager()->getExternalPhy(
          GlobalXphyID(chip.second.get_physicalID()));
    } catch (FbossError& e) {
      XLOG(ERR) << "XPHY not present in system "
                << GlobalXphyID(chip.second.get_physicalID());
      continue;
    }

    const auto& actualFw = xphy->fwVersion();
    EXPECT_EQ(actualFw.version(), desiredFw.version());
    EXPECT_EQ(actualFw.versionStr(), desiredFw.versionStr());
    EXPECT_EQ(actualFw.crc(), desiredFw.crc());
    EXPECT_EQ(actualFw.minorVersion(), desiredFw.minorVersion());
    EXPECT_EQ(actualFw.dateCode(), desiredFw.dateCode());
  }
}
} // namespace facebook::fboss
