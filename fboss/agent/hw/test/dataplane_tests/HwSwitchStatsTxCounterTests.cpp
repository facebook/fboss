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

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/HwSwitchFb303Stats.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/types.h"

#include <folly/IPAddress.h>

namespace facebook::fboss {

class HwSwitchStatsTxCounterTest : public HwTest {
 protected:
  cfg::SwitchConfig initialConfig() const {
    return utility::oneL3IntfConfig(
        getHwSwitch(),
        masterLogicalPortIds()[0],
        getAsic()->desiredLoopbackMode());
  }
  void checkTxCounters();
};

void HwSwitchStatsTxCounterTest::checkTxCounters() {
  auto setup = [=]() { applyNewConfig(initialConfig()); };
  auto verify = [=]() {
    const PortID port = PortID(masterLogicalPortIds()[0]);
    bool isOutOfPort = true;
    auto hwSwitch = getHwSwitch();
    auto vlanId = utility::firstVlanID(initialConfig());
    auto intfMac = utility::getFirstInterfaceMac(getProgrammedState());
    for (int i = 0; i < 2; i++) {
      isOutOfPort = !isOutOfPort;
      uint32_t txAllocCountBefore =
          hwSwitch->getSwitchStats()->getTxPktAllocCount();
      auto pkt = utility::makeUDPTxPacket(
          hwSwitch,
          vlanId,
          intfMac,
          intfMac,
          folly::IPAddressV6("2620:0:1cfe:face:b00c::3"),
          folly::IPAddressV6("2620:0:1cfe:face:b00c::4"),
          8000,
          8001);

      uint32_t txSentCountBefore = hwSwitch->getSwitchStats()->getTxSentCount();
      if (isOutOfPort) {
        getHwSwitchEnsemble()->ensureSendPacketOutOfPort(std::move(pkt), port);
      } else {
        getHwSwitchEnsemble()->ensureSendPacketSwitched(std::move(pkt));
      }
      uint32_t txAllocCountAfter =
          hwSwitch->getSwitchStats()->getTxPktAllocCount();
      uint32_t txSentCountAfter = hwSwitch->getSwitchStats()->getTxSentCount();
      // verify that counters actually increased
      EXPECT_EQ(1, txAllocCountAfter - txAllocCountBefore);
      EXPECT_EQ(1, txSentCountAfter - txSentCountBefore);
    }
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwSwitchStatsTxCounterTest, SendPacket) {
  checkTxCounters();
}

} // namespace facebook::fboss
