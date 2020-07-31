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
#include "fboss/agent/hw/sai/switch/SaiFdbManager.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiPortManager.h"
#include "fboss/agent/hw/sai/switch/SaiVlanManager.h"
#include "fboss/agent/hw/sai/switch/tests/ManagerTestBase.h"
#include "fboss/agent/types.h"

#include <string>

#include <gtest/gtest.h>

using namespace facebook::fboss;

class FdbManagerTest : public ManagerTestBase {
 public:
  void SetUp() override {
    setupStage = SetupStage::PORT | SetupStage::VLAN | SetupStage::INTERFACE;
    ManagerTestBase::SetUp();
    intf0 = testInterfaces[1];
  }

  void checkFdbEntry(
      const InterfaceID& intfId,
      const folly::MacAddress& mac,
      PortID portId,
      sai_uint32_t metadata) {
    auto vlanId = VlanID(intfId);
    SaiFdbTraits::FdbEntry entry{1, vlanId, mac};
    auto portHandle = saiManagerTable->portManager().getPortHandle(portId);
    auto expectedBridgePortId = portHandle->bridgePort->adapterKey();
    auto bridgePortId = saiApiTable->fdbApi().getAttribute(
        entry, SaiFdbTraits::Attributes::BridgePortId{});
    EXPECT_EQ(bridgePortId, expectedBridgePortId);
    EXPECT_EQ(
        saiApiTable->fdbApi().getAttribute(
            entry, SaiFdbTraits::Attributes::Metadata{}),
        metadata);
  }

  TestInterface intf0;
};

TEST_F(FdbManagerTest, addFdbEntry) {
  folly::MacAddress mac1{"00:11:11:11:11:11"};
  InterfaceID intfId = InterfaceID(intf0.id);
  auto portId = PortID(intf0.id);
  saiManagerTable->fdbManager().addFdbEntry(
      portId, intfId, mac1, SAI_FDB_ENTRY_TYPE_STATIC, 42);
  checkFdbEntry(intfId, mac1, portId, 42);
}
