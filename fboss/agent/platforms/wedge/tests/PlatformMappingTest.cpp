/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/wedge/tests/PlatformMappingTest.h"

#include "fboss/agent/platforms/wedge/minipack/Minipack16QPimPlatformMapping.h"
#include "fboss/agent/platforms/wedge/wedge400/Wedge400PlatformMapping.h"
#include "fboss/agent/platforms/wedge/yamp/YampPlatformMapping.h"

#include <gtest/gtest.h>

namespace facebook {
namespace fboss {
namespace test {

TEST_F(PlatformMappingTest, VerifyWedge400PlatformMapping) {
  // supported profiles
  std::vector<cfg::PortProfileID> expectedProfiles = {
      cfg::PortProfileID::PROFILE_10G_1_NRZ_NOFEC_COPPER,
      cfg::PortProfileID::PROFILE_25G_1_NRZ_NOFEC_COPPER,
      cfg::PortProfileID::PROFILE_40G_4_NRZ_NOFEC_COPPER,
      cfg::PortProfileID::PROFILE_50G_2_NRZ_NOFEC_COPPER,
      cfg::PortProfileID::PROFILE_100G_4_NRZ_RS528_COPPER,
      cfg::PortProfileID::PROFILE_100G_4_NRZ_RS528_OPTICAL,
      cfg::PortProfileID::PROFILE_200G_4_PAM4_RS544X2N_COPPER,
      cfg::PortProfileID::PROFILE_200G_4_PAM4_RS544X2N_OPTICAL,
      cfg::PortProfileID::PROFILE_400G_8_PAM4_RS544X2N_OPTICAL};

  // Wedge400 has 16 uplinks + 4 * 32 downlink ports = 144
  // 32 TH3 Blackhawk cores + 48 transceivers
  setExpection(144, 32, 0, 48, expectedProfiles);

  auto mapping = std::make_unique<Wedge400PlatformMapping>();
  verify(mapping.get());
}

TEST_F(PlatformMappingTest, VerifyYampPlatformMapping) {
  // supported profiles
  std::vector<cfg::PortProfileID> expectedProfiles = {
      cfg::PortProfileID::PROFILE_40G_4_NRZ_NOFEC,
      cfg::PortProfileID::PROFILE_100G_4_NRZ_RS528,
      cfg::PortProfileID::PROFILE_200G_4_PAM4_RS544X2N};

  // Yamp has 128 ports
  // 32 TH3 Blackhawk cores + 128 transceivers + 64 xphy
  setExpection(128, 32, 64, 128, expectedProfiles);

  auto mapping = std::make_unique<YampPlatformMapping>();
  verify(mapping.get());
}

TEST_F(PlatformMappingTest, VerifyMinipack16QPlatformMapping) {
  // supported profiles
  std::vector<cfg::PortProfileID> expectedProfiles = {
      cfg::PortProfileID::PROFILE_40G_4_NRZ_NOFEC,
      cfg::PortProfileID::PROFILE_100G_4_NRZ_RS528};

  // Minipack16Q has 128 ports
  // 32 TH3 Blackhawk cores + 128 transceivers + 32 xphy
  setExpection(128, 32, 32, 128, expectedProfiles);

  auto mapping = std::make_unique<Minipack16QPimPlatformMapping>();
  verify(mapping.get());
}
} // namespace test
} // namespace fboss
} // namespace facebook
