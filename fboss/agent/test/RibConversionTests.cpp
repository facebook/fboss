/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/StandaloneRibConversions.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/SwSwitchRouteUpdateWrapper.h"

#include "fboss/agent/hw/sim/SimPlatform.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/HwTestHandle.h"
#include "fboss/agent/test/RouteScaleGenerators.h"
#include "fboss/agent/test/TestUtils.h"

#include <folly/MacAddress.h>
#include <folly/dynamic.h>
#include <gtest/gtest.h>

using namespace facebook::fboss;

DECLARE_bool(enable_standalone_rib);

class RibConversionTest : public ::testing::Test {
 public:
  void SetUp() override {
    auto constexpr kEcmpWidth = 4;

    SimPlatform plat(folly::MacAddress(), 128);
    std::vector<PortID> ports;
    for (int i = 0; i < 128; ++i) {
      ports.push_back(PortID(i));
    }
    cfg::SwitchConfig config =
        utility::onePortPerVlanConfig(plat.getHwSwitch(), ports);
    utility::RouteDistributionGenerator::RouteChunks routeChunks;
    {
      // First SwSwitch with legacy rib. Scoped block to
      // destroy this SwSwitch on block exit
      auto testHandle = createTestHandle(&config);
      auto sw = testHandle->getSw();
      auto generator = utility::RouteDistributionGenerator(
          sw->getState(),
          {
              {64, 100},
          },
          {
              {24, 100},
          },
          sw->isStandaloneRibEnabled(),
          4000,
          2);
      routeChunks = generator.get();
      programRoutes(routeChunks, sw);
      legacyRibSwitchState_ = sw->getState();
      legacyRibGracefulExitJson_ = sw->gracefulExitState();
    }
    testHandleWithRib_ =
        createTestHandle(&config, SwitchFlags::ENABLE_STANDALONE_RIB);
    auto sw = testHandleWithRib_->getSw();
    programRoutes(routeChunks, sw);
  }

 protected:
  void assertOldRib(const HwInitResult& result) {
    // No new rib
    EXPECT_EQ(nullptr, result.rib);
    // No new fib
    EXPECT_EQ(0, result.switchState->getFibs()->size());
    EXPECT_EQ(
        legacyRibSwitchState_->getRouteTables()->toFollyDynamic(),
        result.switchState->getRouteTables()->toFollyDynamic());
  }
  void assertNewRib(const HwInitResult& result) {
    // Have new rib
    ASSERT_NE(nullptr, result.rib);
    // No old rib/fib
    EXPECT_EQ(0, result.switchState->getRouteTables()->size());
    const RouterID kRid0(0);
    // New rib as expected
    EXPECT_EQ(
        testHandleWithRib_->getSw()->getRib()->getRouteTableDetails(kRid0),
        result.rib->getRouteTableDetails(kRid0));
    // New fib as expected
    EXPECT_EQ(
        testHandleWithRib_->getSw()->getState()->getFibs()->toFollyDynamic(),
        result.switchState->getFibs()->toFollyDynamic());
  }
  std::shared_ptr<SwitchState> legacyRibSwitchState_;
  folly::dynamic legacyRibGracefulExitJson_;
  std::unique_ptr<HwTestHandle> testHandleWithRib_;
};

// Old rib -> old rib
TEST_F(RibConversionTest, fromLegacyRibToLegacyRib) {
  HwInitResult result{legacyRibSwitchState_, nullptr};
  FLAGS_enable_standalone_rib = false;
  handleStandaloneRIBTransition(legacyRibGracefulExitJson_, result);
  assertOldRib(result);
}

// New rib -> new rib
TEST_F(RibConversionTest, fromNewRibToNewRib) {
  HwInitResult result{testHandleWithRib_->getSw()->getState(), nullptr};
  FLAGS_enable_standalone_rib = true;
  handleStandaloneRIBTransition(
      testHandleWithRib_->getSw()->gracefulExitState(), result);
  assertNewRib(result);
}

// Old rib -> new rib
TEST_F(RibConversionTest, fromLegacyRibToNewRib) {
  HwInitResult result{legacyRibSwitchState_, nullptr};
  FLAGS_enable_standalone_rib = true;
  handleStandaloneRIBTransition(legacyRibGracefulExitJson_, result);
  assertNewRib(result);
}

// New rib -> old rib
TEST_F(RibConversionTest, fromNewRibToLegacyRib) {
  HwInitResult result{testHandleWithRib_->getSw()->getState(), nullptr};
  FLAGS_enable_standalone_rib = false;
  handleStandaloneRIBTransition(
      testHandleWithRib_->getSw()->gracefulExitState(), result);
  assertOldRib(result);
}
