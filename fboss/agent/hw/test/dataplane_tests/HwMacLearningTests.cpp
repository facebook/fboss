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
#include "fboss/agent/state/Interface.h"
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
#include "fboss/agent/hw/test/HwSwitchEnsembleRouteUpdateWrapper.h"
#include "fboss/agent/hw/test/HwTestLearningUpdateObserver.h"
#include "fboss/agent/hw/test/HwTestMacUtils.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"

#include "fboss/agent/hw/test/ConfigFactory.h"

#include <folly/IPAddress.h>
#include <folly/Optional.h>

#include <memory>
#include <set>
#include <vector>

using facebook::fboss::HwAsic;
using facebook::fboss::L2EntryThrift;
using folly::IPAddress;
using folly::IPAddressV4;
using folly::IPAddressV6;

namespace {
// Even when running the same test repeatedly could result in different
// learning counts based on hash insertion  order.
// Maximum theortical is 8k for TH ..but practically we hit numbers below it
// Putting the value ot 5K should give enough buffer
// TODO(joseph5wu) Due to T83358080, we found out TH3 takes way longer to learn
// MAC compared to other platforms and if the scale is too big, the tests will
// be very flaky. For now, change 7K to 5K while waiting for Broadcom to help
// us debug this issue.
int constexpr L2_LEARN_MAX_MAC_COUNT = 5000;

std::set<folly::MacAddress>
getMacsForPort(const facebook::fboss::HwSwitch* hw, int port, bool isTrunk) {
  std::set<folly::MacAddress> macs;
  std::vector<L2EntryThrift> l2Entries;
  hw->fetchL2Table(&l2Entries);
  for (auto& l2Entry : l2Entries) {
    if ((isTrunk && l2Entry.trunk().value_or({}) == port) ||
        *l2Entry.port() == port) {
      macs.insert(folly::MacAddress(*l2Entry.mac()));
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
        getAsic()->desiredLoopbackMode());
  }

  static MacAddress kSourceMac() {
    return MacAddress("02:00:00:00:00:05");
  }

  static MacAddress kSourceMac2() {
    return MacAddress("02:00:00:00:00:06");
  }

  void sendPkt(MacAddress srcMac = kSourceMac()) {
    auto txPacket = utility::makeEthTxPacket(
        getHwSwitch(),
        VlanID(*initialConfig().vlanPorts()[0].vlanID()),
        srcMac,
        MacAddress::BROADCAST,
        ETHERTYPE::ETHERTYPE_LLDP);

    getHwSwitchEnsemble()->ensureSendPacketOutOfPort(
        std::move(txPacket), PortID(masterLogicalPortIds()[0]));
  }

  bool wasMacLearnt(
      PortDescriptor portDescr,
      folly::MacAddress mac,
      bool shouldExist = true) {
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
           wasMacLearntInSwitchState(shouldExist, mac) &&
           isInL2Table(portDescr, mac, shouldExist)) ||
          (l2LearningMode == cfg::L2LearningMode::HARDWARE &&
           wasMacLearntInHw(shouldExist, mac))) {
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
        l2Entry.getVlanID(), VlanID(*initialConfig().vlanPorts()[0].vlanID()));
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
    *newCfg.switchSettings()->l2LearningMode() = l2LearningMode;

    if (portDescr.isAggregatePort()) {
      utility::findCfgPort(newCfg, masterLogicalPortIds()[0])->state() =
          cfg::PortState::ENABLED;
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
    return (getAsic()->isSupported(HwAsic::Feature::PENDING_L2_ENTRY))
        ? L2Entry::L2EntryType::L2_ENTRY_TYPE_PENDING
        : L2Entry::L2EntryType::L2_ENTRY_TYPE_VALIDATED;
  }

  void testHwLearningHelper(PortDescriptor portDescr) {
    auto setup = [this, portDescr]() {
      setupHelper(cfg::L2LearningMode::HARDWARE, portDescr);
      // Disable aging, so entry stays in L2 table when we verify.
      utility::setMacAgeTimerSeconds(getHwSwitchEnsemble(), 0);
      sendPkt();
    };

    auto verify = [this, portDescr]() {
      EXPECT_TRUE(wasMacLearnt(portDescr, kSourceMac()));
    };

    // MACs learned should be preserved across warm boot
    verifyAcrossWarmBoots(setup, verify);
  }

  void testHwAgingHelper(PortDescriptor portDescr) {
    auto setup = [this, portDescr]() {
      setupHelper(cfg::L2LearningMode::HARDWARE, portDescr);
    };

    auto verify = [this, portDescr]() {
      // Disable aging, so entry stays in L2 table when we verify.
      utility::setMacAgeTimerSeconds(getHwSwitchEnsemble(), 0);
      sendPkt();
      EXPECT_TRUE(wasMacLearnt(portDescr, kSourceMac()));

      // Force MAC aging to as fast as possible but min is still 1 second
      utility::setMacAgeTimerSeconds(getHwSwitchEnsemble(), kMinAgeInSecs());
      EXPECT_TRUE(wasMacLearnt(portDescr, kSourceMac(), false /* MAC aged */));
    };

    verifyAcrossWarmBoots(setup, verify);
  }

  void testHwToSwLearningHelper(PortDescriptor portDescr) {
    auto setup = [this, portDescr]() {
      setupHelper(cfg::L2LearningMode::HARDWARE, portDescr);

      // Disable aging, so entry stays in L2 table when we verify.
      utility::setMacAgeTimerSeconds(getHwSwitchEnsemble(), 0);
      sendPkt();
    };

    auto verify = [this, portDescr]() {
      EXPECT_TRUE(wasMacLearnt(portDescr, kSourceMac()));
    };

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
      EXPECT_TRUE(wasMacLearnt(portDescr, kSourceMac()));
    };

    verifyAcrossWarmBoots(setup, verify, setupPostWarmboot, verifyPostWarmboot);
  }

  void testSwToHwLearningHelper(PortDescriptor portDescr) {
    auto setup = [this, portDescr]() {
      setupHelper(cfg::L2LearningMode::SOFTWARE, portDescr);
      // Disable aging, so entry stays in L2 table when we verify.
      utility::setMacAgeTimerSeconds(getHwSwitchEnsemble(), 0);

      l2LearningObserver_.reset();
      sendPkt();

      verifyL2TableCallback(
          l2LearningObserver_.waitForLearningUpdates().front(),
          portDescr,
          L2EntryUpdateType::L2_ENTRY_UPDATE_TYPE_ADD,
          expectedL2EntryTypeOnAdd());
    };

    auto verify = [this, portDescr]() {
      EXPECT_TRUE(wasMacLearnt(portDescr, kSourceMac()));
    };

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
       */
      EXPECT_TRUE(wasMacLearnt(portDescr, kSourceMac()));
    };

    verifyAcrossWarmBoots(setup, verify, setupPostWarmboot, verifyPostWarmboot);
  }

  VlanID kVlanID() {
    return VlanID(*initialConfig().vlanPorts()[0].vlanID());
  }
  HwTestLearningUpdateObserver l2LearningObserver_;

 private:
  bool wasMacLearntInHw(bool shouldExist, MacAddress mac) {
    bringUpPort(masterLogicalPortIds()[1]);
    auto origPortStats =
        getHwSwitchEnsemble()->getLatestPortStats(masterLogicalPortIds()[1]);
    auto txPacket = utility::makeEthTxPacket(
        getHwSwitch(),
        VlanID(*initialConfig().vlanPorts()[0].vlanID()),
        kSourceMac2(),
        mac,
        ETHERTYPE::ETHERTYPE_LLDP);

    getHwSwitchEnsemble()->ensureSendPacketSwitched(std::move(txPacket));
    auto newPortStats =
        getHwSwitchEnsemble()->getLatestPortStats(masterLogicalPortIds()[1]);

    bringDownPort(masterLogicalPortIds()[1]);
    auto newPortBytes = *newPortStats.outBytes_() - *origPortStats.outBytes_();
    // If MAC should have been learnt then we should get no traffic on
    // masterLogicalPortIds[1], otherwise we should see flooding and
    // non zero bytes on masterLogicalPortIds[1]
    return (shouldExist == (newPortBytes == 0));
  }

  bool wasMacLearntInSwitchState(bool shouldExist, MacAddress mac) const {
    auto vlanID = VlanID(*initialConfig().vlanPorts()[0].vlanID());
    auto state = getProgrammedState();
    auto vlan = state->getVlans()->getVlanIf(vlanID);
    auto* macTable = vlan->getMacTable().get();
    return (shouldExist == (macTable->getMacIf(mac) != nullptr));
  }
  bool isInL2Table(
      const PortDescriptor& portDescr,
      folly::MacAddress mac,
      bool shouldExist) const {
    auto isTrunk = portDescr.isAggregatePort();
    int portId = isTrunk ? portDescr.aggPortID() : portDescr.phyPortID();
    auto macs = getMacsForPort(getHwSwitch(), portId, isTrunk);
    return (shouldExist == (macs.find(mac) != macs.end()));
  }
};

class HwMacSwLearningModeTest : public HwMacLearningTest {
 public:
  cfg::SwitchConfig initialConfig() const override {
    auto cfg = utility::oneL3IntfTwoPortConfig(
        getHwSwitch(),
        masterLogicalPortIds()[0],
        masterLogicalPortIds()[1],
        getAsic()->desiredLoopbackMode());
    cfg.switchSettings()->l2LearningMode() = cfg::L2LearningMode::SOFTWARE;
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
      utility::setMacAgeTimerSeconds(getHwSwitchEnsemble(), 0);
      induceMacLearning(portDescr);
    };

    auto verify = [this, portDescr]() {
      EXPECT_TRUE(wasMacLearnt(portDescr, kSourceMac()));
    };

    // MACs learned should be preserved across warm boot
    verifyAcrossWarmBoots(setup, verify);
  }

  // After the initial sw learning, expect no more l2 udpate as aging is
  // disabled.
  void testSwLearningNoCycleHelper(PortDescriptor portDescr) {
    auto setup = [this, portDescr]() {
      setupHelper(cfg::L2LearningMode::SOFTWARE, portDescr);
      // Disable aging, so entry stays in L2 table when we verify.
      utility::setMacAgeTimerSeconds(getHwSwitchEnsemble(), 0);
      induceMacLearning(portDescr);
    };

    // Verify no more l2 callbacks after the inital one
    auto verify = [this, portDescr]() {
      // Wait for 3 updates up to 15 seconds.
      EXPECT_LE(l2LearningObserver_.waitForLearningUpdates(3, 15).size(), 1);
    };

    // MACs learned should be preserved across warm boot
    verifyAcrossWarmBoots(setup, verify);
  }

  void testSwAgingHelper(PortDescriptor portDescr) {
    auto setup = [this, portDescr]() {
      setupHelper(cfg::L2LearningMode::SOFTWARE, portDescr);
    };

    auto verify = [this, portDescr]() {
      // Disable aging, so entry stays in L2 table when we verify.
      utility::setMacAgeTimerSeconds(getHwSwitchEnsemble(), 0);
      induceMacLearning(portDescr);

      EXPECT_TRUE(wasMacLearnt(portDescr, kSourceMac()));

      // Force MAC aging to as fast as possible but min is still 1 second
      l2LearningObserver_.reset();
      utility::setMacAgeTimerSeconds(getHwSwitchEnsemble(), kMinAgeInSecs());

      // Verify if we get DELETE (aging) callback for VALIDATED entry
      verifyL2TableCallback(
          l2LearningObserver_.waitForLearningUpdates().front(),
          portDescr,
          L2EntryUpdateType::L2_ENTRY_UPDATE_TYPE_DELETE,
          L2Entry::L2EntryType::L2_ENTRY_TYPE_VALIDATED);
      EXPECT_TRUE(wasMacLearnt(portDescr, kSourceMac(), false /* MAC aged */));
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
    if (macTable->getMacIf(kSourceMac())) {
      macTable->updateEntry(kSourceMac(), physPortDescr(), std::nullopt, type);
    } else {
      auto macEntry = std::make_shared<MacEntry>(
          kSourceMac(),
          physPortDescr(),
          std::optional<cfg::AclLookupClass>(std::nullopt),
          type);
      macTable->addEntry(macEntry);
    }
    applyNewState(newState);
  };
};

TEST_F(HwMacLearningStaticEntriesTest, VerifyStaticMacEntryAdd) {
  auto setup = [this] {
    setupHelper(cfg::L2LearningMode::HARDWARE, physPortDescr());
    utility::setMacAgeTimerSeconds(getHwSwitchEnsemble(), kMinAgeInSecs());
    addOrUpdateMacEntry(MacEntryType::STATIC_ENTRY);
  };
  auto verify = [this] {
    std::this_thread::sleep_for(std::chrono::seconds(2 * kMinAgeInSecs()));
    // Static entries shouldn't age
    EXPECT_TRUE(wasMacLearnt(physPortDescr(), kSourceMac()));
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwMacLearningStaticEntriesTest, VerifyStaticDynamicTransformations) {
  auto setup = [this] {
    setupHelper(cfg::L2LearningMode::HARDWARE, physPortDescr());
    utility::setMacAgeTimerSeconds(getHwSwitchEnsemble(), kMinAgeInSecs());
    addOrUpdateMacEntry(MacEntryType::STATIC_ENTRY);
    addOrUpdateMacEntry(MacEntryType::DYNAMIC_ENTRY);
    std::this_thread::sleep_for(std::chrono::seconds(2 * kMinAgeInSecs()));
    // Dynamic entries should get aged out
    EXPECT_TRUE(
        wasMacLearnt(physPortDescr(), kSourceMac(), false /*aged in hw*/));
    // Transform back to STATIC entry (we still have entry in switch state).
    // Its important to test that we can update or add a static entry regardless
    // of the HW state. Since for dynamic entries, they might get aged out in
    // parallel to us transforming them to STATIC.
    addOrUpdateMacEntry(MacEntryType::STATIC_ENTRY);
  };
  auto verify = [this] {
    std::this_thread::sleep_for(std::chrono::seconds(2 * kMinAgeInSecs()));
    // Static entries shouldn't age
    EXPECT_TRUE(wasMacLearnt(physPortDescr(), kSourceMac()));
  };
  verifyAcrossWarmBoots(setup, verify);
}

class HwMacLearningAndMyStationInteractionTest : public HwMacLearningTest {
 public:
  cfg::SwitchConfig initialConfig() const override {
    return utility::onePortPerInterfaceConfig(
        getHwSwitch(), masterLogicalPortIds());
  }
  /*
   * Tests added in response to S234435. We hit a vendor SDK bug,
   * whereby when router MACs aged out on a particular VLAN, the
   * MAC got pruned  from MY STATION table in addition to the L2
   * table. This then broke routing on ports belonging to such a VLAN
   * The test below induces MAC learning and aging of router MAC
   * on ALL vlans, ingresses a packet on one such port, and asserts
   * that the packet was actually routed correctly.
   */
  void testMyStationInteractionHelper(cfg::L2LearningMode mode) {
    auto setup = [this, mode] {
      setL2LearningMode(mode);
      utility::EcmpSetupTargetedPorts6 ecmp6(getProgrammedState());
      applyNewState(ecmp6.resolveNextHops(
          getProgrammedState(), {PortDescriptor(masterLogicalPortIds()[0])}));
      ecmp6.programRoutes(
          getRouteUpdater(), {PortDescriptor(masterLogicalPortIds()[0])});
    };
    auto verify = [this]() {
      if (getHwSwitch()->getBootType() == BootType::WARM_BOOT) {
        // Let mac age out learned during prior run
        utility::setMacAgeTimerSeconds(getHwSwitchEnsemble(), kMinAgeInSecs());
        // @lint-ignore CLANGTIDY
        sleep(kMinAgeInSecs() * 3);
      }
      utility::setMacAgeTimerSeconds(getHwSwitchEnsemble(), 0);
      auto induceMacLearning = [this]() {
        // Send gratuitous ARPs so intf/router MAC is learnt on all VLANs
        // and ports
        for (const auto& port : masterLogicalPortIds()) {
          l2LearningObserver_.reset();
          auto vlanID = getProgrammedState()
                            ->getPorts()
                            ->getNodeIf(port)
                            ->getVlans()
                            .begin()
                            ->first;
          auto intf =
              getProgrammedState()->getInterfaces()->getInterfaceInVlan(vlanID);
          for (auto iter : std::as_const(*intf->getAddresses())) {
            auto addrEntry = folly::IPAddress(iter.first);
            if (!addrEntry.isV4()) {
              continue;
            }
            auto v4Addr = addrEntry.asV4();
            l2LearningObserver_.reset();
            auto arpPacket = utility::makeARPTxPacket(
                getHwSwitch(),
                intf->getVlanID(),
                intf->getMac(),
                folly::MacAddress::BROADCAST,
                v4Addr,
                v4Addr,
                ARP_OPER::ARP_OPER_REQUEST,
                folly::MacAddress::BROADCAST);
            getHwSwitch()->sendPacketOutOfPortSync(std::move(arpPacket), port);
            l2LearningObserver_.waitForLearningUpdates(1, 1);
          }
        }
        l2LearningObserver_.waitForStateUpdate();
      };
      induceMacLearning();
      utility::setMacAgeTimerSeconds(getHwSwitchEnsemble(), kMinAgeInSecs());
      // Let mac age out
      sleep(kMinAgeInSecs() * 3);
      // Re learn MAC so FDB entries are learnt and the packet
      // below is not sacrificed to a MAC learning callback event.
      // Secondly its important to WB with L2 entries learnt for
      // the bug seen in S234435 to repro.
      utility::setMacAgeTimerSeconds(getHwSwitchEnsemble(), 0);
      induceMacLearning();
      auto portStatsBefore =
          getHwSwitchEnsemble()->getLatestPortStats(masterLogicalPortIds());
      auto sendRoutedPkt = [this]() {
        auto vlanId = VlanID(*initialConfig().vlanPorts()[0].vlanID());
        auto intfMac = utility::getInterfaceMac(getProgrammedState(), vlanId);
        auto txPacket = utility::makeIpTxPacket(
            getHwSwitch(),
            vlanId,
            intfMac,
            intfMac,
            folly::IPAddress("1000::1"),
            folly::IPAddress("2000::1"));
        getHwSwitchEnsemble()->ensureSendPacketOutOfPort(
            std::move(txPacket), masterLogicalPortIds()[1]);
      };
      sendRoutedPkt();
      EXPECT_TRUE(getHwSwitchEnsemble()->waitPortStatsCondition(
          [this, &portStatsBefore, &sendRoutedPkt](
              const std::map<PortID, HwPortStats>& portStatsAfter) {
            auto oldPortStats = portStatsBefore[masterLogicalPortIds()[0]];
            auto newPortStats =
                portStatsAfter.find(masterLogicalPortIds()[0])->second;
            auto oldOutBytes = *oldPortStats.outBytes_();
            auto newOutBytes = *newPortStats.outBytes_();
            XLOG(DBG2) << " Port Id: " << masterLogicalPortIds()[0]
                       << " Old out bytes: " << oldOutBytes
                       << " New out bytes: " << newOutBytes
                       << " Delta : " << (newOutBytes - oldOutBytes);
            if ((newOutBytes - oldOutBytes) > 0) {
              return true;
            }
            /*
             * Resend routed packet, with SW learning, we can get
             * the first packet lost to MAC learning as the src mac
             * may still be in PENDING state on some platforms. Resent
             * here, we are looking for persisting my station breakage
             * and not a single packet racing with mac learning
             */
            sendRoutedPkt();
            return false;
          }));
    };
    verifyAcrossWarmBoots(setup, verify);
  }
};

TEST_F(
    HwMacLearningAndMyStationInteractionTest,
    verifyInteractionHwMacLearning) {
  testMyStationInteractionHelper(cfg::L2LearningMode::HARDWARE);
}
TEST_F(
    HwMacLearningAndMyStationInteractionTest,
    verifyInteractionSwMacLearning) {
  testMyStationInteractionHelper(cfg::L2LearningMode::SOFTWARE);
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

TEST_F(HwMacSwLearningModeTest, VerifySwLearningForPortNoCycle) {
  testSwLearningNoCycleHelper(physPortDescr());
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
    utility::EcmpSetupTargetedPorts6 ecmpHelper6{
        getProgrammedState(), kSourceMac()};
    applyNewState(
        ecmpHelper6.resolveNextHops(getProgrammedState(), {physPortDescr()}));
  };
  auto verify = [this] { wasMacLearnt(physPortDescr(), kSourceMac()); };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwMacSwLearningModeTest, VerifyCallbacksOnMacEntryChange) {
  auto setup = [this]() { bringDownPort(masterLogicalPortIds()[1]); };
  auto verify = [this]() {
    bool isTH3 = getPlatform()->getAsic()->getAsicType() ==
        cfg::AsicType::ASIC_TYPE_TOMAHAWK3;
    // Disable aging, so entry stays in L2 table when we verify.
    utility::setMacAgeTimerSeconds(getHwSwitchEnsemble(), 0);
    enum class MacOp { ASSOCIATE, DISSOASSOCIATE, DELETE };
    induceMacLearning(physPortDescr());
    auto doMacOp = [this, isTH3](MacOp op) {
      l2LearningObserver_.reset();
      auto numExpectedUpdates = 0;
      switch (op) {
        case MacOp::ASSOCIATE:
          XLOG(DBG2) << " Adding classs id to mac";
          associateClassID();
          // TH3 generates a delete and add on class ID update
          numExpectedUpdates = isTH3 ? 2 : 0;
          break;
        case MacOp::DISSOASSOCIATE:
          XLOG(DBG2) << " Removing classs id from mac";
          disassociateClassID();
          // TH3 generates a delete and add on class ID update
          numExpectedUpdates = isTH3 ? 2 : 0;
          break;
        case MacOp::DELETE:
          XLOG(DBG2) << " Removing mac";
          // Force MAC aging to as fast as possible but min is still 1 second
          utility::setMacAgeTimerSeconds(
              getHwSwitchEnsemble(), kMinAgeInSecs());

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
            auto macEntry = macTable->getMacIf(kSourceMac());
            EXPECT_EQ(macEntry->getClassID(), lookupClass);
          };
      // Wait for arbitrarily large (100) extra updates or 5 seconds
      // whichever is sooner
      auto updates = l2LearningObserver_.waitForLearningUpdates(
          numExpectedUpdates + 100, 5);
      EXPECT_EQ(updates.size(), numExpectedUpdates);
      switch (op) {
        case MacOp::ASSOCIATE:
          EXPECT_TRUE(wasMacLearnt(physPortDescr(), kSourceMac()));
          assertClassID(kClassID());
          break;
        case MacOp::DISSOASSOCIATE:
          EXPECT_TRUE(wasMacLearnt(physPortDescr(), kSourceMac()));
          assertClassID(std::nullopt);
          break;
        case MacOp::DELETE:
          EXPECT_TRUE(wasMacLearnt(physPortDescr(), kSourceMac(), false));
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
        getAsic()->desiredLoopbackMode());
    cfg.switchSettings()->l2LearningMode() = cfg::L2LearningMode::SOFTWARE;
    return cfg;
  }

  void sendPkt2() {
    auto txPacket = utility::makeEthTxPacket(
        getHwSwitch(),
        VlanID(*initialConfig().vlanPorts()[0].vlanID()),
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
      utility::setMacAgeTimerSeconds(getHwSwitchEnsemble(), 0);

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
      EXPECT_TRUE(wasMacLearnt(portDescr, kSourceMac()));

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
          cfg::AsicType::ASIC_TYPE_EBRO) {
        // TODO: Remove this once EbroAsic properly generates a
        // MAC move event.
        verifyL2TableCallback(
            l2LearningObserver_.waitForLearningUpdates().front(),
            portDescr2,
            L2EntryUpdateType::L2_ENTRY_UPDATE_TYPE_ADD,
            expectedL2EntryTypeOnAdd());
      } else if (getAsic()->isSupported(HwAsic::Feature::DETAILED_L2_UPDATE)) {
        verifyL2TableCallback(
            l2LearningObserver_.waitForLearningUpdates().front(),
            portDescr2,
            L2EntryUpdateType::L2_ENTRY_UPDATE_TYPE_ADD,
            L2Entry::L2EntryType::L2_ENTRY_TYPE_VALIDATED);
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

      EXPECT_TRUE(wasMacLearnt(portDescr2, kSourceMac()));

      // Aging out MAC prepares for subsequent run of verify()

      // Force MAC aging to as fast as possible but min is still 1 second
      l2LearningObserver_.reset();
      utility::setMacAgeTimerSeconds(getHwSwitchEnsemble(), kMinAgeInSecs());

      // Verify if we get DELETE (aging) callback for VALIDATED entry
      verifyL2TableCallback(
          l2LearningObserver_.waitForLearningUpdates().front(),
          portDescr2,
          L2EntryUpdateType::L2_ENTRY_UPDATE_TYPE_DELETE,
          L2Entry::L2EntryType::L2_ENTRY_TYPE_VALIDATED);
      EXPECT_TRUE(wasMacLearnt(portDescr2, kSourceMac(), false /* MAC aged */));
    };

    // MAC Move should work as expected post warmboot as well.
    verifyAcrossWarmBoots(setup, verify);
  }
};

TEST_F(HwMacLearningMacMoveTest, VerifyMacMoveForPort) {
  testMacMoveHelper();
}

class HwMacLearningBatchEntriesTest : public HwMacLearningTest {
 protected:
  std::pair<int, int> getMacChunkSizeAndSleepUsecs() {
    // Some platforms only support SDK Software Learning, which is usually
    // slower
    // than Hardware Learning to let SDK learn MAC Address from hardware.
    // For these platforms, they will use cache mechanism to let hardware put
    // new MAC addresses into a cache, and then SDK will either read the cache
    // periodically or once the cache is full.
    // Therefore, instead of adding sleep time for sending each L2 packet, we
    // send a chunck of packets consecutively without sleep, which will shorten
    // the whole sleep time for this test.
    // However, we also need to make sure we don't flood the cache with a big
    // chunck size, which will drop those MAC addresses which can't be put in
    // the cache because cache is full. TH3 cache size is 16 so we use 16 as
    // trunk size to maximize the speed. NOTE: There's an on-going investigation
    // why TH3 needs significant longer sleep time than TH4. (T83358080)
    static const std::unordered_map<cfg::AsicType, std::pair<int, int>>
        kAsicToMacChunkSizeAndSleepUsecs = {
            {cfg::AsicType::ASIC_TYPE_TOMAHAWK3, {16, 500000}},
            {cfg::AsicType::ASIC_TYPE_TOMAHAWK4, {32, 10000}},
        };
    if (auto p =
            kAsicToMacChunkSizeAndSleepUsecs.find(getAsic()->getAsicType());
        p != kAsicToMacChunkSizeAndSleepUsecs.end()) {
      return p->second;
    } else {
      return {1 /* No need chunk */, 0 /* No sleep time */};
    }
  }

  std::vector<folly::MacAddress> generateMacs(int num) {
    std::vector<folly::MacAddress> macs;
    auto generator = utility::MacAddressGenerator();
    // start with fixed address and increment deterministically
    // evaluate total learnt l2 entries
    generator.startOver(kSourceMac().u64HBO());
    for (auto i = 0; i < num; ++i) {
      macs.emplace_back(generator.getNext());
    }
    return macs;
  }

  void sendL2Pkts(
      int vlanId,
      std::optional<PortID> outPort,
      const std::vector<folly::MacAddress>& srcMacs,
      const std::vector<folly::MacAddress>& dstMacs,
      int chunkSize = 1,
      int sleepUsecsBetweenChunks = 0) {
    auto originalStats =
        getHwSwitchEnsemble()->getLatestPortStats(masterLogicalPortIds());
    int totalPackets = srcMacs.size() * dstMacs.size();
    auto allSent = [&originalStats, totalPackets](const auto& newStats) {
      auto originalPkts = getPortInPkts(originalStats);
      auto newPkts = getPortInPkts(newStats);
      auto expectedPkts = originalPkts + totalPackets;
      XLOGF(
          INFO,
          "Checking packets sent. Old: {}, New: {}, Expected: {}",
          originalPkts,
          newPkts,
          expectedPkts);
      return newPkts == expectedPkts;
    };
    int numSentPackets = 0;
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

        ++numSentPackets;
        // Some platform uses software learning which takes a little bit longer
        // for SDK learn the L2 addresses from hardware.
        // Add an explicit sleep time to avoid flooding the cache and dropping
        // the mac addresses.
        if (sleepUsecsBetweenChunks > 0 && chunkSize > 0 &&
            (numSentPackets % chunkSize == 0 ||
             numSentPackets == totalPackets)) {
          /* sleep override */
          usleep(sleepUsecsBetweenChunks);
        }
      }
    }
    getHwSwitchEnsemble()->waitPortStatsCondition(allSent);
  }

  folly::MacAddress generateNextMac(folly::MacAddress curMac) {
    auto generator = utility::MacAddressGenerator();
    generator.startOver(curMac.u64HBO());
    return generator.getNext();
  }

  void verifyAllMacsLearnt(const std::vector<folly::MacAddress>& macs) {
    // Because the last verify() will send out some packets with kSourceMac()
    // to verify whether macs are learnt, the SDK/HW might learn kSourceMac(),
    // but this is not what we care and we only care whether the HW has learnt
    // all addresses in `macs`
    const auto& learntMacs =
        getMacsForPort(getHwSwitch(), masterLogicalPortIds()[0], false);
    EXPECT_TRUE(learntMacs.size() >= macs.size());

    bringUpPort(masterLogicalPortIds()[1]);
    auto origPortStats =
        getHwSwitchEnsemble()->getLatestPortStats(masterLogicalPortIds()[1]);
    // Send packets to macs which are expected to have been learned now
    sendL2Pkts(
        *initialConfig().vlanPorts()[0].vlanID(),
        std::nullopt,
        {kSourceMac()},
        macs);
    auto curPortStats =
        getHwSwitchEnsemble()->getLatestPortStats(masterLogicalPortIds()[1]);
    // All packets should have gone through masterLogicalPortIds()[0], port
    // on which macs were learnt and none through any other port in the same
    // VLAN. This lack of broadcast confirms MAC learning.
    EXPECT_EQ(*curPortStats.outBytes_(), *origPortStats.outBytes_());
    bringDownPort(masterLogicalPortIds()[1]);
  }
};

// Intent of this test is to attempt to learn large number of macs
// (L2_LEARN_MAX_MAC_COUNT) and ensure HW can learn them.
TEST_F(HwMacLearningBatchEntriesTest, VerifyMacLearningScale) {
  int chunkSize = 1;
  int sleepUsecsBetweenChunks = 0;
  std::tie(chunkSize, sleepUsecsBetweenChunks) = getMacChunkSizeAndSleepUsecs();

  std::vector<folly::MacAddress> macs = generateMacs(L2_LEARN_MAX_MAC_COUNT);

  auto portDescr = physPortDescr();
  auto setup = [this, portDescr, chunkSize, sleepUsecsBetweenChunks, &macs]() {
    setupHelper(cfg::L2LearningMode::HARDWARE, portDescr);
    // Disable aging, so entry stays in L2 table when we verify.
    utility::setMacAgeTimerSeconds(getHwSwitchEnsemble(), 0);
    sendL2Pkts(
        *initialConfig().vlanPorts()[0].vlanID(),
        masterLogicalPortIds()[0],
        macs,
        {folly::MacAddress::BROADCAST},
        chunkSize,
        sleepUsecsBetweenChunks);
  };

  auto verify = [this, &macs]() { verifyAllMacsLearnt(macs); };

  // MACs learned should be preserved across warm boot
  verifyAcrossWarmBoots(setup, verify);
}

// Intent of this test is to attempt to learn large number of macs
// and then change age time to 1s so that SDK/HW will age all of them correctly.
TEST_F(HwMacLearningBatchEntriesTest, VerifyMacAgingScale) {
  int chunkSize = 1;
  int sleepUsecsBetweenChunks = 0;
  std::tie(chunkSize, sleepUsecsBetweenChunks) = getMacChunkSizeAndSleepUsecs();

  std::vector<folly::MacAddress> macs = generateMacs(L2_LEARN_MAX_MAC_COUNT);

  auto portDescr = physPortDescr();
  auto setup = [this, portDescr, chunkSize, sleepUsecsBetweenChunks, &macs]() {
    setupHelper(cfg::L2LearningMode::HARDWARE, portDescr);

    // First we disable aging, so entry stays in L2 table when we verify.
    utility::setMacAgeTimerSeconds(getHwSwitchEnsemble(), 0);
    sendL2Pkts(
        *initialConfig().vlanPorts()[0].vlanID(),
        masterLogicalPortIds()[0],
        macs,
        {folly::MacAddress::BROADCAST},
        chunkSize,
        sleepUsecsBetweenChunks);

    // Confirm that the SDK has learn all the macs
    const auto& learntMacs =
        getMacsForPort(getHwSwitch(), masterLogicalPortIds()[0], false);
    EXPECT_EQ(learntMacs.size(), macs.size());

    // Force MAC aging to as fast as possible but min is still 1 second
    utility::setMacAgeTimerSeconds(getHwSwitchEnsemble(), kMinAgeInSecs());
  };

  auto verify = [this]() {
    // We can't always guarante that even setting 1s to age, the SDK will
    // age all the MACs out that fast. Add some retry mechanism to make the
    // test more robust
    // Ref: wasMacLearnt() comments
    int waitForAging = 10;
    while (waitForAging) {
      /* sleep override */
      sleep(2 * kMinAgeInSecs());
      const auto& afterAgingMacs =
          getMacsForPort(getHwSwitch(), masterLogicalPortIds()[0], false);
      if (afterAgingMacs.size() == 0) {
        break;
      }
      --waitForAging;
    }

    if (waitForAging == 0) {
      // After 20 seconds, let's check whether there're still some MAC
      // remained in HW
      const auto& afterAgingMacs =
          getMacsForPort(getHwSwitch(), masterLogicalPortIds()[0], false);
      EXPECT_EQ(afterAgingMacs.size(), 0);
    }
  };

  // MACs learned should be preserved across warm boot
  verifyAcrossWarmBoots(setup, verify);
}

// Intent of this test is to attempt to SDK will still be able to learn new
// MACs if the L2 table is not full.
// We recently noticed a bug on TH3 that the SDK internal counter wasn't
// correctly updated, which caused the SDK not able to learn any new MAC even
// the L2 table wasn't full.
TEST_F(HwMacLearningBatchEntriesTest, VerifyMacLearningAgingReleaseResource) {
  int chunkSize = 1;
  int sleepUsecsBetweenChunks = 0;
  std::tie(chunkSize, sleepUsecsBetweenChunks) = getMacChunkSizeAndSleepUsecs();

  // To make it more similar to the real production environment, we try to
  // send 1k MAC addresses at a time, and then let them age out, and then
  // repeat the whole process again for 9 times, and at the final time, we don't
  // age them so that we can verify whether these 1K MACs learned.
  // Basically let the SDK learn MAC addresses for 10K times.
  constexpr int kMultiAgingTestMacsSize = 1000;
  std::vector<folly::MacAddress> macs = generateMacs(kMultiAgingTestMacsSize);

  auto portDescr = physPortDescr();
  auto setup = [this, portDescr, chunkSize, sleepUsecsBetweenChunks, &macs]() {
    setupHelper(cfg::L2LearningMode::HARDWARE, portDescr);

    constexpr int kForceAgingTimes = 10;
    for (int i = 0; i < kForceAgingTimes; ++i) {
      // First we disable aging, so entry stays in L2 table when we verify.
      utility::setMacAgeTimerSeconds(getHwSwitchEnsemble(), 0);
      sendL2Pkts(
          *initialConfig().vlanPorts()[0].vlanID(),
          masterLogicalPortIds()[0],
          macs,
          {folly::MacAddress::BROADCAST},
          chunkSize,
          sleepUsecsBetweenChunks);

      // Confirm that the SDK has learn all the macs
      const auto& learntMacs =
          getMacsForPort(getHwSwitch(), masterLogicalPortIds()[0], false);
      EXPECT_EQ(learntMacs.size(), macs.size());

      // Now force the SDK age them out if it is not the last time.
      if (i == kForceAgingTimes - 1) {
        break;
      }

      // Force MAC aging to as fast as possible but min is still 1 second
      utility::setMacAgeTimerSeconds(getHwSwitchEnsemble(), kMinAgeInSecs());
      // We can't always guarante that even setting 1s to age, the SDK will
      // age all the MACs out that fast. Add some retry mechanism to make the
      // test more robust
      int waitForAging = 5;
      while (waitForAging) {
        /* sleep override */
        sleep(2 * kMinAgeInSecs());
        const auto& afterAgingMacs =
            getMacsForPort(getHwSwitch(), masterLogicalPortIds()[0], false);
        if (afterAgingMacs.size() == 0) {
          break;
        }
        --waitForAging;
      }
      if (waitForAging == 0) {
        // After 10 seconds, let's check whether there're still some MAC
        // remained in HW
        const auto& afterAgingMacs =
            getMacsForPort(getHwSwitch(), masterLogicalPortIds()[0], false);
        EXPECT_EQ(afterAgingMacs.size(), 0);
      } else {
        XLOGF(INFO, "Successfully aged all MACs in HW for #{} times", i + 1);
      }
    }
  };

  auto verify = [this, &macs]() { verifyAllMacsLearnt(macs); };

  // MACs learned should be preserved across warm boot
  verifyAcrossWarmBoots(setup, verify);
}

// Intent of this test is to attempt to verify if we have repeated MACs learnt,
// the SDK will still be able to learn new MACs if the L2 table is not full.
// We recently noticed a bug on TH3 that the SDK internal counter counter is
// updated even if the existing table entry was just updated;
// which caused the SDK not able to learn any new MAC even the L2 table wasn't
// full.
TEST_F(HwMacLearningBatchEntriesTest, VerifyMacLearningUpdateExistingL2Entry) {
  int chunkSize = 1;
  int sleepUsecsBetweenChunks = 0;
  std::tie(chunkSize, sleepUsecsBetweenChunks) = getMacChunkSizeAndSleepUsecs();

  // To make it more similar to the real production environment, we try to
  // send 1k MAC addresses for 11 times, and then send a new packet with new mac
  // address.
  // This will tell us whether updating them same 1K MAC for multiples times
  // will make counter incorrectly and eventually stop us adding a new MAC even
  // though the table is not full yet.
  constexpr int kMultiAgingTestMacsSize = 1000;
  std::vector<folly::MacAddress> macs = generateMacs(kMultiAgingTestMacsSize);
  // Add an extra mac
  auto extraMac = generateNextMac(macs.back());

  auto portDescr = physPortDescr();
  auto setup = [this,
                portDescr,
                chunkSize,
                sleepUsecsBetweenChunks,
                &macs,
                &extraMac]() {
    setupHelper(cfg::L2LearningMode::HARDWARE, portDescr);

    constexpr int kRepeatTimes = 11;
    // First we disable aging, so entry stays in L2 table when we verify.
    utility::setMacAgeTimerSeconds(getHwSwitchEnsemble(), 0);
    // Changing the source ports to trigger L2 entry change
    for (int i = 0; i < kRepeatTimes; ++i) {
      // Changing the same MAC to use different source port, so that trigger
      // SDK to update the existing MAC entry.
      auto upPortID = masterLogicalPortIds()[i % 2];
      auto downPortID = masterLogicalPortIds()[(i + 1) % 2];
      bringUpPort(upPortID);
      bringDownPort(downPortID);

      sendL2Pkts(
          *initialConfig().vlanPorts()[0].vlanID(),
          upPortID,
          macs,
          {folly::MacAddress::BROADCAST},
          chunkSize,
          sleepUsecsBetweenChunks);

      // Confirm that the SDK has learn all the macs
      const auto& upPortLearntMacs =
          getMacsForPort(getHwSwitch(), upPortID, false);
      EXPECT_EQ(upPortLearntMacs.size(), macs.size());
      const auto& downPortLearntMacs =
          getMacsForPort(getHwSwitch(), downPortID, false);
      EXPECT_EQ(downPortLearntMacs.size(), 0);
      XLOGF(INFO, "Successfully updated all MACs in HW for #{} times", i + 1);
    }

    // Add an extra mac
    sendL2Pkts(
        *initialConfig().vlanPorts()[0].vlanID(),
        masterLogicalPortIds()[0],
        {extraMac},
        {folly::MacAddress::BROADCAST},
        chunkSize,
        sleepUsecsBetweenChunks);
  };

  auto verify = [this, &macs, &extraMac]() {
    // Verify all orignal 1K macs and the extra one have been learnt
    macs.push_back(extraMac);
    verifyAllMacsLearnt(macs);
  };

  // MACs learned should be preserved across warm boot
  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
