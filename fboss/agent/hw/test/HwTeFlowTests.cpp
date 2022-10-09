/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <folly/IPAddress.h>
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwLinkStateDependentTest.h"
#include "fboss/agent/hw/test/HwTeFlowTestUtils.h"
#include "fboss/agent/hw/test/HwTestTeFlowUtils.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/TeFlowEntry.h"
#include "fboss/agent/test/EcmpSetupHelper.h"

using namespace facebook::fboss::utility;

namespace facebook::fboss {

using facebook::network::toBinaryAddress;
using folly::IPAddress;
using folly::StringPiece;

namespace {
static folly::IPAddressV6 kAddr1{"100::"};
static std::string kNhopAddrA("1::1");
static std::string kNhopAddrB("2::2");
static std::string kIfName1("fboss2000");
static std::string kIfName2("fboss2001");
static TeCounterID kCounterID0("counter0");
static TeCounterID kCounterID1("counter1");
static TeCounterID kCounterID2("counter2");
static int kPrefixLength0(0);
static int kPrefixLength1(61);
static int kPrefixLength2(64);
} // namespace

class HwTeFlowTest : public HwLinkStateDependentTest {
 protected:
  void SetUp() override {
    FLAGS_enable_exact_match = true;
    HwLinkStateDependentTest::SetUp();
    ecmpHelper_ = std::make_unique<utility::EcmpSetupTargetedPorts6>(
        getProgrammedState(), RouterID(0));
  }

  cfg::SwitchConfig initialConfig() const override {
    std::vector<PortID> ports = {
        masterLogicalPortIds()[0],
        masterLogicalPortIds()[1],
    };
    auto cfg = utility::onePortPerInterfaceConfig(
        getHwSwitch(), std::move(ports), getAsic()->desiredLoopbackMode());
    return cfg;
  }

  void resolveNextHop(PortDescriptor port) {
    applyNewState(ecmpHelper_->resolveNextHops(
        getProgrammedState(),
        {
            port,
        }));
  }

  bool skipTest() {
    return !getPlatform()->getAsic()->isSupported(HwAsic::Feature::EXACT_MATCH);
  }

  std::unique_ptr<utility::EcmpSetupTargetedPorts6> ecmpHelper_;
};

TEST_F(HwTeFlowTest, VerifyTeFlowGroupEnable) {
  if (this->skipTest()) {
    return;
  }

  setExactMatchCfg(getHwSwitchEnsemble(), kPrefixLength1);

  auto verify = [&](int prefixLength) {
    EXPECT_TRUE(
        utility::validateTeFlowGroupEnabled(getHwSwitch(), prefixLength));
  };

  verify(kPrefixLength1);
}

TEST_F(HwTeFlowTest, validateAddDeleteTeFlow) {
  if (this->skipTest()) {
    return;
  }

  setExactMatchCfg(getHwSwitchEnsemble(), kPrefixLength1);

  this->resolveNextHop(PortDescriptor(masterLogicalPortIds()[0]));
  this->resolveNextHop(PortDescriptor(masterLogicalPortIds()[1]));
  auto flowEntry1 = makeFlowEntry(
      "100::", kNhopAddrA, kIfName1, masterLogicalPortIds()[0], kCounterID0);
  addFlowEntry(getHwSwitchEnsemble(), flowEntry1);

  EXPECT_EQ(utility::getNumTeFlowEntries(getHwSwitch()), 1);
  utility::checkSwHwTeFlowMatch(
      getHwSwitch(),
      getProgrammedState(),
      makeFlowKey("100::", masterLogicalPortIds()[0]));

  auto flowEntry2 = makeFlowEntry(
      "101::", kNhopAddrB, kIfName2, masterLogicalPortIds()[1], kCounterID1);
  addFlowEntry(getHwSwitchEnsemble(), flowEntry2);

  EXPECT_EQ(utility::getNumTeFlowEntries(getHwSwitch()), 2);
  utility::checkSwHwTeFlowMatch(
      getHwSwitch(),
      getProgrammedState(),
      makeFlowKey("101::", masterLogicalPortIds()[1]));

  // Modify nexthop
  modifyFlowEntry(
      getHwSwitchEnsemble(),
      "100::",
      masterLogicalPortIds()[0],
      kNhopAddrB,
      kIfName2,
      kCounterID0);
  EXPECT_EQ(utility::getNumTeFlowEntries(getHwSwitch()), 2);
  utility::checkSwHwTeFlowMatch(
      getHwSwitch(),
      getProgrammedState(),
      makeFlowKey("100::", masterLogicalPortIds()[0]));

  // Modify counter id
  modifyFlowEntry(
      getHwSwitchEnsemble(),
      "100::",
      masterLogicalPortIds()[0],
      kNhopAddrB,
      kIfName2,
      kCounterID2);
  utility::checkSwHwTeFlowMatch(
      getHwSwitch(),
      getProgrammedState(),
      makeFlowKey("100::", masterLogicalPortIds()[0]));

  deleteFlowEntry(getHwSwitchEnsemble(), flowEntry1);

  EXPECT_EQ(utility::getNumTeFlowEntries(getHwSwitch()), 1);
}

TEST_F(HwTeFlowTest, validateEnableDisableTeFlow) {
  if (this->skipTest()) {
    return;
  }

  setExactMatchCfg(getHwSwitchEnsemble(), kPrefixLength1);

  this->resolveNextHop(PortDescriptor(masterLogicalPortIds()[0]));
  this->resolveNextHop(PortDescriptor(masterLogicalPortIds()[1]));
  auto flowEntry1 = makeFlowEntry(
      "100::", kNhopAddrA, kIfName1, masterLogicalPortIds()[0], kCounterID0);
  addFlowEntry(getHwSwitchEnsemble(), flowEntry1);
  auto flowEntry2 = makeFlowEntry(
      "101::", kNhopAddrB, kIfName2, masterLogicalPortIds()[1], kCounterID1);
  addFlowEntry(getHwSwitchEnsemble(), flowEntry2);

  EXPECT_EQ(utility::getNumTeFlowEntries(getHwSwitch()), 2);
  utility::checkSwHwTeFlowMatch(
      getHwSwitch(),
      getProgrammedState(),
      makeFlowKey("100::", masterLogicalPortIds()[0]));
  utility::checkSwHwTeFlowMatch(
      getHwSwitch(),
      getProgrammedState(),
      makeFlowKey("101::", masterLogicalPortIds()[1]));

  // Disable entry 1
  auto newFlowEntry1 = makeFlowEntry(
      "100::", kNhopAddrA, kIfName1, masterLogicalPortIds()[0], kCounterID0);
  modifyFlowEntry(getHwSwitchEnsemble(), newFlowEntry1, false);
  utility::checkSwHwTeFlowMatch(
      getHwSwitch(),
      getProgrammedState(),
      makeFlowKey("100::", masterLogicalPortIds()[0]));

  // Disable entry 2
  auto newFlowEntry2 = makeFlowEntry(
      "101::", kNhopAddrB, kIfName2, masterLogicalPortIds()[1], kCounterID1);
  modifyFlowEntry(getHwSwitchEnsemble(), newFlowEntry2, false);
  utility::checkSwHwTeFlowMatch(
      getHwSwitch(),
      getProgrammedState(),
      makeFlowKey("101::", masterLogicalPortIds()[1]));

  // Modify nexthop, enable, verify and delete entry 1
  newFlowEntry1 = makeFlowEntry(
      "100::", kNhopAddrB, kIfName2, masterLogicalPortIds()[0], kCounterID0);
  modifyFlowEntry(getHwSwitchEnsemble(), newFlowEntry1, true);
  utility::checkSwHwTeFlowMatch(
      getHwSwitch(),
      getProgrammedState(),
      makeFlowKey("100::", masterLogicalPortIds()[0]));
  deleteFlowEntry(getHwSwitchEnsemble(), newFlowEntry1);
  EXPECT_EQ(utility::getNumTeFlowEntries(getHwSwitch()), 1);

  // Enable entry 2
  newFlowEntry2 = makeFlowEntry(
      "101::", kNhopAddrB, kIfName2, masterLogicalPortIds()[1], kCounterID1);
  modifyFlowEntry(getHwSwitchEnsemble(), newFlowEntry2, true);
  EXPECT_EQ(utility::getNumTeFlowEntries(getHwSwitch()), 1);
  utility::checkSwHwTeFlowMatch(
      getHwSwitch(),
      getProgrammedState(),
      makeFlowKey("101::", masterLogicalPortIds()[1]));
}

TEST_F(HwTeFlowTest, validateExactMatchTableConfigs) {
  if (this->skipTest()) {
    return;
  }

  auto setup = [&]() {
    setExactMatchCfg(getHwSwitchEnsemble(), kPrefixLength1);
    this->resolveNextHop(PortDescriptor(masterLogicalPortIds()[0]));
    this->resolveNextHop(PortDescriptor(masterLogicalPortIds()[1]));
  };

  auto addDeleteFlowEntries = [&](bool isAdd) {
    auto flowEntry1 = makeFlowEntry(
        "100::", kNhopAddrA, kIfName1, masterLogicalPortIds()[0], kCounterID0);
    auto flowEntry2 = makeFlowEntry(
        "101::", kNhopAddrB, kIfName2, masterLogicalPortIds()[1], kCounterID1);
    if (isAdd) {
      addFlowEntry(getHwSwitchEnsemble(), flowEntry1);
      addFlowEntry(getHwSwitchEnsemble(), flowEntry2);
    } else {
      deleteFlowEntry(getHwSwitchEnsemble(), flowEntry1);
      deleteFlowEntry(getHwSwitchEnsemble(), flowEntry2);
    }
  };

  auto verify = [&](int prefixLength) {
    EXPECT_TRUE(
        utility::validateTeFlowGroupEnabled(getHwSwitch(), prefixLength));
    EXPECT_EQ(utility::getNumTeFlowEntries(getHwSwitch()), 2);
    utility::checkSwHwTeFlowMatch(
        getHwSwitch(),
        getProgrammedState(),
        makeFlowKey("100::", masterLogicalPortIds()[0]));
    utility::checkSwHwTeFlowMatch(
        getHwSwitch(),
        getProgrammedState(),
        makeFlowKey("101::", masterLogicalPortIds()[1]));
  };

  setup();
  addDeleteFlowEntries(true);
  verify(kPrefixLength1);

  // TODO- verify the prefix length change without removing entries
  addDeleteFlowEntries(false);

  // Modify prefixLength
  setExactMatchCfg(getHwSwitchEnsemble(), kPrefixLength2);

  addDeleteFlowEntries(true);
  verify(kPrefixLength2);

  // Delete prefixLength config
  addDeleteFlowEntries(false);
  setExactMatchCfg(getHwSwitchEnsemble(), kPrefixLength0);
  EXPECT_TRUE(
      utility::validateTeFlowGroupEnabled(getHwSwitch(), kPrefixLength0));
}

} // namespace facebook::fboss
