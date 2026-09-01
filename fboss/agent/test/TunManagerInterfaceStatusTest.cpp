/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

/**
 * @def TUNMANAGER_INTERFACE_STATUS_FRIEND_TESTS
 * @brief Friend declarations granting these tests access to
 * TunManager::getInterfaceStatus, which is private.
 */
#define TUNMANAGER_INTERFACE_STATUS_FRIEND_TESTS                           \
  friend class TunManagerInterfaceStatusTest;                              \
  FRIEND_TEST(TunManagerInterfaceStatusTest, PortInterface);               \
  FRIEND_TEST(TunManagerInterfaceStatusTest, AggregateUpWhenMinLinksMet);  \
  FRIEND_TEST(                                                             \
      TunManagerInterfaceStatusTest, AggregateUpWithOneMemberForwarding);  \
  FRIEND_TEST(                                                             \
      TunManagerInterfaceStatusTest, AggregateDownWhenNoMemberForwarding); \
  FRIEND_TEST(TunManagerInterfaceStatusTest, AggregateDownWhenBelowMinLinks);

#include <gtest/gtest.h>

#include <algorithm>
#include <optional>

#include "fboss/agent/TunManager.h"
#include "fboss/agent/state/AggregatePort.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/HwTestHandle.h"
#include "fboss/agent/test/TestUtils.h"

namespace facebook::fboss {

class TunManagerInterfaceStatusTest : public ::testing::Test {
 public:
  void SetUp() override {
    auto config = testConfigAWithAggregatePortInterface();
    handle_ = createTestHandle(&config);
    sw_ = handle_->getSw();
    auto aggPort = sw_->getState()->getAggregatePorts()->getNode(
        AggregatePortID(kAggregatePortKey));
    for (const auto& subport : aggPort->sortedSubports()) {
      memberPorts_.push_back(subport.portID);
    }
  }

  // Puts the aggregate's members into the given forwarding states, leaving
  // every physical port up so that only the aggregate's own view of its
  // members can drive the interface status.
  std::shared_ptr<SwitchState> withForwarding(
      const std::vector<PortID>& forwardingPorts,
      std::optional<uint8_t> minimumLinkCount = std::nullopt) {
    auto newState = bringAllPortsUp(sw_->getState());
    auto aggPort = newState->getAggregatePorts()
                       ->getNode(AggregatePortID(kAggregatePortKey))
                       ->modify(&newState);
    for (auto portID : memberPorts_) {
      auto forwarding =
          std::find(forwardingPorts.begin(), forwardingPorts.end(), portID) !=
              forwardingPorts.end()
          ? AggregatePort::Forwarding::ENABLED
          : AggregatePort::Forwarding::DISABLED;
      aggPort->setForwardingState(portID, forwarding);
    }
    if (minimumLinkCount) {
      aggPort->setMinimumLinkCount(*minimumLinkCount);
    }
    return newState;
  }

  bool aggregateInterfaceUp(const std::shared_ptr<SwitchState>& state) {
    auto status = TunManager::getInterfaceStatus(state);
    auto it = status.find(InterfaceID(kAggregatePortInterfaceID));
    EXPECT_NE(it, status.end());
    return it != status.end() && it->second;
  }

  SwSwitch* sw_{nullptr};
  std::unique_ptr<HwTestHandle> handle_{nullptr};
  std::vector<PortID> memberPorts_;
};

TEST_F(TunManagerInterfaceStatusTest, PortInterface) {
  // An interface bound to a physical port still follows that one port.
  auto state = bringAllPortsUp(sw_->getState());
  auto status = TunManager::getInterfaceStatus(state);
  for (const auto& [_, portMap] : std::as_const(*state->getPorts())) {
    for (const auto& [_, port] : std::as_const(*portMap)) {
      auto intfID = InterfaceID(port->getInterfaceID());
      if (intfID == InterfaceID(kAggregatePortInterfaceID)) {
        continue;
      }
      auto it = status.find(intfID);
      ASSERT_NE(it, status.end());
      EXPECT_TRUE(it->second);
    }
  }
}

TEST_F(TunManagerInterfaceStatusTest, AggregateUpWhenMinLinksMet) {
  ASSERT_EQ(2, memberPorts_.size());

  // minimumCapacity defaults to ALL_LINKS, so every member has to forward.
  EXPECT_TRUE(aggregateInterfaceUp(withForwarding(memberPorts_)));
}

TEST_F(TunManagerInterfaceStatusTest, AggregateUpWithOneMemberForwarding) {
  ASSERT_EQ(2, memberPorts_.size());

  // With a minimum of one link, a single forwarding member is enough to keep
  // the aggregate up, whichever one it is.
  EXPECT_TRUE(aggregateInterfaceUp(withForwarding({memberPorts_[0]}, 1)));
  EXPECT_TRUE(aggregateInterfaceUp(withForwarding({memberPorts_[1]}, 1)));
}

TEST_F(TunManagerInterfaceStatusTest, AggregateDownWhenNoMemberForwarding) {
  EXPECT_FALSE(aggregateInterfaceUp(withForwarding({})));
}

TEST_F(TunManagerInterfaceStatusTest, AggregateDownWhenBelowMinLinks) {
  ASSERT_EQ(2, memberPorts_.size());

  // Both members forwarding satisfies a minimum of two...
  EXPECT_TRUE(aggregateInterfaceUp(withForwarding(memberPorts_, 2)));

  // ...but one does not, even though both member links are still up.
  EXPECT_FALSE(aggregateInterfaceUp(withForwarding({memberPorts_[0]}, 2)));
}

} // namespace facebook::fboss
