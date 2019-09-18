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

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/test/HwLinkStateDependentTest.h"
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

} // namespace fboss
} // namespace facebook
