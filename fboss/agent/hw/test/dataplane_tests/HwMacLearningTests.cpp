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
#include "fboss/agent/test/TrunkUtils.h"

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
std::set<folly::MacAddress>
getMacsForPort(const facebook::fboss::HwSwitch* hw, int port, bool isTrunk) {
  std::set<folly::MacAddress> macs;
  std::vector<L2EntryThrift> l2Entries;
  hw->fetchL2Table(&l2Entries);
  for (auto& l2Entry : l2Entries) {
    if ((isTrunk && l2Entry.trunk_ref().value_unchecked() == port) ||
        l2Entry.port == port) {
      macs.insert(folly::MacAddress(l2Entry.mac));
    }
  }
  return macs;
}
} // namespace

namespace facebook {
namespace fboss {

using utility::addAggPort;
using utility::enableTrunkPorts;

class HwMacLearningTest : public HwLinkStateDependentTest {
 protected:
  cfg::SwitchConfig initialConfig() const override {
    return utility::oneL3IntfConfig(
        getHwSwitch(), masterLogicalPortIds()[0], cfg::PortLoopbackMode::MAC);
  }

  MacAddress kSourceMac() const {
    return MacAddress("02:00:00:00:00:05");
  }

  void sendPkt() {
    auto txPacket = utility::makeEthTxPacket(
        getHwSwitch(),
        VlanID(initialConfig().vlanPorts[0].vlanID),
        kSourceMac(),
        MacAddress::BROADCAST,
        ETHERTYPE::ETHERTYPE_LLDP);

    getHwSwitch()->sendPacketOutOfPortSync(
        std::move(txPacket), PortID(masterLogicalPortIds()[0]));
  }
  bool wasMacLearnt(PortDescriptor portDescr, bool shouldExist = true) const {
    /***
     * shouldExist - if set to true (default), retry until mac is found.
     *             - if set to false, retry until mac is no longer learned
     * @return true if the desired condition occurs before timeout, else false
     */
    int retries = 5;
    while (retries--) {
      auto isTrunk = portDescr.isAggregatePort();
      int portId = isTrunk ? portDescr.aggPortID() : portDescr.phyPortID();
      auto macs = getMacsForPort(getHwSwitch(), portId, isTrunk);
      if (shouldExist == (macs.find(kSourceMac()) != macs.end())) {
        return true;
      }
      // Typically the MAC learning is immediate post a packet sent,
      // but adding a few retries just to avoid test noise
      sleep(1);
    }
    return false;
  }

  void verifyL2TableCallback(
      const std::pair<L2Entry, L2EntryUpdateType>* l2EntryAndUpdateType,
      L2EntryUpdateType expectedL2EntryUpdateType) {
    auto [l2Entry, l2EntryUpdateType] = *l2EntryAndUpdateType;

    EXPECT_EQ(l2Entry.getMac(), kSourceMac());
    EXPECT_EQ(l2Entry.getVlanID(), VlanID(initialConfig().vlanPorts[0].vlanID));
    EXPECT_TRUE(l2Entry.getPortDescriptor().isPhysicalPort());
    EXPECT_EQ(
        l2Entry.getPortDescriptor().phyPortID(), masterLogicalPortIds()[0]);
    EXPECT_EQ(l2EntryUpdateType, expectedL2EntryUpdateType);
  }

  void verifyLearningAndAgingHelper(cfg::L2LearningMode l2LearningMode) {
    constexpr int kMinAgeSecs = 1;
    bool removed = false;

    // sendPkt here instead of setup b/c the last step of the test
    // removes the packet, so we need to reset it with each verify()
    sendPkt();

    // Verify that Mac aging is enabled, that is, >0
    EXPECT_GT(utility::getMacAgeTimerSeconds(getHwSwitch()), 0);

    // If Learning Mode is SOFTWARE, verify if we get callback for learned MAC
    if (l2LearningMode == cfg::L2LearningMode::SOFTWARE) {
      HwTestLearningUpdateObserver learnObserver(getHwSwitchEnsemble());
      verifyL2TableCallback(
          learnObserver.waitForLearningUpdate(),
          L2EntryUpdateType::L2_ENTRY_UPDATE_TYPE_ADD);
    }

    // Verify that we really learned that MAC
    EXPECT_TRUE(
        wasMacLearnt(PortDescriptor(PortID(masterLogicalPortIds()[0]))));

    // Force MAC aging to as fast a possible but min is still 1 second
    utility::setMacAgeTimerSeconds(getHwSwitch(), kMinAgeSecs);

    // If Learning Mode is SOFTWARE, verify if we get callback for aged MAC
    if (l2LearningMode == cfg::L2LearningMode::SOFTWARE) {
      HwTestLearningUpdateObserver ageObserver(getHwSwitchEnsemble());
      verifyL2TableCallback(
          ageObserver.waitForLearningUpdate(),
          L2EntryUpdateType::L2_ENTRY_UPDATE_TYPE_DELETE);
    }

    // Verify the mac has been removed; this call will wait up to
    // seconds before giving up, which is longer than the 2*kMinAge
    // needed
    removed = wasMacLearnt(
        PortDescriptor(PortID(masterLogicalPortIds()[0])),
        /* shouldExist */ false); // return true if mac no longer learned
    EXPECT_TRUE(removed);
  }
};

TEST_F(HwMacLearningTest, TrunkCheckMacsLearned) {
  auto setup = [this]() {
    auto newCfg{initialConfig()};
    // We enabled the port after applying initial config,
    // don't disable it again
    newCfg.ports[0].state = cfg::PortState::ENABLED;
    addAggPort(
        std::numeric_limits<AggregatePortID>::max(),
        {masterLogicalPortIds()[0]},
        &newCfg);
    auto state = applyNewConfig(newCfg);
    applyNewState(enableTrunkPorts(state));
    sendPkt();
  };
  auto verify = [this]() {
    EXPECT_TRUE(wasMacLearnt(PortDescriptor(
        AggregatePortID(std::numeric_limits<AggregatePortID>::max()))));
  };
  setup();
  verify();
}

TEST_F(HwMacLearningTest, PortCheckMacsLearned) {
  auto setup = [this]() { sendPkt(); };
  auto verify = [this]() {
    EXPECT_TRUE(
        wasMacLearnt(PortDescriptor(PortID(masterLogicalPortIds()[0]))));
  };
  // MACs learned should be preserved across warm boot
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwMacLearningTest, MacAging) {
  auto setup = [=]() { /* NOOP */ };
  auto verify = [=]() {
    verifyLearningAndAgingHelper(cfg::L2LearningMode::HARDWARE);
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwMacLearningTest, VerifyL2TableUpdateOnLearningAndAging) {
  /*
   * TODO (skhare)
   * L2 Learning implementation on TH3 is different from TD2 and TH.
   * Discussing this with Broadcom in CS9327819, and once the case is revoled,
   * we would revisit this.
   */
  if (getPlatform()->getAsic()->getAsicType() ==
      HwAsic::AsicType::ASIC_TYPE_TOMAHAWK3) {
    return;
  }

  auto setup = [this] {
    auto newCfg{initialConfig()};
    newCfg.switchSettings.l2LearningMode = cfg::L2LearningMode::SOFTWARE;
    applyNewConfig(newCfg);
  };

  auto verify = [this]() {
    verifyLearningAndAgingHelper(cfg::L2LearningMode::SOFTWARE);
  };

  verifyAcrossWarmBoots(setup, verify);
}

} // namespace fboss
} // namespace facebook
