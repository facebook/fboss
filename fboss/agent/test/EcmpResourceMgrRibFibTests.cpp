/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/EcmpResourceManager.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/state/Route.h"
#include "fboss/agent/state/RouteNextHopEntry.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/BaseEcmpResourceManagerTest.h"
#include "fboss/agent/test/HwTestHandle.h"
#include "fboss/agent/test/TestUtils.h"
#include "fboss/agent/types.h"

#include <folly/IPAddress.h>
#include <gtest/gtest.h>

namespace facebook::fboss {

class EcmpResourceManagerRibFibTest : public ::testing::Test {
 public:
  RouteNextHopSet defaultNhops() const {
    return makeNextHops(54);
  }
  using NextHopGroupId = EcmpResourceManager::NextHopGroupId;
  void SetUp() override {
    FLAGS_enable_ecmp_resource_manager = true;
    FLAGS_ecmp_resource_manager_make_before_break_buffer = 0;
    FLAGS_ecmp_resource_percentage = 100;
    auto cfg = onePortPerIntfConfig(10);
    handle_ = createTestHandle(&cfg);
    sw_ = handle_->getSw();
    ASSERT_NE(sw_->getEcmpResourceManager(), nullptr);
    // Taken from mock asic
    EXPECT_EQ(sw_->getEcmpResourceManager()->getMaxPrimaryEcmpGroups(), 4);
  }
  std::unique_ptr<HwTestHandle> handle_;
  SwSwitch* sw_;
};

TEST_F(EcmpResourceManagerRibFibTest, init) {}
} // namespace facebook::fboss
