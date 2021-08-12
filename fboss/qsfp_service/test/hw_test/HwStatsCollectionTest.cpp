/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/qsfp_service/StatsPublisher.h"
#include "fboss/qsfp_service/platforms/wedge/WedgeManager.h"
#include "fboss/qsfp_service/test/hw_test/HwExternalPhyPortTest.h"
#include "fboss/qsfp_service/test/hw_test/HwQsfpEnsemble.h"
#include "fboss/qsfp_service/test/hw_test/HwTest.h"

namespace facebook::fboss {

TEST_F(HwTest, publishStats) {
  StatsPublisher publisher(getHwQsfpEnsemble()->getWedgeManager());
  publisher.init();
  publisher.publishStats(nullptr, 0);
  getHwQsfpEnsemble()->getWedgeManager()->publishI2cTransactionStats();
}

class HwXphyPortStatsCollectionTest : public HwExternalPhyPortTest {
 public:
  const std::vector<phy::ExternalPhy::Feature>& neededFeatures()
      const override {
    static const std::vector<phy::ExternalPhy::Feature> kNeededFeatures = {
        phy::ExternalPhy::Feature::PORT_STATS};
    return kNeededFeatures;
  }

  void runTest() {
    const auto& availableXphyPorts = findAvailableXphyPorts();
    auto setup = [this, &availableXphyPorts]() {
      auto* wedgeManager = getHwQsfpEnsemble()->getWedgeManager();
      for (const auto& [port, profile] : availableXphyPorts) {
        // First program the xphy port
        wedgeManager->programXphyPort(port, profile);
      }
    };

    auto verify = [&]() {
      // Change the default xphy port stat collection delay to 1s
      gflags::SetCommandLineOptionWithMode(
          "xphy_port_stat_interval_secs", "0", gflags::SET_FLAGS_DEFAULT);
      getHwQsfpEnsemble()->getWedgeManager()->updateAllXphyPortsStats();
      // Minipack Xphy stats are relatively slow, one port xphy stat can take
      // about 3s. And because all ports from the same pim need to wait for
      // the pim EventBase, even though we can make the stats collection across
      // all 8 pims in parallel, we might still need about 3X16=48 seconds for
      // all ports. Therefore, use 1 min for all stats to be collected.
      auto platformMode =
          getHwQsfpEnsemble()->getWedgeManager()->getPlatformMode();
      if (platformMode == PlatformMode::MINIPACK ||
          platformMode == PlatformMode::FUJI) {
        /* sleep override */
        sleep(60);
      } else {
        /* sleep override */
        sleep(5);
      }
      // Now check the stats collection future job is done.
      for (const auto& [port, _] : availableXphyPorts) {
        EXPECT_TRUE(
            getHwQsfpEnsemble()->getPhyManager()->isXphyStatsCollectionDone(
                port))
            << "port:" << port << " xphy stats collection is not done";
      }
    };
    verifyAcrossWarmBoots(setup, verify);
  }
};

TEST_F(HwXphyPortStatsCollectionTest, checkXphyStatsCollectionDone) {
  runTest();
}
} // namespace facebook::fboss
