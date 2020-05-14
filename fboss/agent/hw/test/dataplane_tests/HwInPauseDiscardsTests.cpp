/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/Platform.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwLinkStateDependentTest.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/test/EcmpSetupHelper.h"

#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/PortMap.h"
#include "fboss/agent/state/SwitchState.h"

using folly::IPAddress;
using folly::IPAddressV6;
using std::string;

namespace facebook::fboss {

class HwInPauseDiscardsCounterTest : public HwLinkStateDependentTest {
 private:
  cfg::SwitchConfig initialConfig() const override {
    auto cfg = utility::onePortPerVlanConfig(
        getHwSwitch(), masterLogicalPortIds(), cfg::PortLoopbackMode::MAC);
    return cfg;
  }
  void pumpTraffic() {
    std::vector<uint8_t> payload{0x00, 0x01, 0x00, 0x02};
    std::vector<uint8_t> padding(42, 0);
    payload.insert(payload.end(), padding.begin(), padding.end());
    auto pkt = utility::makeEthTxPacket(
        getHwSwitch(),
        VlanID(1),
        getPlatform()->getLocalMac(),
        folly::MacAddress("01:80:C2:00:00:01"),
        ETHERTYPE::ETHERTYPE_EPON,
        payload);
    getHwSwitchEnsemble()->ensureSendPacketOutOfPort(
        std::move(pkt), PortID(masterLogicalPortIds()[0]));
  }

 protected:
  void runTest(bool enableRxPause) {
    auto setup = [=]() {
      if (enableRxPause) {
        auto newState = getProgrammedState()->clone();
        auto portMap = newState->getPorts()->modify(&newState);
        auto port =
            portMap->getPort(PortID(masterLogicalPortIds()[0]))->clone();
        cfg::PortPause pauseCfg;
        *pauseCfg.rx_ref() = true;
        port->setPause(pauseCfg);
        portMap->updatePort(port);
        applyNewState(newState);
      }
    };
    auto verify = [=]() {
      auto portStatsBefore = getLatestPortStats(masterLogicalPortIds()[0]);
      pumpTraffic();
      auto portStatsAfter = getLatestPortStats(masterLogicalPortIds()[0]);

      EXPECT_EQ(
          1,
          *portStatsAfter.inDiscardsRaw__ref() -
              *portStatsBefore.inDiscardsRaw__ref());
      EXPECT_EQ(
          1, *portStatsAfter.inPause__ref() - *portStatsBefore.inPause__ref());
      EXPECT_EQ(
          0,
          *portStatsAfter.inDiscards__ref() -
              *portStatsBefore.inDiscards__ref());
    };
    verifyAcrossWarmBoots(setup, verify);
  }
};

/*
 * The chip behavior is that as long as MMU is lossy, in pause frames count as
 * discards, even though with RX pause enabled the chip does throttle traffic on
 * receiving a pause frame. This was very surprising behavior, hence the test
 * for both cases - rx pause enabled vs not. For our use case we always have
 * chip mode set to lossy
 * */

TEST_F(HwInPauseDiscardsCounterTest, rxPauseDisabled) {
  runTest(false);
}

TEST_F(HwInPauseDiscardsCounterTest, rxPauseEnabled) {
  runTest(true);
}

} // namespace facebook::fboss
