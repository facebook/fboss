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
    auto cfg = utility::onePortPerInterfaceConfig(
        getHwSwitch(),
        masterLogicalPortIds(),
        getAsic()->desiredLoopbackModes());
    return cfg;
  }

 protected:
  void pumpTraffic() {
    std::vector<uint8_t> payload{0x00, 0x01, 0x00, 0x02};
    std::vector<uint8_t> padding(42, 0);
    payload.insert(payload.end(), padding.begin(), padding.end());
    auto pkt = utility::makeEthTxPacket(
        getHwSwitch(),
        utility::firstVlanID(getProgrammedState()),
        getPlatform()->getLocalMac(),
        folly::MacAddress("01:80:C2:00:00:01"),
        ETHERTYPE::ETHERTYPE_EPON,
        payload);
    getHwSwitchEnsemble()->ensureSendPacketOutOfPort(
        std::move(pkt), PortID(masterLogicalInterfacePortIds()[0]));
  }

  void commonSetup(bool enableRxPause, std::vector<PortID>& ports) {
    if (enableRxPause) {
      auto newState = getProgrammedState()->clone();
      auto portMap = newState->getPorts()->modify(&newState);
      for (auto pId : ports) {
        auto port = portMap->getNodeIf(pId)->clone();
        cfg::PortPause pauseCfg;
        *pauseCfg.rx() = true;
        port->setPause(pauseCfg);
        portMap->updateNode(port, scopeResolver().scope(port));
      }
      applyNewState(newState);
    }
  }
  void runTest(bool enableRxPause) {
    auto setup = [=, this]() {
      std::vector<PortID> ports = {PortID(masterLogicalInterfacePortIds()[0])};
      this->commonSetup(enableRxPause, ports);
    };
    auto verify = [=, this]() {
      auto portStatsBefore =
          getLatestPortStats(masterLogicalInterfacePortIds()[0]);
      pumpTraffic();
      auto portStatsAfter =
          getLatestPortStats(masterLogicalInterfacePortIds()[0]);

      /*
       * Certain asics count the pause packets when pause is disabled while
       * some asics don't. Adjust the packet count based on the asic type.
       * ---------------------------------------------------------------------
       * |   Asic     |   Pause      |     In discards Raw   |  In pause     |
       * ---------------------------------------------------------------------
       * |    BCM     |   Enabled    |         1             |      1        |
       * |    BCM     |   Disabled   |         1             |      1        |
       * |    Tajo    |   Enabled    |         1             |      1        |
       * |    Tajo    |   Disabled   |         0             |      0        |
       * ---------------------------------------------------------------------
       */
      auto expectedPktCount = !enableRxPause &&
              (getHwSwitch()->getPlatform()->getAsic()->getAsicType() ==
                   cfg::AsicType::ASIC_TYPE_EBRO ||
               getHwSwitch()->getPlatform()->getAsic()->getAsicType() ==
                   cfg::AsicType::ASIC_TYPE_YUBA)
          ? 0
          : 1;
      auto expectedDiscardsIncrement =
          isSupported(HwAsic::Feature::IN_PAUSE_INCREMENTS_DISCARDS)
          ? expectedPktCount
          : 0;
      EXPECT_EQ(
          expectedDiscardsIncrement,
          *portStatsAfter.inDiscardsRaw_() - *portStatsBefore.inDiscardsRaw_());
      EXPECT_EQ(
          expectedPktCount,
          *portStatsAfter.inPause_() - *portStatsBefore.inPause_());
      EXPECT_EQ(
          0, *portStatsAfter.inDiscards_() - *portStatsBefore.inDiscards_());
    };
    verifyAcrossWarmBoots(setup, verify);
  }
};

class HwInPauseFloodTest : public HwInPauseDiscardsCounterTest {
 private:
  cfg::SwitchConfig initialConfig() const override {
    auto cfg = utility::oneL3IntfNPortConfig(
        getHwSwitch()->getPlatform()->getPlatformMapping(),
        getHwSwitch()->getPlatform()->getAsic(),
        masterLogicalPortIds(),
        getHwSwitch()->getPlatform()->supportsAddRemovePort(),
        getAsic()->desiredLoopbackModes());
    return cfg;
  }

 protected:
  void runFloodTest(bool enableRxPause) {
    auto setup = [=, this]() {
      std::vector<PortID> ports = {
          PortID(masterLogicalInterfacePortIds()[0]),
          PortID(masterLogicalInterfacePortIds()[1])};
      this->commonSetup(enableRxPause, ports);
    };
    auto verify = [=, this]() {
      auto portStatsBefore =
          getLatestPortStats(masterLogicalInterfacePortIds()[1]);
      pumpTraffic();
      auto portStatsAfter =
          getLatestPortStats(masterLogicalInterfacePortIds()[1]);
      XLOG(DBG0) << "Port " << PortID(masterLogicalInterfacePortIds()[1])
                 << ", outBytes, before: " << *portStatsBefore.outBytes_()
                 << ", after: " << *portStatsAfter.outBytes_();
      // Irrespective of the PAUSE enabled status on ports, we expect PAUSE
      // frames to be dropped by the MAC and not flooded.
      EXPECT_EQ(*portStatsAfter.outBytes_(), *portStatsBefore.outBytes_());
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

TEST_F(HwInPauseFloodTest, rxPauseDisabledValidateFlooding) {
  runFloodTest(false);
}

TEST_F(HwInPauseFloodTest, rxPauseEnabledValidateFlooding) {
  runFloodTest(true);
}

} // namespace facebook::fboss
