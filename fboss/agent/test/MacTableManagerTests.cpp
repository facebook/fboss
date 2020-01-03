/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <gtest/gtest.h>

#include "fboss/agent/L2Entry.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/Vlan.h"
#include "fboss/agent/test/HwTestHandle.h"
#include "fboss/agent/test/TestUtils.h"

#include <folly/MacAddress.h>

namespace facebook::fboss {

class MacTableManagerTest : public ::testing::Test {
 public:
  using Func = folly::Function<void()>;
  using StateUpdateFn = SwSwitch::StateUpdateFn;

  void SetUp() override {
    handle_ = createTestHandle(testStateA());
    sw_ = handle_->getSw();
  }

  void verifyStateUpdate(Func func) {
    runInUpdateEventBaseAndWait(std::move(func));
  }

  void TearDown() override {
    schedulePendingTestStateUpdates();
  }

  void updateState(folly::StringPiece name, StateUpdateFn func) {
    sw_->updateStateBlocking(name, func);
  }

  VlanID kVlan() const {
    return VlanID(1);
  }

  PortID kPortID() const {
    return PortID(1);
  }

  folly::MacAddress kMacAddress() const {
    return MacAddress("01:02:03:04:05:06");
  }

  void triggerMacLearnedCb() {
    triggerMacCbHelper(
        facebook::fboss::L2EntryUpdateType::L2_ENTRY_UPDATE_TYPE_ADD);
  }

  void triggerMacAgedCb() {
    triggerMacCbHelper(
        facebook::fboss::L2EntryUpdateType::L2_ENTRY_UPDATE_TYPE_DELETE);
  }

  void verifyMacIsAdded() {
    verifyStateUpdate([=]() {
      auto vlan = sw_->getState()->getVlans()->getVlan(kVlan());
      auto* macTable = vlan->getMacTable().get();
      auto node = macTable->getNodeIf(kMacAddress());

      EXPECT_NE(nullptr, node);
      EXPECT_EQ(kMacAddress(), node->getMac());
      EXPECT_EQ(kPortID(), node->getPort().phyPortID());
    });
  }

  void verifyMacIsDeleted() {
    verifyStateUpdate([=]() {
      auto vlan = sw_->getState()->getVlans()->getVlan(kVlan());
      auto* macTable = vlan->getMacTable().get();
      auto node = macTable->getNodeIf(kMacAddress());

      EXPECT_EQ(nullptr, node);
    });
  }

 private:
  void runInUpdateEventBaseAndWait(Func func) {
    auto* evb = sw_->getUpdateEvb();
    evb->runInEventBaseThreadAndWait(std::move(func));
  }

  void schedulePendingTestStateUpdates() {
    runInUpdateEventBaseAndWait([]() {});
  }

  void triggerMacCbHelper(L2EntryUpdateType l2EntryUpdateType) {
    auto l2Entry = L2Entry(
        kMacAddress(),
        kVlan(),
        PortDescriptor(kPortID()),
        L2Entry::L2EntryType::L2_ENTRY_TYPE_PENDING);

    sw_->l2LearningUpdateReceived(l2Entry, l2EntryUpdateType);

    waitForBackgroundThread(sw_);
    waitForStateUpdates(sw_);
  }

  std::unique_ptr<HwTestHandle> handle_;
  SwSwitch* sw_;
};

TEST_F(MacTableManagerTest, MacLearnedCb) {
  triggerMacLearnedCb();

  verifyMacIsAdded();
}

TEST_F(MacTableManagerTest, SameMacLearnedCbTwice) {
  triggerMacLearnedCb();
  triggerMacLearnedCb();

  verifyMacIsAdded();
}

TEST_F(MacTableManagerTest, MacAgedCb) {
  triggerMacLearnedCb();
  triggerMacAgedCb();

  verifyMacIsDeleted();
}

TEST_F(MacTableManagerTest, SameMacAgedCbTwice) {
  triggerMacLearnedCb();
  triggerMacAgedCb();
  triggerMacAgedCb();

  verifyMacIsDeleted();
}

} // namespace facebook::fboss
