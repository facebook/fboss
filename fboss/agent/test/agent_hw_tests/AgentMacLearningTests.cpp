/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/MacTableUtils.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/packet/Ethertype.h"
#include "fboss/agent/state/PortDescriptor.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/TestUtils.h"
#include "fboss/agent/test/TrunkUtils.h"
#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/agent/test/utils/L2LearningUpdateObserverUtil.h"
#include "fboss/agent/test/utils/MacTestUtils.h"
#include "fboss/agent/test/utils/PacketTestUtils.h"
#include "fboss/agent/test/utils/PortStatsTestUtils.h"
#include "fboss/lib/CommonUtils.h"

#include <folly/IPAddress.h>

#include <memory>
#include <set>
#include <vector>

using facebook::fboss::HwAsic;
using facebook::fboss::L2EntryThrift;
using folly::IPAddress;
using folly::IPAddressV4;
using folly::IPAddressV6;

namespace {
std::set<folly::MacAddress>
getMacsForPort(facebook::fboss::SwSwitch* sw, int port, bool isTrunk) {
  std::set<folly::MacAddress> macs;
  std::vector<L2EntryThrift> l2Entries;
  facebook::fboss::ThriftHandler handler(sw);
  handler.getL2Table(l2Entries);
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

class AgentMacLearningTest : public AgentHwTest {
 protected:
  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    return {production_features::ProductionFeature::MAC_LEARNING};
  }

  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto switchId = ensemble.getSw()
                        ->getScopeResolver()
                        ->scope(ensemble.masterLogicalPortIds())
                        .switchId();
    auto asic = ensemble.getSw()->getHwAsicTable()->getHwAsic(switchId);
    auto cfg = utility::oneL3IntfTwoPortConfig(
        ensemble.getSw()->getPlatformMapping(),
        asic,
        ensemble.masterLogicalPortIds()[0],
        ensemble.masterLogicalPortIds()[1],
        ensemble.getSw()->getPlatformSupportsAddRemovePort(),
        asic->desiredLoopbackModes());
    return cfg;
  }

  static MacAddress kSourceMac() {
    return MacAddress("02:00:00:00:00:05");
  }

  static MacAddress kSourceMac2() {
    return MacAddress("02:00:00:00:00:06");
  }

  void sendPkt(MacAddress srcMac = kSourceMac()) {
    auto txPacket = utility::makeEthTxPacket(
        getSw(),
        VlanID(*initialConfig(*getAgentEnsemble()).vlanPorts()[0].vlanID()),
        srcMac,
        MacAddress::BROADCAST,
        ETHERTYPE::ETHERTYPE_LLDP);
    getSw()->sendPacketOutOfPortAsync(
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
        utility::getFirstNodeIf(getProgrammedState()->getSwitchSettings())
            ->getL2LearningMode();

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

    // State udpate that will add/remove MacEntry happens asynchronously in
    // Event base. Give it chance to run.
    // Typically the MAC learning is immediate post a packet sent, but retries
    // help avoid test noise.

    WITH_RETRIES({
      if (l2LearningMode == cfg::L2LearningMode::SOFTWARE) {
        EXPECT_EVENTUALLY_TRUE(wasMacLearntInSwitchState(shouldExist, mac));
        EXPECT_EVENTUALLY_TRUE(isInL2Table(portDescr, mac, shouldExist));
        return true;
      }
      if (l2LearningMode == cfg::L2LearningMode::HARDWARE) {
        EXPECT_EVENTUALLY_TRUE(isInL2Table(portDescr, mac, shouldExist));
        return true;
      }
    });

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
        VlanID(*initialConfig(*getAgentEnsemble()).vlanPorts()[0].vlanID()));
    EXPECT_EQ(l2Entry.getPort(), portDescr);
    EXPECT_EQ(l2Entry.getType(), expectedL2EntryType);
    EXPECT_EQ(l2EntryUpdateType, expectedL2EntryUpdateType);
  }
  void setL2LearningMode(cfg::L2LearningMode l2LearningMode) {
    if (utility::getFirstNodeIf(getProgrammedState()->getSwitchSettings())
            ->getL2LearningMode() == l2LearningMode) {
      return;
    }
    applyNewState(
        [&](const std::shared_ptr<SwitchState>& in) {
          auto newState = in->clone();
          auto switchSettings =
              utility::getFirstNodeIf(newState->getSwitchSettings());
          auto newSwitchSettings = switchSettings->modify(&newState);
          newSwitchSettings->setL2LearningMode(l2LearningMode);
          return newState;
        },
        "setL2LearningMode");
  }
  void setupHelper(
      cfg::L2LearningMode l2LearningMode,
      PortDescriptor portDescr) {
    auto newCfg{initialConfig(*getAgentEnsemble())};
    *newCfg.switchSettings()->l2LearningMode() = l2LearningMode;

    if (portDescr.isAggregatePort()) {
      utility::findCfgPort(newCfg, masterLogicalPortIds()[0])->state() =
          cfg::PortState::ENABLED;
      addAggPort(
          std::numeric_limits<AggregatePortID>::max(),
          {masterLogicalPortIds()[0]},
          &newCfg);
      applyNewConfig(newCfg);
      applyNewState(
          [&](const std::shared_ptr<SwitchState>& in) {
            auto newState = enableTrunkPorts(in);
            return newState;
          },
          "setL2LearningMode");
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
    auto switchId = getSw()
                        ->getScopeResolver()
                        ->scope(masterLogicalPortIds()[0])
                        .switchId();
    /*
     * TH learns the entry as PENDING, TH3 and newer asics learns the entry as
     * VALIDATED
     */
    auto asic = getSw()->getHwAsicTable()->getHwAsic(switchId);
    return (asic->isSupported(HwAsic::Feature::PENDING_L2_ENTRY))
        ? L2Entry::L2EntryType::L2_ENTRY_TYPE_PENDING
        : L2Entry::L2EntryType::L2_ENTRY_TYPE_VALIDATED;
  }

  void testHwLearningHelper(PortDescriptor portDescr) {
    auto setup = [this, portDescr]() {
      setupHelper(cfg::L2LearningMode::HARDWARE, portDescr);
      // Disable aging, so entry stays in L2 table when we verify.
      utility::setMacAgeTimerSeconds(getAgentEnsemble(), 0);
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
      utility::setMacAgeTimerSeconds(getAgentEnsemble(), 0);
      sendPkt();
      EXPECT_TRUE(wasMacLearnt(portDescr, kSourceMac()));

      // Force MAC aging to as fast as possible but min is still 1 second
      utility::setMacAgeTimerSeconds(getAgentEnsemble(), kMinAgeInSecs());
      EXPECT_TRUE(wasMacLearnt(portDescr, kSourceMac(), false /* MAC aged */));
    };

    verifyAcrossWarmBoots(setup, verify);
  }

  void testHwToSwLearningHelper(PortDescriptor portDescr) {
    auto setup = [this, portDescr]() {
      setupHelper(cfg::L2LearningMode::HARDWARE, portDescr);

      // Disable aging, so entry stays in L2 table when we verify.
      utility::setMacAgeTimerSeconds(getAgentEnsemble(), 0);
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
      utility::setMacAgeTimerSeconds(getAgentEnsemble(), 0);

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
    return VlanID(*initialConfig(*getAgentEnsemble()).vlanPorts()[0].vlanID());
  }

  L2LearningUpdateObserverUtil l2LearningObserver_;

 private:
  bool wasMacLearntInHw(bool shouldExist, MacAddress mac) {
    bringUpPort(masterLogicalPortIds()[1]);
    auto origPortStats =
        getAgentEnsemble()->getLatestPortStats(masterLogicalPortIds()[1]);
    auto txPacket = utility::makeEthTxPacket(
        getSw(),
        VlanID(*initialConfig(*getAgentEnsemble()).vlanPorts()[0].vlanID()),
        kSourceMac2(),
        mac,
        ETHERTYPE::ETHERTYPE_LLDP);

    getSw()->sendPacketSwitchedAsync(std::move(txPacket));
    auto newPortStats =
        getAgentEnsemble()->getLatestPortStats(masterLogicalPortIds()[1]);

    bringDownPort(masterLogicalPortIds()[1]);
    auto newPortBytes = *newPortStats.outBytes_() - *origPortStats.outBytes_();
    // If MAC should have been learnt then we should get no traffic on
    // masterLogicalPortIds[1], otherwise we should see flooding and
    // non zero bytes on masterLogicalPortIds[1]
    return (shouldExist == (newPortBytes == 0));
  }

  bool wasMacLearntInSwitchState(bool shouldExist, MacAddress mac) const {
    auto vlanID =
        VlanID(*initialConfig(*getAgentEnsemble()).vlanPorts()[0].vlanID());
    auto state = getProgrammedState();
    auto vlan = state->getVlans()->getNodeIf(vlanID);
    auto* macTable = vlan->getMacTable().get();
    return (shouldExist == (macTable->getMacIf(mac) != nullptr));
  }
  bool isInL2Table(
      const PortDescriptor& portDescr,
      folly::MacAddress mac,
      bool shouldExist) const {
    auto isTrunk = portDescr.isAggregatePort();
    int portId = isTrunk ? portDescr.aggPortID() : portDescr.phyPortID();
    auto macs = getMacsForPort(getSw(), portId, isTrunk);
    return (shouldExist == (macs.find(mac) != macs.end()));
  }
};

class AgentMacSwLearningModeTest : public AgentMacLearningTest {
 public:
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto switchId = ensemble.getSw()
                        ->getScopeResolver()
                        ->scope(ensemble.masterLogicalPortIds()[0])
                        .switchId();
    auto asic = ensemble.getSw()->getHwAsicTable()->getHwAsic(switchId);
    auto cfg = utility::oneL3IntfTwoPortConfig(
        ensemble.getSw()->getPlatformMapping(),
        asic,
        ensemble.masterLogicalPortIds()[0],
        ensemble.masterLogicalPortIds()[1],
        ensemble.getSw()->getPlatformSupportsAddRemovePort(),
        asic->desiredLoopbackModes());
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
      getAgentEnsemble()->bringDownPort(masterLogicalPortIds()[1]);
      // Disable aging, so entry stays in L2 table when we verify.
      utility::setMacAgeTimerSeconds(getAgentEnsemble(), 0);
      l2LearningObserver_.startObserving(getAgentEnsemble());
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
      getAgentEnsemble()->bringDownPort(masterLogicalPortIds()[1]);
      // Disable aging, so entry stays in L2 table when we verify.
      utility::setMacAgeTimerSeconds(getAgentEnsemble(), 0);
      l2LearningObserver_.startObserving(getAgentEnsemble());
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
      getAgentEnsemble()->bringDownPort(masterLogicalPortIds()[1]);
      // Disable aging, so entry stays in L2 table when we verify.
      utility::setMacAgeTimerSeconds(getAgentEnsemble(), 0);
      l2LearningObserver_.startObserving(getAgentEnsemble());
      induceMacLearning(portDescr);

      EXPECT_TRUE(wasMacLearnt(portDescr, kSourceMac()));

      // Force MAC aging to as fast as possible but min is still 1 second
      l2LearningObserver_.reset();
      utility::setMacAgeTimerSeconds(getAgentEnsemble(), kMinAgeInSecs());

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
    applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      auto macEntry = std::make_shared<MacEntry>(kSourceMac(), physPortDescr());
      auto newState = MacTableUtils::updateOrAddEntryWithClassID(
          in, kVlanID(), macEntry, kClassID());
      return newState;
    });
  }

  void disassociateClassID() {
    applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      auto macEntry = std::make_shared<MacEntry>(kSourceMac(), physPortDescr());
      auto newState =
          MacTableUtils::removeClassIDForEntry(in, kVlanID(), macEntry);
      return newState;
    });
  }

  cfg::AclLookupClass kClassID() {
    return cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_2;
  }
};

class AgentMacLearningStaticEntriesTest : public AgentMacLearningTest {
 protected:
  void addOrUpdateMacEntry(MacEntryType type) {
    applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      auto newState = in->clone();
      auto vlan = newState->getVlans()->getNodeIf(kVlanID()).get();
      auto macTable = vlan->getMacTable().get();
      macTable = macTable->modify(&vlan, &newState);
      if (macTable->getMacIf(kSourceMac())) {
        macTable->updateEntry(
            kSourceMac(), physPortDescr(), std::nullopt, type);
      } else {
        auto macEntry = std::make_shared<MacEntry>(
            kSourceMac(),
            physPortDescr(),
            std::optional<cfg::AclLookupClass>(std::nullopt),
            type);
        macTable->addEntry(macEntry);
      }
      return newState;
    });
  }
};

TEST_F(AgentMacLearningStaticEntriesTest, VerifyStaticMacEntryAdd) {
  auto setup = [this] {
    setupHelper(cfg::L2LearningMode::HARDWARE, physPortDescr());
    utility::setMacAgeTimerSeconds(getAgentEnsemble(), kMinAgeInSecs());
    addOrUpdateMacEntry(MacEntryType::STATIC_ENTRY);
  };
  auto verify = [this] {
    std::this_thread::sleep_for(std::chrono::seconds(2 * kMinAgeInSecs()));
    // Static entries shouldn't age
    EXPECT_TRUE(wasMacLearnt(physPortDescr(), kSourceMac()));
  };
  verifyAcrossWarmBoots(setup, verify);
}

class AgentMacLearningAndMyStationInteractionTest
    : public AgentMacLearningTest {
 public:
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto switchId = ensemble.getSw()
                        ->getScopeResolver()
                        ->scope(ensemble.masterLogicalPortIds()[0])
                        .switchId();
    auto asic = ensemble.getSw()->getHwAsicTable()->getHwAsic(switchId);
    auto cfg = utility::onePortPerInterfaceConfig(
        ensemble.getSw()->getPlatformMapping(),
        asic,
        ensemble.masterLogicalPortIds(),
        ensemble.getSw()->getPlatformSupportsAddRemovePort(),
        asic->desiredLoopbackModes());
    return cfg;
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
      applyNewState([&](const std::shared_ptr<SwitchState>& in) {
        auto newState = ecmp6.resolveNextHops(
            in, {PortDescriptor(masterLogicalPortIds()[0])});
        return newState;
      });
      auto wrapper = getSw()->getRouteUpdater();
      ecmp6.programRoutes(
          &wrapper, {PortDescriptor(masterLogicalPortIds()[0])});
    };
    auto verify = [this]() {
      if (getSw()->getBootType() == BootType::WARM_BOOT) {
        // Let mac age out learned during prior run
        utility::setMacAgeTimerSeconds(getAgentEnsemble(), kMinAgeInSecs());
        // @lint-ignore CLANGTIDY
        sleep(kMinAgeInSecs() * 3);
      }
      utility::setMacAgeTimerSeconds(getAgentEnsemble(), 0);
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
          for (const auto& iter : std::as_const(*intf->getAddresses())) {
            auto addrEntry = folly::IPAddress(iter.first);
            if (!addrEntry.isV4()) {
              continue;
            }
            auto v4Addr = addrEntry.asV4();
            l2LearningObserver_.reset();
            auto arpPacket = utility::makeARPTxPacket(
                getSw(),
                intf->getVlanID(),
                intf->getMac(),
                folly::MacAddress::BROADCAST,
                v4Addr,
                v4Addr,
                ARP_OPER::ARP_OPER_REQUEST,
                folly::MacAddress::BROADCAST);
            getSw()->sendPacketOutOfPortAsync(std::move(arpPacket), port);
            l2LearningObserver_.waitForLearningUpdates(1, 1);
          }
        }
      };
      induceMacLearning();
      utility::setMacAgeTimerSeconds(getAgentEnsemble(), kMinAgeInSecs());
      // Let mac age out
      sleep(kMinAgeInSecs() * 3);
      // Re learn MAC so FDB entries are learnt and the packet
      // below is not sacrificed to a MAC learning callback event.
      // Secondly its important to WB with L2 entries learnt for
      // the bug seen in S234435 to repro.
      utility::setMacAgeTimerSeconds(getAgentEnsemble(), 0);
      induceMacLearning();
      auto sendRoutedPkt = [this]() {
        auto vlanId =
            VlanID(*initialConfig(*getAgentEnsemble()).vlanPorts()[0].vlanID());
        auto intfMac = utility::getInterfaceMac(getProgrammedState(), vlanId);
        auto txPacket = utility::makeIpTxPacket(
            [sw = getSw()](uint32_t size) { return sw->allocatePacket(size); },
            vlanId,
            intfMac,
            intfMac,
            folly::IPAddress("1000::1"),
            folly::IPAddress("2000::1"));
        getSw()->sendPacketOutOfPortAsync(
            std::move(txPacket), masterLogicalPortIds()[1]);
      };
      HwPortStats oldPortStats;
      WITH_RETRIES({
        auto portStatsBefore = getLatestPortStats(masterLogicalPortIds());
        EXPECT_EVENTUALLY_NE(
            portStatsBefore.find(masterLogicalPortIds()[0]),
            portStatsBefore.end());
        if (portStatsBefore.find(masterLogicalPortIds()[0]) !=
            portStatsBefore.end()) {
          oldPortStats = portStatsBefore[masterLogicalPortIds()[0]];
        }
      });
      WITH_RETRIES({
        /*
         * Send routed packet, with SW learning, we can get
         * the first packet lost to MAC learning as the src mac
         * may still be in PENDING state on some platforms. Resent
         * here, we are looking for persisting my station breakage
         * and not a single packet racing with mac learning
         */
        sendRoutedPkt();
        auto portStatsAfter = getLatestPortStats(masterLogicalPortIds());
        EXPECT_EVENTUALLY_NE(
            portStatsAfter.find(masterLogicalPortIds()[0]),
            portStatsAfter.end());
        if (portStatsAfter.find(masterLogicalPortIds()[0]) !=
            portStatsAfter.end()) {
          auto newPortStats =
              portStatsAfter.find(masterLogicalPortIds()[0])->second;
          auto oldOutBytes = *oldPortStats.outBytes_();
          auto newOutBytes = *newPortStats.outBytes_();
          XLOG(DBG2) << " Port Id: " << masterLogicalPortIds()[0]
                     << " Old out bytes: " << oldOutBytes
                     << " New out bytes: " << newOutBytes
                     << " Delta : " << (newOutBytes - oldOutBytes);
          EXPECT_EVENTUALLY_TRUE(newOutBytes > oldOutBytes);
        }
      });
    };
    verifyAcrossWarmBoots(setup, verify);
  }
};

class AgentMacLearningAndMyStationInteractionTestHw
    : public AgentMacLearningAndMyStationInteractionTest {
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto cfg =
        AgentMacLearningAndMyStationInteractionTest::initialConfig(ensemble);
    cfg.switchSettings()->l2LearningMode() = cfg::L2LearningMode::HARDWARE;
    return cfg;
  }
};

class AgentMacLearningAndMyStationInteractionTestSw
    : public AgentMacLearningAndMyStationInteractionTest {
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto cfg =
        AgentMacLearningAndMyStationInteractionTest::initialConfig(ensemble);
    cfg.switchSettings()->l2LearningMode() = cfg::L2LearningMode::SOFTWARE;
    return cfg;
  }
};

TEST_F(
    AgentMacLearningAndMyStationInteractionTestHw,
    verifyInteractionHwMacLearning) {
  testMyStationInteractionHelper(cfg::L2LearningMode::HARDWARE);
}
TEST_F(
    AgentMacLearningAndMyStationInteractionTestSw,
    verifyInteractionSwMacLearning) {
  testMyStationInteractionHelper(cfg::L2LearningMode::SOFTWARE);
}

TEST_F(AgentMacLearningTest, VerifyHwLearningForPort) {
  testHwLearningHelper(physPortDescr());
}

TEST_F(AgentMacLearningTest, VerifyHwLearningForTrunk) {
  testHwLearningHelper(aggPortDescr());
}

TEST_F(AgentMacLearningTest, VerifyHwAgingForPort) {
  testHwAgingHelper(physPortDescr());
}

TEST_F(AgentMacLearningTest, VerifyHwAgingForTrunk) {
  testHwAgingHelper(aggPortDescr());
}

TEST_F(AgentMacSwLearningModeTest, VerifySwLearningForPort) {
  testSwLearningHelper(physPortDescr());
}

TEST_F(AgentMacSwLearningModeTest, VerifySwLearningForPortNoCycle) {
  testSwLearningNoCycleHelper(physPortDescr());
}

TEST_F(AgentMacSwLearningModeTest, VerifySwLearningForTrunk) {
  testSwLearningHelper(aggPortDescr());
}

TEST_F(AgentMacSwLearningModeTest, VerifySwAgingForPort) {
  testSwAgingHelper(physPortDescr());
}

TEST_F(AgentMacSwLearningModeTest, VerifySwAgingForTrunk) {
  testSwAgingHelper(aggPortDescr());
}

TEST_F(AgentMacSwLearningModeTest, VerifyNbrMacInL2Table) {
  auto setup = [this] {
    getAgentEnsemble()->bringDownPort(masterLogicalPortIds()[1]);
    utility::EcmpSetupTargetedPorts6 ecmpHelper6{
        getProgrammedState(), kSourceMac()};
    applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      auto newState = ecmpHelper6.resolveNextHops(in, {physPortDescr()});
      return newState;
    });
  };
  auto verify = [this] { wasMacLearnt(physPortDescr(), kSourceMac()); };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentMacSwLearningModeTest, VerifyCallbacksOnMacEntryChange) {
  auto verify = [this]() {
    auto switchId = getSw()
                        ->getScopeResolver()
                        ->scope(masterLogicalPortIds()[1])
                        .switchId();
    auto asic = getSw()->getHwAsicTable()->getHwAsic(switchId);
    bool isTH3 = asic->getAsicType() == cfg::AsicType::ASIC_TYPE_TOMAHAWK3;
    // Disable aging, so entry stays in L2 table when we verify.
    utility::setMacAgeTimerSeconds(getAgentEnsemble(), 0);
    enum class MacOp { ASSOCIATE, DISSOASSOCIATE, DELETE };
    getAgentEnsemble()->bringDownPort(masterLogicalPortIds()[1]);
    l2LearningObserver_.startObserving(getAgentEnsemble());
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
          utility::setMacAgeTimerSeconds(getAgentEnsemble(), kMinAgeInSecs());

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
            WITH_RETRIES({
              auto macTable = getProgrammedState()
                                  ->getVlans()
                                  ->getNodeIf(kVlanID())
                                  ->getMacTable();
              auto macEntry = macTable->getMacIf(kSourceMac());
              EXPECT_EVENTUALLY_NE(macEntry, nullptr);
              if (macEntry) {
                EXPECT_EVENTUALLY_EQ(macEntry->getClassID(), lookupClass);
              }
            });
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
  verifyAcrossWarmBoots([]() {}, verify);
}

class AgentMacLearningMacMoveTest : public AgentMacLearningTest {
 protected:
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto switchId = ensemble.getSw()
                        ->getScopeResolver()
                        ->scope(ensemble.masterLogicalPortIds()[0])
                        .switchId();
    auto asic = ensemble.getSw()->getHwAsicTable()->getHwAsic(switchId);
    auto cfg = utility::oneL3IntfTwoPortConfig(
        ensemble.getSw()->getPlatformMapping(),
        asic,
        ensemble.masterLogicalPortIds()[0],
        ensemble.masterLogicalPortIds()[1],
        ensemble.getSw()->getPlatformSupportsAddRemovePort(),
        asic->desiredLoopbackModes());
    cfg.switchSettings()->l2LearningMode() = cfg::L2LearningMode::SOFTWARE;
    return cfg;
  }

  void sendPkt2() {
    auto txPacket = utility::makeEthTxPacket(
        getSw(),
        VlanID(*initialConfig(*getAgentEnsemble()).vlanPorts()[0].vlanID()),
        kSourceMac(),
        MacAddress::BROADCAST,
        ETHERTYPE::ETHERTYPE_LLDP);

    getSw()->sendPacketOutOfPortAsync(
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
      utility::setMacAgeTimerSeconds(getAgentEnsemble(), 0);

      l2LearningObserver_.startObserving(getAgentEnsemble());
      XLOG(DBG2) << "Send pkt on up port, other port is down";
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

      auto switchId =
          getSw()->getScopeResolver()->scope(portDescr.phyPortID()).switchId();
      auto asic = getSw()->getHwAsicTable()->getHwAsic(switchId);

      // When MAC Moves from port1 to port2, we get DELETE on port1 and ADD on
      // port2
      if (asic->getAsicType() == cfg::AsicType::ASIC_TYPE_EBRO) {
        // TODO: Remove this once EbroAsic properly generates a
        // MAC move event.
        verifyL2TableCallback(
            l2LearningObserver_.waitForLearningUpdates().front(),
            portDescr2,
            L2EntryUpdateType::L2_ENTRY_UPDATE_TYPE_ADD,
            expectedL2EntryTypeOnAdd());
      } else if (asic->isSupported(HwAsic::Feature::DETAILED_L2_UPDATE)) {
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
      utility::setMacAgeTimerSeconds(getAgentEnsemble(), kMinAgeInSecs());
      l2LearningObserver_.reset();

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

TEST_F(AgentMacLearningMacMoveTest, VerifyMacMoveForPort) {
  testMacMoveHelper();
}
} // namespace facebook::fboss
