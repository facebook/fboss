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

#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/L2Entry.h"
#include "fboss/agent/ResourceAccountant.h"
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
    FLAGS_enable_hw_update_protection = true;
  }

  void verifyStateUpdate(Func func) {
    runInUpdateEventBaseAndWait(std::move(func));
  }

  void TearDown() override {
    schedulePendingTestStateUpdates();
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

  void triggerMacLearnedCb(bool wait = true) {
    triggerMacCbHelper(
        facebook::fboss::L2EntryUpdateType::L2_ENTRY_UPDATE_TYPE_ADD, wait);
  }

  void triggerMacAgedCb(bool wait = true) {
    triggerMacCbHelper(
        facebook::fboss::L2EntryUpdateType::L2_ENTRY_UPDATE_TYPE_DELETE, wait);
  }

  void triggerMacBulkLearnedCb(
      const std::vector<folly::MacAddress>& macs,
      bool wait = true) {
    triggerMacBulkCbHelper(
        macs,
        facebook::fboss::L2EntryUpdateType::L2_ENTRY_UPDATE_TYPE_ADD,
        wait);
  }

  void triggerMacBulkAgedCb(
      const std::vector<folly::MacAddress>& macs,
      bool wait = true) {
    triggerMacBulkCbHelper(
        macs,
        facebook::fboss::L2EntryUpdateType::L2_ENTRY_UPDATE_TYPE_DELETE,
        wait);
  }

  void verifyMacIsAdded() {
    verifyStateUpdate([=]() {
      auto vlan = sw_->getState()->getVlans()->getNode(kVlan());
      auto* macTable = vlan->getMacTable().get();
      auto node = macTable->getMacIf(kMacAddress());

      EXPECT_NE(nullptr, node);
      EXPECT_EQ(kMacAddress(), node->getMac());
      EXPECT_EQ(kPortID(), node->getPort().phyPortID());
    });
  }

  // verify mac entris in switchState is in sync with resourceAccountant
  // l2Entries_
  void verifyMacEntryCount(int l2ResourceAccountantEntries) {
    verifyStateUpdate([=, this]() {
      auto vlan = sw_->getState()->getVlans()->getNode(kVlan());
      auto* macTable = vlan->getMacTable().get();
      XLOG(DBG2) << "macTable size: " << macTable->size()
                 << ", l2ResourceAccountantEntries: "
                 << l2ResourceAccountantEntries
                 << ", getMacTableUpdateFailure: "
                 << sw_->stats()->getMacTableUpdateFailure();

      EXPECT_EQ(l2ResourceAccountantEntries, macTable->size());
    });
  }

  void verifyMacIsDeleted() {
    verifyStateUpdate([=]() {
      auto vlan = sw_->getState()->getVlans()->getNode(kVlan());
      auto* macTable = vlan->getMacTable().get();
      auto node = macTable->getMacIf(kMacAddress());

      EXPECT_EQ(nullptr, node);
    });
  }

  SwSwitch* getSw() {
    return sw_;
  }

 private:
  void runInUpdateEventBaseAndWait(Func func) {
    auto* evb = sw_->getUpdateEvb();
    evb->runInFbossEventBaseThreadAndWait(std::move(func));
  }

  void schedulePendingTestStateUpdates() {
    runInUpdateEventBaseAndWait([]() {});
  }

  void triggerMacCbHelper(
      L2EntryUpdateType l2EntryUpdateType,
      bool wait = true) {
    auto l2Entry = L2Entry(
        kMacAddress(),
        kVlan(),
        PortDescriptor(kPortID()),
        L2Entry::L2EntryType::L2_ENTRY_TYPE_PENDING);

    sw_->l2LearningUpdateReceived(l2Entry, l2EntryUpdateType);

    if (wait) {
      waitForBackgroundThread(sw_);
      waitForStateUpdates(sw_);
    }
  }

  void triggerMacBulkCbHelper(
      const std::vector<folly::MacAddress>& macs,
      L2EntryUpdateType l2EntryUpdateType,
      bool wait = true) {
    for (const auto& mac : macs) {
      auto l2Entry = L2Entry(
          mac,
          kVlan(),
          PortDescriptor(kPortID()),
          L2Entry::L2EntryType::L2_ENTRY_TYPE_PENDING);
      sw_->l2LearningUpdateReceived(l2Entry, l2EntryUpdateType);
    }
    if (wait) {
      waitForBackgroundThread(sw_);
      waitForStateUpdates(sw_);
    }
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

TEST_F(MacTableManagerTest, MacAgedLearnedCb) {
  WaitForMacEntryAddedOrDeleted macAdded1(
      getSw(), kMacAddress(), kVlan(), true);
  triggerMacLearnedCb(true);
  EXPECT_TRUE(macAdded1.wait());

  // Mac age and learn callback should cause
  // entry remove and add in switch state,
  // these state updates should not be coalesced.
  // It is because SW mac learning always need
  // to re-program learnt L2 entry so as to move
  // it out of pending state.
  WaitForMacEntryAddedOrDeleted macDeleted(
      getSw(), kMacAddress(), kVlan(), false);
  WaitForMacEntryAddedOrDeleted macAdded2(
      getSw(), kMacAddress(), kVlan(), true);
  triggerMacAgedCb(false);
  triggerMacLearnedCb(true);
  EXPECT_TRUE(macDeleted.wait());
  EXPECT_TRUE(macAdded2.wait());
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
// This test is to verify resourceAccountant is updated correctly
TEST_F(MacTableManagerTest, MacLearnedBulkCb) {
  int16_t numMac = 100;
  int16_t deleted = 0;
  int16_t batchSize = 10;
  std::vector<folly::MacAddress> macs;
  for (int16_t i = 0; i < numMac; i++) {
    macs.push_back(MacAddress::fromHBO(i));
  }

  // learn 10 mac entries
  triggerMacBulkLearnedCb(
      std::vector<folly::MacAddress>(macs.begin(), macs.begin() + batchSize));
  // expect 10 mac entries in switchState and resourceAccountant
  verifyMacEntryCount(getSw()->getResourceAccountant()->l2Entries_);

  // age 10 mac entries
  triggerMacBulkAgedCb(
      std::vector<folly::MacAddress>(macs.begin(), macs.begin() + batchSize));
  deleted = batchSize;
  // expect 0 mac entries in switchState and resourceAccountant
  verifyMacEntryCount(getSw()->getResourceAccountant()->l2Entries_);

  // learn 90 mac entries
  triggerMacBulkLearnedCb(
      std::vector<folly::MacAddress>(macs.begin() + batchSize, macs.end()));
  // expect 90 mac entries in switchState and resourceAccountant
  verifyMacEntryCount(getSw()->getResourceAccountant()->l2Entries_);

  // age 20 mac entries
  batchSize = 20;
  triggerMacBulkAgedCb(std::vector<folly::MacAddress>(
      macs.begin() + deleted, macs.begin() + deleted + batchSize));
  deleted += batchSize;
  // expect 70 mac entries in switchState and resourceAccountant
  verifyMacEntryCount(getSw()->getResourceAccountant()->l2Entries_);
}

} // namespace facebook::fboss
