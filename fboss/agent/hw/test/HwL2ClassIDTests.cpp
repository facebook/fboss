/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/test/HwTest.h"

#include "fboss/agent/MacTableUtils.h"
#include "fboss/agent/hw/test/ConfigFactory.h"

namespace facebook::fboss {

class HwL2ClassIDTest : public HwTest {
 protected:
  cfg::SwitchConfig initialConfig() const {
    auto cfg =
        utility::oneL3IntfConfig(getHwSwitch(), masterLogicalPortIds()[0]);
    return cfg;
  }

  folly::MacAddress kSourceMac() {
    return folly::MacAddress("0:1:2:3:4:5");
  }

  PortDescriptor physPortDescr() const {
    return PortDescriptor(PortID(masterLogicalPortIds()[0]));
  }

  cfg::AclLookupClass kClassID() {
    return cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_2;
  }

  VlanID kVlanID() {
    return VlanID(*initialConfig().vlanPorts_ref()[0].vlanID_ref());
  }

  void associateClassID() {
    auto macEntry = std::make_shared<MacEntry>(kSourceMac(), physPortDescr());
    auto state = getProgrammedState();
    auto newState = MacTableUtils::updateOrAddEntryWithClassID(
        state, kVlanID(), macEntry, kClassID());
    applyNewState(newState);
  }

  void disassociateClassID() {
    auto macEntry = std::make_shared<MacEntry>(kSourceMac(), physPortDescr());
    auto state = getProgrammedState();
    auto newState =
        MacTableUtils::removeClassIDForEntry(state, kVlanID(), macEntry);
    applyNewState(newState);
  }

  bool l2EntryHasClassID(
      std::optional<cfg::AclLookupClass> classID = std::nullopt) {
    std::vector<L2EntryThrift> l2Entries;
    getHwSwitch()->fetchL2Table(&l2Entries);

    for (auto& l2Entry : l2Entries) {
      if (*l2Entry.mac_ref() == kSourceMac().toString() &&
          VlanID(*l2Entry.vlanID_ref()) == kVlanID()) {
        if (classID.has_value()) {
          return l2Entry.get_classID() &&
              (static_cast<int>(classID.value()) == (*l2Entry.get_classID()));
        } else {
          return !l2Entry.get_classID();
        }
      }
    }
    return false;
  }
};

TEST_F(HwL2ClassIDTest, VerifyClassID) {
  auto setup = [=]() {
    applyNewConfig(initialConfig());
    associateClassID();
    EXPECT_TRUE(l2EntryHasClassID(kClassID()));

    disassociateClassID();
    EXPECT_TRUE(l2EntryHasClassID());

    associateClassID();
  };

  auto verify = [=]() { EXPECT_TRUE(l2EntryHasClassID(kClassID())); };

  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
