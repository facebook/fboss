/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/qsfp_service/test/hw_test/HwExternalPhyPortTest.h"

#include "fboss/lib/phy/gen-cpp2/phy_types.h"
#include "fboss/qsfp_service/test/hw_test/HwQsfpEnsemble.h"

#include <boost/preprocessor/cat.hpp>
#include <optional>

namespace facebook::fboss {

namespace {
static const std::unordered_map<
    PlatformType,
    std::unordered_map<phy::IpModulation, std::vector<int32_t>>>
    kSupportedPolynominal = {
        {PlatformType::PLATFORM_MINIPACK,
         {{phy::IpModulation::NRZ, {7, 9, 10, 11, 13, 15, 20, 23, 31, 49, 58}},
          {phy::IpModulation::PAM4,
           {7, 9, 10, 11, 13, 15, 20, 23, 31, 49, 58}}}},
        {PlatformType::PLATFORM_YAMP,
         {{{phy::IpModulation::NRZ, {9, 15, 23, 31}},
           {phy::IpModulation::PAM4, {9, 13, 15, 31}}}}}};
}

template <phy::Side Side, phy::IpModulation Modulation>
class HwPortPrbsTest : public HwExternalPhyPortTest {
 public:
  const std::vector<phy::ExternalPhy::Feature>& neededFeatures()
      const override {
    static const std::vector<phy::ExternalPhy::Feature> kNeededFeatures = {
        phy::ExternalPhy::Feature::PRBS, phy::ExternalPhy::Feature::PRBS_STATS};
    return kNeededFeatures;
  }

  std::vector<std::pair<PortID, cfg::PortProfileID>> findAvailableXphyPorts()
      override {
    std::vector<std::pair<PortID, cfg::PortProfileID>> filteredPorts;
    const auto& origAvailablePorts =
        HwExternalPhyPortTest::findAvailableXphyPorts();
    for (const auto& [port, profile] : origAvailablePorts) {
      const auto& expectedPhyPortConfig =
          getHwQsfpEnsemble()->getPhyManager()->getDesiredPhyPortConfig(
              port, profile, std::nullopt);
      auto ipModulation =
          (Side == phy::Side::SYSTEM
               ? *expectedPhyPortConfig.profile.system.modulation()
               : *expectedPhyPortConfig.profile.line.modulation());
      if (ipModulation != Modulation) {
        continue;
      }
      filteredPorts.push_back(std::make_pair(port, profile));
    }
    CHECK(!filteredPorts.empty())
        << "Can't find xphy ports to support features:" << neededFeatureNames();
    return filteredPorts;
  }

 protected:
  void runTest(bool enable) {
    // Find any available xphy port
    const auto& availableXphyPorts = findAvailableXphyPorts();

    auto* wedgeManager = getHwQsfpEnsemble()->getWedgeManager();
    auto platformType = wedgeManager->getPlatformType();
    auto ipModToPolynominalListIt = kSupportedPolynominal.find(platformType);
    if (ipModToPolynominalListIt == kSupportedPolynominal.end()) {
      throw FbossError(
          "Platform:",
          platformType,
          " doesn't have supoorted polynominal list");
    }

    auto polynominalList = ipModToPolynominalListIt->second.find(Modulation);
    if (polynominalList == ipModToPolynominalListIt->second.end()) {
      throw FbossError(
          "IpModulation:",
          apache::thrift::util::enumNameSafe(Modulation),
          " doesn't have supoorted polynominal list");
    }

    // Make sure we assign one poly value to one port
    std::unordered_map<PortID, std::pair<cfg::PortProfileID, int32_t>>
        portToProfileAndPoly;
    if (availableXphyPorts.size() >= polynominalList->second.size()) {
      for (int i = 0; i < polynominalList->second.size(); i++) {
        const auto& portToProfile = availableXphyPorts[i];
        portToProfileAndPoly.emplace(
            portToProfile.first,
            std::make_pair(portToProfile.second, polynominalList->second[i]));
      }
    } else {
      throw FbossError(
          "Not enough available ports(",
          availableXphyPorts.size(),
          ") to support all polynominals(",
          polynominalList->second.size(),
          ")");
    }

    auto setup = [wedgeManager, enable, &portToProfileAndPoly]() {
      for (const auto& [port, profileAndPoly] : portToProfileAndPoly) {
        XLOG(INFO) << "About to set port:" << port << ", profile:"
                   << apache::thrift::util::enumNameSafe(profileAndPoly.first)
                   << ", side:" << apache::thrift::util::enumNameSafe(Side)
                   << ", polynominal=" << profileAndPoly.second
                   << ", enabled=" << (enable ? "true" : "false");
        // Then try to program xphy prbs
        phy::PortPrbsState prbs;
        prbs.enabled() = enable;
        prbs.polynominal() = profileAndPoly.second;
        wedgeManager->programXphyPortPrbs(port, Side, prbs);
      }
    };

    auto verify = [wedgeManager, enable, &portToProfileAndPoly]() {
      auto* phyManager = wedgeManager->getPhyManager();
      phy::PortComponent component =
          (Side == phy::Side::SYSTEM ? phy::PortComponent::GB_SYSTEM
                                     : phy::PortComponent::GB_LINE);
      // Verify all programmed xphy prbs matching with the desired values
      for (const auto& [port, profileAndPoly] : portToProfileAndPoly) {
        const auto& hwPrbs = wedgeManager->getXphyPortPrbs(port, Side);
        EXPECT_EQ(*hwPrbs.enabled(), enable)
            << "Port:" << port << " has undesired prbs enable state";
        EXPECT_EQ(*hwPrbs.polynominal(), profileAndPoly.second)
            << "Port:" << port << " has undesired prbs polynominal";

        // Verify prbs stats collection is enabled or not
        const auto& prbsStats = wedgeManager->getPortPrbsStats(port, component);
        EXPECT_EQ(*prbsStats.portId(), static_cast<int32_t>(port));
        EXPECT_EQ(*prbsStats.component(), component);
        const auto& laneStats = *prbsStats.laneStats();
        if (enable) {
          const auto& actualPortConfig = phyManager->getHwPhyPortConfig(port);
          const auto& lanes = (Side == phy::Side::SYSTEM)
              ? actualPortConfig.config.system.lanes
              : actualPortConfig.config.line.lanes;
          EXPECT_EQ(laneStats.size(), lanes.size());
          int laneStatsIdx = 0;
          for (const auto& laneIt : lanes) {
            EXPECT_EQ(
                laneIt.first, uint32_t(*laneStats[laneStatsIdx++].laneId()));
          }
        } else {
          EXPECT_TRUE(laneStats.empty());
        }
      }
    };
    verifyAcrossWarmBoots(setup, verify);
  }
};

#define TEST_NAME(SIDE, MODULATION, ENABLE) \
  BOOST_PP_CAT(                             \
      BOOST_PP_CAT(HwPortPrbsTest, _),      \
      BOOST_PP_CAT(                         \
          BOOST_PP_CAT(SIDE, _),            \
          BOOST_PP_CAT(BOOST_PP_CAT(MODULATION, _), ENABLE)))

#define TEST_SET_PRBS(SIDE, MODULATION, ENABLE)          \
  struct TEST_NAME(SIDE, MODULATION, ENABLE)             \
      : public HwPortPrbsTest<                           \
            phy::Side::SIDE,                             \
            phy::IpModulation::MODULATION> {};           \
  TEST_F(TEST_NAME(SIDE, MODULATION, ENABLE), SetPrbs) { \
    runTest(ENABLE);                                     \
  }

TEST_SET_PRBS(SYSTEM, NRZ, true);
TEST_SET_PRBS(LINE, NRZ, true);
TEST_SET_PRBS(SYSTEM, NRZ, false);
TEST_SET_PRBS(LINE, NRZ, false);

TEST_SET_PRBS(SYSTEM, PAM4, true);
TEST_SET_PRBS(LINE, PAM4, true);
TEST_SET_PRBS(SYSTEM, PAM4, false);
TEST_SET_PRBS(LINE, PAM4, false);
} // namespace facebook::fboss
