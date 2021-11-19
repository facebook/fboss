// Copyright 2021-present Facebook. All Rights Reserved.

#include "fboss/qsfp_service/test/TransceiverManagerTest.h"

#include "fboss/qsfp_service/platforms/wedge/tests/MockWedgeManager.h"

#include <folly/Singleton.h>
#include <gtest/gtest.h>

using namespace facebook::fboss;

namespace facebook {
namespace fboss {

void TransceiverManagerTest::SetUp() {
  // Reset any global state being tracked in singletons
  // Each test then sets up its own state as needed.
  folly::SingletonVault::singleton()->destroyInstances();
  folly::SingletonVault::singleton()->reenableInstances();
  transceiverManager_ =
      std::make_unique<MockWedgeManager>(numModules, numPortsPerModule);
}

TEST_F(TransceiverManagerTest, getPortNameToModuleMap) {
  EXPECT_EQ(transceiverManager_->getPortNameToModuleMap().at("eth1/1/1"), 0);
  EXPECT_EQ(transceiverManager_->getPortNameToModuleMap().at("eth1/1/2"), 0);
  EXPECT_EQ(transceiverManager_->getPortNameToModuleMap().at("eth1/2/1"), 1);
  EXPECT_THROW(
      transceiverManager_->getPortNameToModuleMap().at("no_such_port"),
      std::out_of_range);
}

} // namespace fboss
} // namespace facebook
