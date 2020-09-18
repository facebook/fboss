/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/ApplyThriftConfig.h"
#include "fboss/agent/packet/Ethertype.h"
#include "fboss/agent/state/AggregatePort.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/PortDescriptor.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/ResourceLibUtil.h"
#include "fboss/agent/test/TrunkUtils.h"

#include "fboss/agent/MacTableUtils.h"
#include "fboss/agent/Platform.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/hw/test/HwLinkStateDependentTest.h"
#include "fboss/agent/hw/test/HwTestLearningUpdateObserver.h"
#include "fboss/agent/hw/test/HwTestMacUtils.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"

#include "fboss/agent/hw/test/ConfigFactory.h"

#include <folly/IPAddress.h>
#include <folly/Optional.h>

#include <memory>
#include <set>
#include <vector>

using facebook::fboss::L2EntryThrift;
using folly::IPAddress;
using folly::IPAddressV4;
using folly::IPAddressV6;

namespace {
// Even when running the same test repeatedly could result in different
// learning counts based on hash insertion  order.
// Maximum theortical is 8k for TH ..but practically we hit numbers below it
// Putting the value ot 7K should give enough buffer
int constexpr L2_LEARN_MAX_MAC_COUNT = 7000;
std::set<folly::MacAddress>
getMacsForPort(const facebook::fboss::HwSwitch* hw, int port, bool isTrunk) {
  std::set<folly::MacAddress> macs;
  std::vector<L2EntryThrift> l2Entries;
  hw->fetchL2Table(&l2Entries);
  for (auto& l2Entry : l2Entries) {
    if ((isTrunk && l2Entry.trunk_ref().value_or({}) == port) ||
        *l2Entry.port_ref() == port) {
      macs.insert(folly::MacAddress(*l2Entry.mac_ref()));
    }
  }
  return macs;
}
} // namespace

namespace facebook::fboss {

using utility::addAggPort;
using utility::enableTrunkPorts;

class HwMacLearningTest : public HwLinkStateDependentTest {
 protected:
  void SetUp() override {
    HwLinkStateDependentTest::SetUp();
    l2LearningObserver_.startObserving(getHwSwitchEnsemble());
  }

  void TearDown() override {
    l2LearningObserver_.stopObserving();
    HwLinkStateDependentTest::TearDown();
  }

  cfg::SwitchConfig initialConfig() const override {
    return utility::oneL3IntfTwoPortConfig(
        getHwSwitch(),
        masterLogicalPortIds()[0],
        masterLogicalPortIds()[1],
        cfg::PortLoopbackMode::MAC);
  }

  static MacAddress kSourceMac() {
    return MacAddress("02:00:00:00:00:05");
  }

  static MacAddress kSourceMac2() {
    return MacAddress("02:00:00:00:00:06");
  }

  void sendL2Pkts(
      int vlanId,
      std::optional<PortID> outPort,
      const std::vector<folly::MacAddress>& srcMacs,
      const std::vector<folly::MacAddress>& dstMacs) {
    auto originalStats =
        getHwSwitchEnsemble()->getLatestPortStats(masterLogicalPortIds());
    auto allSent = [&originalStats](const auto& newStats) {
      auto originalPkts = getPortInPkts(originalStats);
      auto newPkts = getPortInPkts(newStats);
      auto expectedPkts = originalPkts + L2_LEARN_MAX_MAC_COUNT;
      XLOGF(
          INFO,
          "Checking packets sent. Old: {}, New: {}, Expected: {}",
          originalPkts,
          newPkts,
          expectedPkts);
      return newPkts == expectedPkts;
    };
    for (auto srcMac : srcMacs) {
      for (auto dstMac : dstMacs) {
        auto txPacket = utility::makeEthTxPacket(
            getHwSwitch(),
            facebook::fboss::VlanID(vlanId),
            srcMac,
            dstMac,
            facebook::fboss::ETHERTYPE::ETHERTYPE_LLDP);
        if (outPort) {
          getHwSwitch()->sendPacketOutOfPortSync(
              std::move(txPacket), outPort.value());
        } else {
          getHwSwitch()->sendPacketSwitchedSync(std::move(txPacket));
        }
      }
    }
    getHwSwitchEnsemble()->waitPortStatsCondition(allSent);
  }

  void sendPkt() {
    auto txPacket = utility::makeEthTxPacket(
        getHwSwitch(),
        VlanID(*initialConfig().vlanPorts_ref()[0].vlanID_ref()),
        kSourceMac(),
        MacAddress::BROADCAST,
        ETHERTYPE::ETHERTYPE_LLDP);

    getHwSwitchEnsemble()->ensureSendPacketOutOfPort(
        std::move(txPacket), PortID(masterLogicalPortIds()[0]));
  }

  bool wasMacLearnt(PortDescriptor portDescr, bool shouldExist = true) {
    /***
     * shouldExist - if set to true (default), retry until mac is found.
     *             - if set to false, retry until mac is no longer learned
     * @return true if the desired condition occurs before timeout, else false
     */
    auto l2LearningMode =
        getProgrammedState()->getSwitchSettings()->getL2LearningMode();

    /*
     * For HwMacLearningTest.VerifyHwAgingForPort:
     *  - On SDK 6.5.16, the test PASS'ed across several (100+) iterations.
     *  - On SDK 6.5.17, the test fails intermittently as at times, the L2
     *    entry is aged out, albeit, with delay.
     *
     *  CSP CS10327477 reports this regression to Broadcom. In the meantime,
     *  we bump up the retries to 10 (for all tests using this util function,
     *  and all devices).
     */
    int retries = 10;
    while (retries--) {
      if ((l2LearningMode == cfg::L2LearningMode::SOFTWARE &&
           wasMacLearntInSwitchState(shouldExist) &&
           isInL2Table(shouldExist, portDescr)) ||
          (l2LearningMode == cfg::L2LearningMode::HARDWARE &&
           wasMacLearntInHw(shouldExist))) {
        return true;
      }

      // State udpate that will add/remove MacEntry happens asynchronously in
      // Event base. Give it chance to run.
      // Typically the MAC learning is immediate post a packet sent, but retries
      // help avoid test noise.
      sleep(1);
    }
    return false;
  }

  void verifyL2TableCallback(
      std::pair<L2Entry, L2EntryUpdateType> l2EntryAndUpdateType,
      PortDescriptor portDescr,
      L2EntryUpdateType expectedL2EntryUpdateType,
      L2Entry::L2EntryType expectedL2EntryType,
      folly::MacAddress expectedMac = kSourceMac()) {
    auto [l2Entry, l2EntryUpdateType] = l2EntryAndUpdateType;

    EXPECT_EQ(l2Entry.getMac(), expectedMac);
    EXPECT_EQ(
        l2Entry.getVlanID(),
        VlanID(*initialConfig().vlanPorts_ref()[0].vlanID_ref()));
    EXPECT_EQ(l2Entry.getPort(), portDescr);
    EXPECT_EQ(l2Entry.getType(), expectedL2EntryType);
    EXPECT_EQ(l2EntryUpdateType, expectedL2EntryUpdateType);
  }
  void setL2LearningMode(cfg::L2LearningMode l2LearningMode) {
    if (getProgrammedState()->getSwitchSettings()->getL2LearningMode() ==
        l2LearningMode) {
      return;
    }
    auto newState = getProgrammedState()->clone();
    auto newSwitchSettings = newState->getSwitchSettings()->clone();
    newSwitchSettings->setL2LearningMode(l2LearningMode);
    newState->resetSwitchSettings(newSwitchSettings);
    applyNewState(newState);
  }
  void setupHelper(
      cfg::L2LearningMode l2LearningMode,
      PortDescriptor portDescr) {
    auto newCfg{initialConfig()};
    *newCfg.switchSettings_ref()->l2LearningMode_ref() = l2LearningMode;

    if (portDescr.isAggregatePort()) {
      *newCfg.ports_ref()[0].state_ref() = cfg::PortState::ENABLED;
      addAggPort(
          std::numeric_limits<AggregatePortID>::max(),
          {masterLogicalPortIds()[0]},
          &newCfg);
      auto state = applyNewConfig(newCfg);
      applyNewState(enableTrunkPorts(state));
    } else {
      setL2LearningMode(l2LearningMode);
    }
    bringDownPort(masterLogicalPortIds()[1]);
  }

  PortDescriptor physPortDescr() const {
    return PortDescriptor(PortID(masterLogicalPortIds()[0]));
  }

  PortDescriptor aggPortDescr() const {
    return PortDescriptor(
        AggregatePortID(std::numeric_limits<AggregatePortID>::max()));
  }

  int kMinAgeInSecs() {
    return 1;
  }

  L2Entry::L2EntryType expectedL2EntryTypeOnAdd() const {
    /*
     * TD2 and TH learn the entry as PENDING, TH3 learns the entry as VALIDATED
     */
    return (getAsic()->getAsicType() != HwAsic::AsicType::ASIC_TYPE_TOMAHAWK3)
        ? L2Entry::L2EntryType::L2_ENTRY_TYPE_PENDING
        : L2Entry::L2EntryType::L2_ENTRY_TYPE_VALIDATED;
  }

  void testHwLearningHelper(PortDescriptor portDescr) {
    auto setup = [this, portDescr]() {
      setupHelper(cfg::L2LearningMode::HARDWARE, portDescr);
      // Disable aging, so entry stays in L2 table when we verify.
      utility::setMacAgeTimerSeconds(getHwSwitch(), 0);
      sendPkt();
    };

    auto verify = [this, portDescr]() { EXPECT_TRUE(wasMacLearnt(portDescr)); };

    // MACs learned should be preserved across warm boot
    verifyAcrossWarmBoots(setup, verify);
  }

  void testHwAgingHelper(PortDescriptor portDescr) {
    auto setup = [this, portDescr]() {
      setupHelper(cfg::L2LearningMode::HARDWARE, portDescr);
    };

    auto verify = [this, portDescr]() {
      // Disable aging, so entry stays in L2 table when we verify.
      utility::setMacAgeTimerSeconds(getHwSwitch(), 0);
      sendPkt();
      EXPECT_TRUE(wasMacLearnt(portDescr));

      // Force MAC aging to as fast a possible but min is still 1 second
      utility::setMacAgeTimerSeconds(getHwSwitch(), kMinAgeInSecs());
      EXPECT_TRUE(wasMacLearnt(portDescr, false /* MAC aged */));
    };

    verifyAcrossWarmBoots(setup, verify);
  }

  void testHwToSwLearningHelper(PortDescriptor portDescr) {
    auto setup = [this, portDescr]() {
      setupHelper(cfg::L2LearningMode::HARDWARE, portDescr);

      // Disable aging, so entry stays in L2 table when we verify.
      utility::setMacAgeTimerSeconds(getHwSwitch(), 0);
      sendPkt();
    };

    auto verify = [this, portDescr]() { EXPECT_TRUE(wasMacLearnt(portDescr)); };

    auto setupPostWarmboot = [this, portDescr]() {
      l2LearningObserver_.reset();
      setL2LearningMode(cfg::L2LearningMode::SOFTWARE);
    };

    auto verifyPostWarmboot = [this, portDescr]() {
      auto updates = l2LearningObserver_.waitForLearningUpdates(2);
      for (const auto& update : updates) {
        auto gotMac = update.first.getMac();
        EXPECT_TRUE(gotMac == kSourceMac() || gotMac == kSourceMac2());
        verifyL2TableCallback(
            update,
            portDescr,
            L2EntryUpdateType::L2_ENTRY_UPDATE_TYPE_ADD,
            L2Entry::L2EntryType::L2_ENTRY_TYPE_VALIDATED,
            gotMac);
      }
      EXPECT_TRUE(wasMacLearnt(portDescr));
    };

    verifyAcrossWarmBoots(setup, verify, setupPostWarmboot, verifyPostWarmboot);
  }

  void testSwToHwLearningHelper(PortDescriptor portDescr) {
    auto setup = [this, portDescr]() {
      setupHelper(cfg::L2LearningMode::SOFTWARE, portDescr);
      // Disable aging, so entry stays in L2 table when we verify.
      utility::setMacAgeTimerSeconds(getHwSwitch(), 0);

      l2LearningObserver_.reset();
      sendPkt();

      verifyL2TableCallback(
          l2LearningObserver_.waitForLearningUpdates().front(),
          portDescr,
          L2EntryUpdateType::L2_ENTRY_UPDATE_TYPE_ADD,
          expectedL2EntryTypeOnAdd());
    };

    auto verify = [this, portDescr]() { EXPECT_TRUE(wasMacLearnt(portDescr)); };

    auto setupPostWarmboot = [this, portDescr]() {
      setupHelper(cfg::L2LearningMode::HARDWARE, portDescr);
    };

    auto verifyPostWarmboot = [this, portDescr]() {
      /*
       * We only maintain MacTable in the SwitchState in SOFTWARE
       * l2LearningMode.
       *
       * Thus, when we transition from SOFTWRE l2LearningMode to HARDWARE
       * l2Learning:
       * - BCM layer traverses l2Table and calls deleteCb for every entry.
       * - The deleteCb processing removes l2 entries from the switch state.
       * - However, this causes subsequent state update to
       *   'processMacTableChanges' and remove L2 entries programmed in ASIC.
       *
       * If the traffic is flowing, the L2 entries would be immediately
       * relearned (by HARDWARE learning).
       *
       * We could modify processMacTableChanges to omit processing of updates
       * when l2LearningMode is HARDWARE. But, for cleaner design, we chose to
       * maintain the abstraction of HwSwitch just applying switch states
       * passed down to it.
       *
       * Thus, here we ASSERT that the MAC is removed.
       */
      EXPECT_TRUE(wasMacLearnt(portDescr, false /* MAC aged */));
    };

    verifyAcrossWarmBoots(setup, verify, setupPostWarmboot, verifyPostWarmboot);
  }

  VlanID kVlanID() {
    return VlanID(*initialConfig().vlanPorts_ref()[0].vlanID_ref());
  }
  HwTestLearningUpdateObserver l2LearningObserver_;

 private:
  bool wasMacLearntInHw(bool shouldExist) {
    bringUpPort(masterLogicalPortIds()[1]);
    auto origPortStats =
        getHwSwitchEnsemble()->getLatestPortStats(masterLogicalPortIds()[1]);
    auto txPacket = utility::makeEthTxPacket(
        getHwSwitch(),
        VlanID(*initialConfig().vlanPorts_ref()[0].vlanID_ref()),
        kSourceMac2(),
        kSourceMac(),
        ETHERTYPE::ETHERTYPE_LLDP);

    getHwSwitchEnsemble()->ensureSendPacketSwitched(std::move(txPacket));
    auto newPortStats =
        getHwSwitchEnsemble()->getLatestPortStats(masterLogicalPortIds()[1]);

    bringDownPort(masterLogicalPortIds()[1]);
    auto newPortBytes =
        *newPortStats.outBytes__ref() - *origPortStats.outBytes__ref();
    // If MAC should have been learnt then we should get no traffic on
    // masterLogicalPortIds[1], otherwise we should see flooding and
    // non zero bytes on masterLogicalPortIds[1]
    return (shouldExist == (newPortBytes == 0));
  }

  bool wasMacLearntInSwitchState(bool shouldExist) const {
    auto vlanID = VlanID(*initialConfig().vlanPorts_ref()[0].vlanID_ref());
    auto state = getProgrammedState();
    auto vlan = state->getVlans()->getVlanIf(vlanID);
    auto* macTable = vlan->getMacTable().get();
    return (shouldExist == (macTable->getNodeIf(kSourceMac()) != nullptr));
  }
  bool isInL2Table(bool shouldExist, const PortDescriptor& portDescr) const {
    auto isTrunk = portDescr.isAggregatePort();
    int portId = isTrunk ? portDescr.aggPortID() : portDescr.phyPortID();
    auto macs = getMacsForPort(getHwSwitch(), portId, isTrunk);
    return (shouldExist == (macs.find(kSourceMac()) != macs.end()));
  }
};

class HwMacSwLearningModeTest : public HwMacLearningTest {
 public:
  cfg::SwitchConfig initialConfig() const override {
    auto cfg = utility::oneL3IntfTwoPortConfig(
        getHwSwitch(),
        masterLogicalPortIds()[0],
        masterLogicalPortIds()[1],
        cfg::PortLoopbackMode::MAC);
    cfg.switchSettings_ref()->l2LearningMode_ref() =
        cfg::L2LearningMode::SOFTWARE;
    return cfg;
  }

 protected:
  void induceMacLearning(PortDescriptor portDescr) {
    l2LearningObserver_.reset();
    sendPkt();

    verifyL2TableCallback(
        l2LearningObserver_.waitForLearningUpdates().front(),
        portDescr,
        L2EntryUpdateType::L2_ENTRY_UPDATE_TYPE_ADD,
        expectedL2EntryTypeOnAdd());
  }
  void testSwLearningHelper(PortDescriptor portDescr) {
    auto setup = [this, portDescr]() {
      setupHelper(cfg::L2LearningMode::SOFTWARE, portDescr);
      // Disable aging, so entry stays in L2 table when we verify.
      utility::setMacAgeTimerSeconds(getHwSwitch(), 0);
      induceMacLearning(portDescr);
    };

    auto verify = [this, portDescr]() { EXPECT_TRUE(wasMacLearnt(portDescr)); };

    // MACs learned should be preserved across warm boot
    verifyAcrossWarmBoots(setup, verify);
  }

  void testSwAgingHelper(PortDescriptor portDescr) {
    auto setup = [this, portDescr]() {
      setupHelper(cfg::L2LearningMode::SOFTWARE, portDescr);
    };

    auto verify = [this, portDescr]() {
      // Disable aging, so entry stays in L2 table when we verify.
      utility::setMacAgeTimerSeconds(getHwSwitch(), 0);
      induceMacLearning(portDescr);

      EXPECT_TRUE(wasMacLearnt(portDescr));

      // Force MAC aging to as fast a possible but min is still 1 second
      l2LearningObserver_.reset();
      utility::setMacAgeTimerSeconds(getHwSwitch(), kMinAgeInSecs());

      // Verify if we get DELETE (aging) callback for VALIDATED entry
      verifyL2TableCallback(
          l2LearningObserver_.waitForLearningUpdates().front(),
          portDescr,
          L2EntryUpdateType::L2_ENTRY_UPDATE_TYPE_DELETE,
          L2Entry::L2EntryType::L2_ENTRY_TYPE_VALIDATED);
      EXPECT_TRUE(wasMacLearnt(portDescr, false /* MAC aged */));
    };

    verifyAcrossWarmBoots(setup, verify);
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

  cfg::AclLookupClass kClassID() {
    return cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_2;
  }
};

class HwMacLearningStaticEntriesTest : public HwMacLearningTest {
 protected:
  void addOrUpdateMacEntry(MacEntryType type) {
    auto newState = getProgrammedState()->clone();
    auto vlan = newState->getVlans()->getVlanIf(kVlanID()).get();
    auto macTable = vlan->getMacTable().get();
    macTable = macTable->modify(&vlan, &newState);
    if (macTable->getNodeIf(kSourceMac())) {
      macTable->updateEntry(kSourceMac(), physPortDescr(), std::nullopt, type);
    } else {
      auto macEntry = std::make_shared<MacEntry>(
          kSourceMac(), physPortDescr(), std::nullopt, type);
      macTable->addEntry(macEntry);
    }
    applyNewState(newState);
  };
};

TEST_F(HwMacLearningStaticEntriesTest, VerifyStaticMacEntryAdd) {
  auto setup = [this] {
    setupHelper(cfg::L2LearningMode::HARDWARE, physPortDescr());
    utility::setMacAgeTimerSeconds(getHwSwitch(), kMinAgeInSecs());
    addOrUpdateMacEntry(MacEntryType::STATIC_ENTRY);
  };
  auto verify = [this] {
    std::this_thread::sleep_for(std::chrono::seconds(2 * kMinAgeInSecs()));
    // Static entries shouldn't age
    EXPECT_TRUE(wasMacLearnt(physPortDescr()));
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwMacLearningStaticEntriesTest, VerifyStaticDynamicTransformations) {
  auto setup = [this] {
    setupHelper(cfg::L2LearningMode::HARDWARE, physPortDescr());
    utility::setMacAgeTimerSeconds(getHwSwitch(), kMinAgeInSecs());
    addOrUpdateMacEntry(MacEntryType::STATIC_ENTRY);
    addOrUpdateMacEntry(MacEntryType::DYNAMIC_ENTRY);
    std::this_thread::sleep_for(std::chrono::seconds(2 * kMinAgeInSecs()));
    // Dynamic entries should get aged out
    EXPECT_TRUE(wasMacLearnt(physPortDescr(), false /*aged in hw*/));
    // Transform back to STATIC entry (we still have entry in switch state).
    // Its important to test that we can update or add a static entry regardless
    // of the HW state. Since for dynamic entries, they might get aged out in
    // parallel to us transforming them to STATIC.
    addOrUpdateMacEntry(MacEntryType::STATIC_ENTRY);
  };
  auto verify = [this] {
    std::this_thread::sleep_for(std::chrono::seconds(2 * kMinAgeInSecs()));
    // Static entries shouldn't age
    EXPECT_TRUE(wasMacLearnt(physPortDescr()));
  };
  verifyAcrossWarmBoots(setup, verify);
}
// Intent of this test is to attempt to learn large number of macs
// (L2_LEARN_MAX_MAC_COUNT) and ensure HW can learn them.
TEST_F(HwMacLearningTest, VerifyMacLearningScale) {
  if (getAsic()->getAsicType() == HwAsic::AsicType::ASIC_TYPE_TOMAHAWK3) {
    // this test is not valid for TH3 as chip supports SW based learning only
    // which is much slower to learn for a scaled test. Also SW introduces
    // variability in results .
    XLOG(INFO) << "Skip the test for TH3 platform";
    return;
  }
  std::vector<folly::MacAddress> macs;
  auto generator = utility::MacAddressGenerator();
  // start with fixed address and increment deterministically
  // evaluate total learnt l2 entries
  generator.startOver(kSourceMac().u64HBO());
  for (auto i = 0; i < L2_LEARN_MAX_MAC_COUNT; ++i) {
    macs.emplace_back(generator.getNext());
  }

  auto portDescr = physPortDescr();
  auto setup = [this, portDescr, &macs]() {
    setupHelper(cfg::L2LearningMode::HARDWARE, portDescr);
    // Disable aging, so entry stays in L2 table when we verify.
    utility::setMacAgeTimerSeconds(getHwSwitch(), 0);
    sendL2Pkts(
        *initialConfig().vlanPorts_ref()[0].vlanID_ref(),
        masterLogicalPortIds()[0],
        macs,
        {folly::MacAddress::BROADCAST});
  };

  auto verify = [this, portDescr, &macs]() {
    bringUpPort(masterLogicalPortIds()[1]);
    auto origPortStats =
        getHwSwitchEnsemble()->getLatestPortStats(masterLogicalPortIds()[1]);
    // Send packets to macs which are expected to have been learned now
    sendL2Pkts(
        *initialConfig().vlanPorts_ref()[0].vlanID_ref(),
        std::nullopt,
        {kSourceMac()},
        macs);
    auto curPortStats =
        getHwSwitchEnsemble()->getLatestPortStats(masterLogicalPortIds()[1]);
    // All packets should have gone through masterLogicalPortIds()[0], port
    // on which macs were learnt and none through any other port in the same
    // VLAN. This lack of broadcast confirms MAC learning.
    EXPECT_EQ(*curPortStats.outBytes__ref(), *origPortStats.outBytes__ref());
    bringDownPort(masterLogicalPortIds()[1]);
  };

  // MACs learned should be preserved across warm boot
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwMacLearningTest, VerifyHwLearningForPort) {
  testHwLearningHelper(physPortDescr());
}

TEST_F(HwMacLearningTest, VerifyHwLearningForTrunk) {
  testHwLearningHelper(aggPortDescr());
}

TEST_F(HwMacLearningTest, VerifyHwAgingForPort) {
  testHwAgingHelper(physPortDescr());
}

TEST_F(HwMacLearningTest, VerifyHwAgingForTrunk) {
  testHwAgingHelper(aggPortDescr());
}

TEST_F(HwMacLearningTest, VerifyHwToSwLearningForPort) {
  testHwToSwLearningHelper(physPortDescr());
}

TEST_F(HwMacLearningTest, VerifySwToHwLearningForPort) {
  testSwToHwLearningHelper(physPortDescr());
}

TEST_F(HwMacSwLearningModeTest, VerifySwLearningForPort) {
  testSwLearningHelper(physPortDescr());
}

TEST_F(HwMacSwLearningModeTest, VerifySwLearningForTrunk) {
  testSwLearningHelper(aggPortDescr());
}

TEST_F(HwMacSwLearningModeTest, VerifySwAgingForPort) {
  testSwAgingHelper(physPortDescr());
}

TEST_F(HwMacSwLearningModeTest, VerifySwAgingForTrunk) {
  testSwAgingHelper(aggPortDescr());
}

TEST_F(HwMacSwLearningModeTest, VerifyNbrMacInL2Table) {
  if (!getHwSwitch()->needL2EntryForNeighbor()) {
    return;
  }
  auto setup = [this] {
    utility::EcmpSetupTargetedPorts6 ecmpHelper6{getProgrammedState(),
                                                 kSourceMac()};
    applyNewState(
        ecmpHelper6.resolveNextHops(getProgrammedState(), {physPortDescr()}));
  };
  auto verify = [this] { wasMacLearnt(physPortDescr()); };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwMacSwLearningModeTest, VerifyCallbacksOnMacEntryChange) {
  auto setup = [this]() { bringDownPort(masterLogicalPortIds()[1]); };
  auto verify = [this]() {
    bool isTH3 = getPlatform()->getAsic()->getAsicType() ==
        HwAsic::AsicType::ASIC_TYPE_TOMAHAWK3;
    // Disable aging, so entry stays in L2 table when we verify.
    utility::setMacAgeTimerSeconds(getHwSwitch(), 0);
    enum class MacOp { ASSOCIATE, DISSOASSOCIATE, DELETE };
    induceMacLearning(physPortDescr());
    auto doMacOp = [this, isTH3](MacOp op) {
      l2LearningObserver_.reset();
      auto numExpectedUpdates = 0;
      switch (op) {
        case MacOp::ASSOCIATE:
          XLOG(INFO) << " Adding classs id to mac";
          associateClassID();
          // TH3 generates a delete and add on class ID update
          numExpectedUpdates = isTH3 ? 2 : 0;
          break;
        case MacOp::DISSOASSOCIATE:
          XLOG(INFO) << " Removing classs id from mac";
          disassociateClassID();
          // TH3 generates a delete and add on class ID update
          numExpectedUpdates = isTH3 ? 2 : 0;
          break;
        case MacOp::DELETE:
          XLOG(INFO) << " Removing mac";
          // Force MAC aging to as fast a possible but min is still 1 second
          utility::setMacAgeTimerSeconds(getHwSwitch(), kMinAgeInSecs());

          // Verify if we get DELETE (aging) callback for VALIDATED entry
          verifyL2TableCallback(
              l2LearningObserver_.waitForLearningUpdates().front(),
              physPortDescr(),
              L2EntryUpdateType::L2_ENTRY_UPDATE_TYPE_DELETE,
              L2Entry::L2EntryType::L2_ENTRY_TYPE_VALIDATED);
          numExpectedUpdates = 1;
          break;
      }
      auto assertClassID =
          [this](std::optional<cfg::AclLookupClass> lookupClass) {
            auto macTable = getProgrammedState()
                                ->getVlans()
                                ->getVlanIf(kVlanID())
                                ->getMacTable();
            auto macEntry = macTable->getNodeIf(kSourceMac());
            EXPECT_EQ(macEntry->getClassID(), lookupClass);
          };
      // Wait for arbitrarily large (100) extra updates or 5 seconds
      // whichever is sooner
      auto updates = l2LearningObserver_.waitForLearningUpdates(
          numExpectedUpdates + 100, 5);
      EXPECT_EQ(updates.size(), numExpectedUpdates);
      switch (op) {
        case MacOp::ASSOCIATE:
          EXPECT_TRUE(wasMacLearnt(physPortDescr()));
          assertClassID(kClassID());
          break;
        case MacOp::DISSOASSOCIATE:
          EXPECT_TRUE(wasMacLearnt(physPortDescr()));
          assertClassID(std::nullopt);
          break;
        case MacOp::DELETE:
          EXPECT_TRUE(wasMacLearnt(physPortDescr(), false));
          break;
      }
    };
    doMacOp(MacOp::ASSOCIATE);
    doMacOp(MacOp::DISSOASSOCIATE);
    doMacOp(MacOp::DELETE);
  };
  verifyAcrossWarmBoots(setup, verify);
}

class HwMacLearningMacMoveTest : public HwMacLearningTest {
 protected:
  cfg::SwitchConfig initialConfig() const override {
    auto cfg = utility::oneL3IntfTwoPortConfig(
        getHwSwitch(),
        masterLogicalPortIds()[0],
        masterLogicalPortIds()[1],
        cfg::PortLoopbackMode::MAC);
    cfg.switchSettings_ref()->l2LearningMode_ref() =
        cfg::L2LearningMode::SOFTWARE;
    return cfg;
  }

  void sendPkt2() {
    auto txPacket = utility::makeEthTxPacket(
        getHwSwitch(),
        VlanID(*initialConfig().vlanPorts_ref()[0].vlanID_ref()),
        kSourceMac(),
        MacAddress::BROADCAST,
        ETHERTYPE::ETHERTYPE_LLDP);

    getHwSwitchEnsemble()->ensureSendPacketOutOfPort(
        std::move(txPacket), PortID(masterLogicalPortIds()[1]));
  }

  PortDescriptor physPortDescr2() const {
    return PortDescriptor(PortID(masterLogicalPortIds()[1]));
  }

  void testMacMoveHelper() {
    auto setup = []() {};

    auto verify = [this]() {
      auto portDescr = physPortDescr();
      auto portDescr2 = physPortDescr2();

      // One port up, other down
      bringUpPort(portDescr.phyPortID());
      bringDownPort(portDescr2.phyPortID());

      // Disable aging, so entry stays in L2 table when we verify.
      utility::setMacAgeTimerSeconds(getHwSwitch(), 0);

      XLOG(DBG2) << "Send pkt on up port, other port is down";
      l2LearningObserver_.reset();
      sendPkt();

      /*
       * Verify if we get ADD (learn) callback for PENDING entry for TD2, TH
       * and VALIDATED entry for TH3.
       */
      verifyL2TableCallback(
          l2LearningObserver_.waitForLearningUpdates().front(),
          portDescr,
          L2EntryUpdateType::L2_ENTRY_UPDATE_TYPE_ADD,
          expectedL2EntryTypeOnAdd());
      EXPECT_TRUE(wasMacLearnt(portDescr));

      // Bring up port down, and down port up
      bringDownPort(portDescr.phyPortID());
      bringUpPort(portDescr2.phyPortID());

      XLOG(DBG2)
          << "Trigger MAC Move: Bring up port down, down port up, and send pkt";
      l2LearningObserver_.reset();
      sendPkt2();

      // When MAC Moves from port1 to port2, we get DELETE on port1 and ADD on
      // port2
      if (getPlatform()->getAsic()->getAsicType() ==
          HwAsic::AsicType::ASIC_TYPE_TAJO) {
        // TODO: Remove this once TajoAsic properly generates a
        // MAC move event.
        verifyL2TableCallback(
            l2LearningObserver_.waitForLearningUpdates().front(),
            portDescr2,
            L2EntryUpdateType::L2_ENTRY_UPDATE_TYPE_ADD,
            expectedL2EntryTypeOnAdd());
      } else {
        auto l2EntryAndUpdateTypeList =
            l2LearningObserver_.waitForLearningUpdates(2);
        verifyL2TableCallback(
            l2EntryAndUpdateTypeList.at(0),
            portDescr,
            L2EntryUpdateType::L2_ENTRY_UPDATE_TYPE_DELETE,
            L2Entry::L2EntryType::L2_ENTRY_TYPE_VALIDATED);
        verifyL2TableCallback(
            l2EntryAndUpdateTypeList.at(1),
            portDescr2,
            L2EntryUpdateType::L2_ENTRY_UPDATE_TYPE_ADD,
            L2Entry::L2EntryType::L2_ENTRY_TYPE_VALIDATED);
      }

      EXPECT_TRUE(wasMacLearnt(portDescr2));

      // Aging out MAC prepares for subsequent run of verify()

      // Force MAC aging to as fast a possible but min is still 1 second
      l2LearningObserver_.reset();
      utility::setMacAgeTimerSeconds(getHwSwitch(), kMinAgeInSecs());

      // Verify if we get DELETE (aging) callback for VALIDATED entry
      verifyL2TableCallback(
          l2LearningObserver_.waitForLearningUpdates().front(),
          portDescr2,
          L2EntryUpdateType::L2_ENTRY_UPDATE_TYPE_DELETE,
          L2Entry::L2EntryType::L2_ENTRY_TYPE_VALIDATED);
      EXPECT_TRUE(wasMacLearnt(portDescr2, false /* MAC aged */));
    };

    // MAC Move should work as expected post warmboot as well.
    verifyAcrossWarmBoots(setup, verify);
  }
};

TEST_F(HwMacLearningMacMoveTest, VerifyMacMoveForPort) {
  testMacMoveHelper();
}

} // namespace facebook::fboss
