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
#include "fboss/lib/phy/SaiPhyManager.h"

#include <gtest/gtest.h>

namespace facebook::fboss::utility {
void veridyPhyPortConnector(PortID portID, PhyManager* xphyManager) {
  auto saiPhyManager = static_cast<SaiPhyManager*>(xphyManager);
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
