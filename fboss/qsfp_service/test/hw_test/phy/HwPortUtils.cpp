/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/qsfp_service/test/hw_test/phy/HwPortUtils.h"

#include "fboss/agent/platforms/common/PlatformMapping.h"
#include "fboss/lib/phy/ExternalPhy.h"
#include "fboss/qsfp_service/test/hw_test/phy/HwPhyEnsemble.h"

#include <folly/logging/xlog.h>
#include <gtest/gtest.h>
#include <thrift/lib/cpp/util/EnumUtils.h>

namespace facebook::fboss::utility {

void verifyPhyPortConfig(
    PortID portID,
    cfg::PortProfileID profileID,
    const PlatformMapping* platformMapping,
    phy::ExternalPhy* xphy) {
  // Expected config from PlatformMapping
  const auto& expectedPinConfig = platformMapping->getPortXphyPinConfig(
      PlatformPortProfileConfigMatcher(profileID, portID));
  const auto& expectedProfileConfig = platformMapping->getPortProfileConfig(
      PlatformPortProfileConfigMatcher(profileID, portID));
  EXPECT_TRUE(expectedProfileConfig);

  // ExternalPhy needs to use lane list to get lane config
  std::vector<uint32_t> sysLanes;
  if (auto xphySys = expectedPinConfig.xphySys_ref()) {
    for (const auto& pinConfig : *xphySys) {
      sysLanes.push_back(pinConfig.get_id().get_lane());
    }
  }
  EXPECT_FALSE(sysLanes.empty());
  std::vector<uint32_t> lineLanes;
  if (auto xphyLine = expectedPinConfig.xphyLine_ref()) {
    for (const auto& pinConfig : *xphyLine) {
      lineLanes.push_back(pinConfig.get_id().get_lane());
    }
  }
  EXPECT_FALSE(lineLanes.empty());

  // Now fetch the config actually programmed to the xphy
  const auto& actualPortConfig = xphy->getConfigOnePort(sysLanes, lineLanes);

  // Check speed
  EXPECT_EQ(expectedProfileConfig->get_speed(), actualPortConfig.profile.speed);
  // Check ProfileSideConfig. Due to we couldn't fetch all the config yet.
  // Just check the attribures we can get
  auto checkProfileSideConfig = [&](const phy::ProfileSideConfig& expected,
                                    const phy::ProfileSideConfig& actual) {
    EXPECT_EQ(expected.get_numLanes(), actual.get_numLanes());
    EXPECT_EQ(expected.get_fec(), actual.get_fec());
  };
  CHECK(expectedProfileConfig->xphySystem_ref());
  checkProfileSideConfig(
      *expectedProfileConfig->xphySystem_ref(),
      actualPortConfig.profile.system);
  CHECK(expectedProfileConfig->xphyLine_ref());
  checkProfileSideConfig(
      *expectedProfileConfig->xphyLine_ref(), actualPortConfig.profile.line);

  // TODO(rajank) Add tx_settings check
}
} // namespace facebook::fboss::utility
