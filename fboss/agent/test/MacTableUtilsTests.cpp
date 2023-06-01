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
#include "fboss/agent/MacTableUtils.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/Vlan.h"
#include "fboss/agent/test/TestUtils.h"

#include <folly/MacAddress.h>

using folly::MacAddress;

namespace facebook::fboss {

class MacTableUtilsTest : public ::testing::Test {
 public:
  VlanID kVlan() const {
    return VlanID(1);
  }

  PortID kPortID() const {
    return PortID(1);
  }

  folly::MacAddress kMacAddress() const {
    return MacAddress("01:02:03:04:05:06");
  }

  L2Entry kL2Entry() {
    return L2Entry(
        kMacAddress(),
        kVlan(),
        PortDescriptor(kPortID()),
        L2Entry::L2EntryType::L2_ENTRY_TYPE_VALIDATED);
  }

  cfg::AclLookupClass kClassID() {
    return cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_2;
  }

  std::shared_ptr<SwitchState> verifyAddOrUpdateClassIDHelper(
      const std::shared_ptr<SwitchState>& state) {
    auto macEntry =
        std::make_shared<MacEntry>(kMacAddress(), PortDescriptor(kPortID()));
    auto newState = MacTableUtils::updateOrAddEntryWithClassID(
        state, kVlan(), macEntry, kClassID());

    auto newMacEntry = getMacEntry(newState);
    EXPECT_NE(newMacEntry, nullptr);
    auto classID = newMacEntry->getClassID();
    EXPECT_TRUE(classID.has_value());
    EXPECT_EQ(classID.value(), kClassID());

    return newState;
  }

  std::shared_ptr<SwitchState> verifyRemoveClassIDHelper(
      const std::shared_ptr<SwitchState>& state,
      bool macPresent) {
    auto macEntry =
        std::make_shared<MacEntry>(kMacAddress(), PortDescriptor(kPortID()));
    auto newState =
        MacTableUtils::removeClassIDForEntry(state, kVlan(), macEntry);

    auto newMacEntry = getMacEntry(newState);
    if (macPresent) {
      EXPECT_NE(newMacEntry, nullptr);
      EXPECT_FALSE(newMacEntry->getClassID().has_value());
    } else {
      EXPECT_EQ(newMacEntry, nullptr);
    }

    return newState;
  }

  std::shared_ptr<SwitchState> verifyAddMac(
      const std::shared_ptr<SwitchState>& state) {
    return verifyAddAndDeleteMacHelper(
        state, L2EntryUpdateType::L2_ENTRY_UPDATE_TYPE_ADD);
  }

  std::shared_ptr<SwitchState> verifyDeleteMac(
      const std::shared_ptr<SwitchState>& state) {
    return verifyAddAndDeleteMacHelper(
        state, L2EntryUpdateType::L2_ENTRY_UPDATE_TYPE_DELETE);
  }

 private:
  std::shared_ptr<SwitchState> verifyAddAndDeleteMacHelper(
      const std::shared_ptr<SwitchState>& state,
      L2EntryUpdateType l2EntryUpdateType) {
    auto newState =
        MacTableUtils::updateMacTable(state, kL2Entry(), l2EntryUpdateType);

    auto vlan = newState->getVlans()->getNodeIf(kVlan()).get();
    auto* macTable = vlan->getMacTable().get();
    auto node = macTable->getMacIf(kL2Entry().getMac());

    if (l2EntryUpdateType == L2EntryUpdateType::L2_ENTRY_UPDATE_TYPE_ADD) {
      EXPECT_NE(nullptr, node);
    } else {
      EXPECT_EQ(nullptr, node);
    }

    return newState;
  }

  std::shared_ptr<MacEntry> getMacEntry(
      const std::shared_ptr<SwitchState>& state) {
    auto vlan = state->getVlans()->getNodeIf(kVlan()).get();
    auto* macTable = vlan->getMacTable().get();
    return macTable->getMacIf(kMacAddress());
  }
};

TEST_F(MacTableUtilsTest, VerifyUpdateMacTableAddMac) {
  verifyAddMac(testStateA());
}

TEST_F(MacTableUtilsTest, VerifyUpdateMacTableAddMacTwice) {
  auto state = verifyAddMac(testStateA());
  verifyAddMac(state);
}

TEST_F(MacTableUtilsTest, VerifyUpdateMacTableDeleteMac) {
  auto state = verifyAddMac(testStateA());
  verifyDeleteMac(state);
}

TEST_F(MacTableUtilsTest, VerifyUpdateMacTableDeleteMacTwice) {
  auto state1 = verifyAddMac(testStateA());
  auto state2 = verifyDeleteMac(state1);
  verifyDeleteMac(state2);
}

TEST_F(MacTableUtilsTest, VerifyUpdateWithClassID) {
  auto state = verifyAddMac(testStateA());
  verifyAddOrUpdateClassIDHelper(state);
}

TEST_F(MacTableUtilsTest, VerifyAddWithClassID) {
  auto state = testStateA();
  verifyAddOrUpdateClassIDHelper(state);
}

TEST_F(MacTableUtilsTest, VerifyRemoveClassID) {
  auto state1 = testStateA();
  auto state2 = verifyAddOrUpdateClassIDHelper(state1);
  verifyRemoveClassIDHelper(state2, true /* macPresent */);
}

TEST_F(MacTableUtilsTest, VerifyRemoveClassIDForAbsentEntry) {
  auto state = testStateA();
  verifyRemoveClassIDHelper(state, false /* macPresent */);
}

} // namespace facebook::fboss
