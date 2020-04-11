/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/sai/api/SaiApiTable.h"
#include "fboss/agent/hw/sai/fake/FakeSai.h"
#include "fboss/agent/hw/sai/switch/SaiBridgeManager.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/tests/ManagerTestBase.h"
#include "fboss/agent/types.h"

#include <string>

#include <gtest/gtest.h>

using namespace facebook::fboss;

class BridgeManagerTest : public ManagerTestBase {};

TEST_F(BridgeManagerTest, addBridgePort) {
  auto bridgePort =
      saiManagerTable->bridgeManager().addBridgePort(PortID(42), PortSaiId(42));
  EXPECT_EQ(
      GET_ATTR(BridgePort, Type, bridgePort->attributes()),
      SAI_BRIDGE_PORT_TYPE_PORT);
  EXPECT_EQ(GET_ATTR(BridgePort, PortId, bridgePort->attributes()), 42);
  auto portId = saiApiTable->bridgeApi().getAttribute(
      bridgePort->adapterKey(), SaiBridgePortTraits::Attributes::PortId{});
  EXPECT_EQ(portId, 42);
}
