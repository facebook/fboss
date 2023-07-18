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

namespace {
int getSleepSeconds(PlatformType platformMode) {
  // Minipack Xphy stats are relatively slow, one port xphy stat can take
  // about 3s. And because all ports from the same pim need to wait for
  // the pim EventBase, even though we can make the stats collection across
  // all 8 pims in parallel, we might still need about 3X16=48 seconds for
  // all ports. Therefore, use 1 min for all stats to be collected.
  int sleepSeconds = 5;
  if (platformMode == PlatformType::PLATFORM_MINIPACK ||
      platformMode == PlatformType::PLATFORM_FUJI) {
    sleepSeconds = 60;
  }
  return sleepSeconds;
}
} // namespace

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
      getHwQsfpEnsemble()->getWedgeManager()->updateAllXphyPortsStats();
      /* sleep override */
      sleep(getSleepSeconds(
          getHwQsfpEnsemble()->getWedgeManager()->getPlatformType()));

      auto counterKeys = fb303::fbData->getCounterKeys();
      // Now check the stats collection future job is done.
      for (const auto& [port, _] : availableXphyPorts) {
        EXPECT_TRUE(
            getHwQsfpEnsemble()->getPhyManager()->isXphyStatsCollectionDone(
                port))
            << "port:" << port << " xphy stats collection is not done";

        // Verify fb303 has the XPHY FEC counters
        auto portName =
            getHwQsfpEnsemble()->getWedgeManager()->getPortNameByPortId(port);
        CHECK(portName.has_value());
        for (const auto side : {"system", "line"}) {
          auto counterPrefix =
              folly::to<std::string>(*portName, ".xphy.", side);
          for (const auto& counterName :
               {folly::to<std::string>(counterPrefix, ".fec_uncorrectable.sum"),
                folly::to<std::string>(
                    counterPrefix, ".fec_correctable.sum")}) {
            EXPECT_TRUE(
                std::find(
                    counterKeys.begin(), counterKeys.end(), counterName) !=
                counterKeys.end())
                << "counter " << counterName << " not present in fb303";
          }
        }
      }
    };
    verifyAcrossWarmBoots(setup, verify);
  }
};

TEST_F(HwXphyPortStatsCollectionTest, checkXphyStatsCollectionDone) {
  runTest();
}

// This is a basic test that doesn't require hardware. It's here in hw_test
// because Fake SDK doesn't support XPHY APIs in BCM SDK yet.
//
// Since this is not a link test, link is not established and all counter
// values are meaningless. We only check struct sanity here.
class HwXphyPortInfoTest : public HwExternalPhyPortTest {
 public:
  const std::vector<phy::ExternalPhy::Feature>& neededFeatures()
      const override {
    static const std::vector<phy::ExternalPhy::Feature> kNeededFeatures = {
        phy::ExternalPhy::Feature::PORT_INFO};
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
      /* sleep override */
      sleep(getSleepSeconds(
          getHwQsfpEnsemble()->getWedgeManager()->getPlatformType()));
      for (const auto& [port, _] : availableXphyPorts) {
        // Assemble our own lane list
        std::vector<LaneID> sysLanes, lineLanes;
        auto portConfig =
            getHwQsfpEnsemble()->getPhyManager()->getHwPhyPortConfig(port);
        for (const auto& item : portConfig.config.system.lanes) {
          sysLanes.push_back(item.first);
        }
        for (const auto& item : portConfig.config.line.lanes) {
          lineLanes.push_back(item.first);
        }

        phy::PhyInfo lastPhyInfo;
        auto portInfo = getHwQsfpEnsemble()
                            ->getPhyManager()
                            ->getExternalPhy(port)
                            ->getPortInfo(sysLanes, lineLanes, lastPhyInfo);

        // Sanity check the info we received
        auto chipInfo = portInfo.phyChip();
        EXPECT_EQ(chipInfo->get_type(), phy::DataPlanePhyChipType::XPHY);
        EXPECT_FALSE(chipInfo->get_name().empty());
        if (auto sysInfo = portInfo.system()) {
          EXPECT_EQ(sysInfo->get_side(), phy::Side::SYSTEM);
          for (auto const& [lane, laneInfo] : sysInfo->get_pmd().get_lanes()) {
            EXPECT_EQ(lane, laneInfo.get_lane());
          }
        }
        if (auto lineInfo = portInfo.line()) {
          EXPECT_EQ(lineInfo->get_side(), phy::Side::LINE);
          for (auto const& [lane, laneInfo] : lineInfo->get_pmd().get_lanes()) {
            EXPECT_EQ(lane, laneInfo.get_lane());
          }
        }
        EXPECT_GT(portInfo.get_timeCollected(), 0);
      }
    };
    verifyAcrossWarmBoots(setup, verify);
  }
};

TEST_F(HwXphyPortInfoTest, getPortInfo) {
  runTest();
}

class HwXphyPrbsStatsCollectionTest : public HwExternalPhyPortTest {
 public:
  const std::vector<phy::ExternalPhy::Feature>& neededFeatures()
      const override {
    static const std::vector<phy::ExternalPhy::Feature> kNeededFeatures = {
        phy::ExternalPhy::Feature::PRBS, phy::ExternalPhy::Feature::PRBS_STATS};
    return kNeededFeatures;
  }

  void runTest(phy::Side side) {
    const auto& availableXphyPorts = findAvailableXphyPorts();
    auto setup = [this, &availableXphyPorts, side]() {
      auto* wedgeManager = getHwQsfpEnsemble()->getWedgeManager();
      for (const auto& [port, profile] : availableXphyPorts) {
        // First program the xphy port
        wedgeManager->programXphyPort(port, profile);
        // Then program this xphy port with a common prbs polynominal
        phy::PortPrbsState prbs;
        prbs.enabled() = true;
        static constexpr auto kCommonPolynominal = 9;
        prbs.polynominal() = kCommonPolynominal;
        wedgeManager->programXphyPortPrbs(port, side, prbs);
      }
    };

    auto verify = [&]() {
      auto platformMode =
          getHwQsfpEnsemble()->getWedgeManager()->getPlatformType();
      int sleepSeconds = getSleepSeconds(platformMode);

      // The first update stats will set the prbs stats to lock and accumulated
      // errors to 0
      getHwQsfpEnsemble()->getWedgeManager()->updateAllXphyPortsStats();
      /* sleep override */
      sleep(sleepSeconds);
      // The second update stats will provide the prbs stats since the first
      // update, which should see some bers
      getHwQsfpEnsemble()->getWedgeManager()->updateAllXphyPortsStats();
      /* sleep override */
      sleep(sleepSeconds);

      // Now check the stats collection future job is done.
      for (const auto& [port, _] : availableXphyPorts) {
        EXPECT_TRUE(
            getHwQsfpEnsemble()->getPhyManager()->isPrbsStatsCollectionDone(
                port))
            << "port:" << port << " prbs stats collection is not done";

        // For Minipack family we need to enable both sides prbs for actually
        // seeing any error bits. Like asic and xphy system side.
        // Since this is only qsfp_hw_test, it won't be able to trigger
        // asic prbs programming without using wedge_agent.
        // Skip the prbs stats check for Minipack family
        if (platformMode == PlatformType::PLATFORM_MINIPACK ||
            platformMode == PlatformType::PLATFORM_FUJI) {
          continue;
        }

        // Also try to get prbs stats
        const auto& prbsStats =
            getHwQsfpEnsemble()->getPhyManager()->getPortPrbsStats(port, side);
        EXPECT_TRUE(!prbsStats.empty());
        const auto& phyPortConfig =
            getHwQsfpEnsemble()->getPhyManager()->getHwPhyPortConfig(port);
        auto modulation =
            (side == phy::Side::SYSTEM
                 ? *phyPortConfig.profile.system.modulation()
                 : *phyPortConfig.profile.line.modulation());
        auto now = std::time(nullptr);
        for (const auto& lanePrbsStats : prbsStats) {
          EXPECT_TRUE(
              now - *lanePrbsStats.timeSinceLastLocked() >= sleepSeconds);
          EXPECT_TRUE(
              now - *lanePrbsStats.timeSinceLastClear() >= sleepSeconds);
          // NRZ on short channels can have 0 errors.
          // Expecting PAM4 to be BER 0 is not reasonable.
          // xphy and optics are both on the pim so the channel is short
          // The system side link from ASIC<->XPHY goes through a connector and
          // that adds loss
          if (modulation == phy::IpModulation::PAM4 &&
              side == phy::Side::SYSTEM) {
            EXPECT_TRUE(*lanePrbsStats.ber() > 0);
          }
          EXPECT_TRUE(*lanePrbsStats.maxBer() >= *lanePrbsStats.ber());
          EXPECT_TRUE(*lanePrbsStats.locked());
          EXPECT_TRUE(*lanePrbsStats.numLossOfLock() == 0);
        }
      }
    };
    verifyAcrossWarmBoots(setup, verify);
  }
};

TEST_F(HwXphyPrbsStatsCollectionTest, getSystemPrbsStats) {
  runTest(phy::Side::SYSTEM);
}
TEST_F(HwXphyPrbsStatsCollectionTest, getLinePrbsStats) {
  runTest(phy::Side::LINE);
}
} // namespace facebook::fboss
