/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/sai/switch/SaiLagManager.h"
#include "fboss/agent/hw/sai/switch/tests/ManagerTestBase.h"
#include "fboss/agent/state/AggregatePort.h"
#include "fboss/agent/types.h"

#include <string>

using namespace facebook::fboss;

class LagManagerTest : public ManagerTestBase {
 public:
  void SetUp() override {
    setupStage = SetupStage::PORT | SetupStage::VLAN;
    ManagerTestBase::SetUp();
    intf0 = testInterfaces[0];
  }
  TestInterface intf0;
};

TEST_F(LagManagerTest, addLag) {
  std::shared_ptr<AggregatePort> swAggregatePort = makeAggregatePort(intf0);
  LagSaiId saiId = saiManagerTable->lagManager().addLag(swAggregatePort);
  auto label = saiApiTable->lagApi().getAttribute(
      saiId, SaiLagTraits::Attributes::Label{});
  std::string value(label.data());
  EXPECT_EQ("lag0", value);
}
