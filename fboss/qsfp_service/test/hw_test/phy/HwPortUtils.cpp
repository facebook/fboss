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

#include "fboss/agent/hw/sai/api/PortApi.h"
#include "fboss/agent/hw/sai/api/SaiApiTable.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiPortManager.h"
#include "fboss/agent/hw/sai/switch/SaiSwitch.h"
#include "fboss/agent/platforms/common/PlatformMapping.h"
#include "fboss/lib/phy/ExternalPhy.h"
#include "fboss/lib/phy/SaiPhyManager.h"
#include "fboss/qsfp_service/test/hw_test/HwQsfpEnsemble.h"

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

void verifyPhyPortConnector(PortID portID, HwQsfpEnsemble* qsfpEnsemble) {
  if (qsfpEnsemble->getWedgeManager()->getPlatformMode() !=
      PlatformMode::ELBERT) {
    return;
  }

  auto saiPhyManager =
      static_cast<SaiPhyManager*>(qsfpEnsemble->getPhyManager());
  // The goal here is to check whether:
  // 1) the connector is using the correct system port and line port.
  // 2) both system and line port admin state is enabled
  // Expected config from PlatformMapping
  auto* saiPortHandle = saiPhyManager->getSaiSwitch(portID)
                            ->managerTable()
                            ->portManager()
                            .getPortHandle(portID);
  auto connector = saiPortHandle->connector;
  auto& portApi = SaiApiTable::getInstance()->portApi();
  auto linePortID = portApi.getAttribute(
      connector->adapterKey(),
      SaiPortConnectorTraits::Attributes::LineSidePortId{});
  EXPECT_EQ(linePortID, saiPortHandle->port->adapterKey());

  auto systemPortID = portApi.getAttribute(
      connector->adapterKey(),
      SaiPortConnectorTraits::Attributes::SystemSidePortId{});
  EXPECT_EQ(systemPortID, saiPortHandle->sysPort->adapterKey());

  auto adminState = portApi.getAttribute(
      saiPortHandle->port->adapterKey(),
      SaiPortTraits::Attributes::AdminState{});
  EXPECT_TRUE(adminState);

  adminState = portApi.getAttribute(
      saiPortHandle->sysPort->adapterKey(),
      SaiPortTraits::Attributes::AdminState{});
  EXPECT_TRUE(adminState);
}
} // namespace facebook::fboss::utility
