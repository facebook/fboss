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
           {phy::IpModulation::PAM4, {9, 13, 15, 31}}}}},
        {PlatformType::PLATFORM_FUJI,
         {{{phy::IpModulation::NRZ, {31}}, {phy::IpModulation::PAM4, {31}}}}},
};

const std::vector<phy::ExternalPhy::Feature>& getPrbsNeededFeatures() {
  static const std::vector<phy::ExternalPhy::Feature> kNeededFeatures = {
      phy::ExternalPhy::Feature::PRBS, phy::ExternalPhy::Feature::PRBS_STATS};
  return kNeededFeatures;
}

std::vector<std::pair<PortID, cfg::PortProfileID>> filterXphyPortsByModulation(
    const std::vector<std::pair<PortID, cfg::PortProfileID>>& availablePorts,
    HwQsfpEnsemble* ensemble,
    phy::Side side,
    phy::IpModulation modulation) {
  std::vector<std::pair<PortID, cfg::PortProfileID>> filteredPorts;
  for (const auto& [port, profile] : availablePorts) {
    const auto& expectedPhyPortConfig =
        ensemble->getPhyManager()->getDesiredPhyPortConfig(
            port, profile, std::nullopt);
    auto ipModulation =
        (side == phy::Side::SYSTEM
             ? *expectedPhyPortConfig.profile.system.modulation()
             : *expectedPhyPortConfig.profile.line.modulation());
    if (ipModulation != modulation) {
      continue;
    }
    filteredPorts.emplace_back(port, profile);
  }
  return filteredPorts;
}

// Configuration struct for PRBS tests
struct PrbsTestConfig {
  phy::Side side;
  phy::IpModulation modulation;
  bool enable;
};

void setupPrbsOnPorts(
    HwQsfpEnsemble* ensemble,
    const PrbsTestConfig& config,
    const std::unordered_map<PortID, std::pair<cfg::PortProfileID, int32_t>>&
        portToProfileAndPoly) {
  auto qsfpServiceHandler = ensemble->getQsfpServiceHandler();
  for (const auto& [port, profileAndPoly] : portToProfileAndPoly) {
    XLOG(INFO) << "About to set port:" << port << ", profile:"
               << apache::thrift::util::enumNameSafe(profileAndPoly.first)
               << ", side:" << apache::thrift::util::enumNameSafe(config.side)
               << ", modulation:"
               << apache::thrift::util::enumNameSafe(config.modulation)
               << ", polynominal=" << profileAndPoly.second
               << ", enabled=" << (config.enable ? "true" : "false");
    phy::PortPrbsState prbs;
    prbs.enabled() = config.enable;
    prbs.polynominal() = profileAndPoly.second;
    qsfpServiceHandler->programXphyPortPrbs(port, config.side, prbs);
  }
}

void verifyPrbsOnPorts(
    HwQsfpEnsemble* ensemble,
    const PrbsTestConfig& config,
    const std::unordered_map<PortID, std::pair<cfg::PortProfileID, int32_t>>&
        portToProfileAndPoly) {
  auto qsfpServiceHandler = ensemble->getQsfpServiceHandler();
  auto phyManager = ensemble->getPhyManager();
  phy::PortComponent component =
      (config.side == phy::Side::SYSTEM ? phy::PortComponent::GB_SYSTEM
                                        : phy::PortComponent::GB_LINE);
  for (const auto& [port, profileAndPoly] : portToProfileAndPoly) {
    const auto& hwPrbs = qsfpServiceHandler->getXphyPortPrbs(port, config.side);
    EXPECT_EQ(*hwPrbs.enabled(), config.enable)
        << "Port:" << port << " has undesired prbs enable state for side:"
        << apache::thrift::util::enumNameSafe(config.side);
    EXPECT_EQ(*hwPrbs.polynominal(), profileAndPoly.second)
        << "Port:" << port << " has undesired prbs polynominal for side:"
        << apache::thrift::util::enumNameSafe(config.side);

    phy::PrbsStats prbsStats;
    qsfpServiceHandler->getPortPrbsStats(prbsStats, port, component);
    EXPECT_EQ(*prbsStats.portId(), static_cast<int32_t>(port));
    EXPECT_EQ(*prbsStats.component(), component);
    const auto& laneStats = *prbsStats.laneStats();
    if (config.enable) {
      const auto& actualPortConfig = phyManager->getHwPhyPortConfig(port);
      const auto& lanes = (config.side == phy::Side::SYSTEM)
          ? actualPortConfig.config.system.lanes
          : actualPortConfig.config.line.lanes;
      EXPECT_EQ(laneStats.size(), lanes.size());
      int laneStatsIdx = 0;
      for (const auto& laneIt : lanes) {
        EXPECT_EQ(laneIt.first, uint32_t(*laneStats[laneStatsIdx++].laneId()));
      }
    } else {
      EXPECT_TRUE(laneStats.empty());
    }
  }
}

std::unordered_map<PortID, std::pair<cfg::PortProfileID, int32_t>>
buildPortToProfileAndPoly(
    PlatformType platformType,
    phy::IpModulation modulation,
    const std::vector<std::pair<PortID, cfg::PortProfileID>>&
        availableXphyPorts) {
  std::unordered_map<PortID, std::pair<cfg::PortProfileID, int32_t>>
      portToProfileAndPoly;

  auto ipModToPolynominalListIt = kSupportedPolynominal.find(platformType);
  if (ipModToPolynominalListIt == kSupportedPolynominal.end()) {
    throw FbossError(
        "Platform:",
        apache::thrift::util::enumNameSafe(platformType),
        " doesn't have supported polynominal list");
  }

  auto polynominalList = ipModToPolynominalListIt->second.find(modulation);
  if (polynominalList == ipModToPolynominalListIt->second.end()) {
    throw FbossError(
        "IpModulation:",
        apache::thrift::util::enumNameSafe(modulation),
        " doesn't have supported polynominal list");
  }

  // Make sure we assign one poly value to one port
  if (availableXphyPorts.size() >= polynominalList->second.size()) {
    for (size_t i = 0; i < polynominalList->second.size(); i++) {
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

  return portToProfileAndPoly;
}
} // namespace

template <phy::Side Side, phy::IpModulation Modulation>
class HwPortPrbsTest : public HwExternalPhyPortTest {
 public:
  const std::vector<phy::ExternalPhy::Feature>& neededFeatures()
      const override {
    return getPrbsNeededFeatures();
  }

  std::vector<std::pair<PortID, cfg::PortProfileID>> findAvailableXphyPorts()
      override {
    const auto& origAvailablePorts =
        HwExternalPhyPortTest::findAvailableXphyPorts();
    auto filteredPorts = filterXphyPortsByModulation(
        origAvailablePorts, getHwQsfpEnsemble(), Side, Modulation);
    CHECK(!filteredPorts.empty())
        << "Can't find xphy ports to support features:" << neededFeatureNames();
    return filteredPorts;
  }

  std::vector<qsfp_production_features::QsfpProductionFeature>
  getProductionFeatures() const override {
    std::vector<qsfp_production_features::QsfpProductionFeature> featureVector =
        HwExternalPhyPortTest::getProductionFeatures();
    if (Side == phy::Side::SYSTEM && Modulation == phy::IpModulation::NRZ) {
      featureVector.push_back(
          qsfp_production_features::QsfpProductionFeature::
              XPHY_SYSTEM_NRZ_PROFILE);
    } else if (
        Side == phy::Side::SYSTEM && Modulation == phy::IpModulation::PAM4) {
      featureVector.push_back(
          qsfp_production_features::QsfpProductionFeature::
              XPHY_SYSTEM_PAM4_PROFILE);
    } else if (
        Side == phy::Side::LINE && Modulation == phy::IpModulation::NRZ) {
      featureVector.push_back(
          qsfp_production_features::QsfpProductionFeature::
              XPHY_LINE_NRZ_PROFILE);
    } else if (
        Side == phy::Side::LINE && Modulation == phy::IpModulation::PAM4) {
      featureVector.push_back(
          qsfp_production_features::QsfpProductionFeature::
              XPHY_LINE_PAM4_PROFILE);
    } else {
      CHECK(false) << "Side and Modulation not specified correctly ("
                   << apache::thrift::util::enumNameSafe(Side) << ","
                   << apache::thrift::util::enumNameSafe(Modulation) << ")";
    }

    return featureVector;
  }

 protected:
  void runTest(bool enable) {
    const auto& availableXphyPorts = findAvailableXphyPorts();

    auto* wedgeManager = getHwQsfpEnsemble()->getWedgeManager();
    auto platformType = wedgeManager->getPlatformType();

    auto portToProfileAndPoly =
        buildPortToProfileAndPoly(platformType, Modulation, availableXphyPorts);

    PrbsTestConfig config{Side, Modulation, enable};
    auto setup = [this, &config, &portToProfileAndPoly]() {
      setupPrbsOnPorts(getHwQsfpEnsemble(), config, portToProfileAndPoly);
    };

    auto verify = [this, &config, &portToProfileAndPoly]() {
      verifyPrbsOnPorts(getHwQsfpEnsemble(), config, portToProfileAndPoly);
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

/*
 * HwPortPrbsTestAll - Runs all PRBS test combinations in a single iteration
 * This class tests all combinations of:
 *   - Side: SYSTEM, LINE
 *   - Modulation: NRZ, PAM4
 *   - Enable: true, false
 */
class HwPortPrbsTestAll : public HwExternalPhyPortTest {
 public:
  const std::vector<phy::ExternalPhy::Feature>& neededFeatures()
      const override {
    return getPrbsNeededFeatures();
  }

  std::vector<qsfp_production_features::QsfpProductionFeature>
  getProductionFeatures() const override {
    std::vector<qsfp_production_features::QsfpProductionFeature> featureVector =
        HwExternalPhyPortTest::getProductionFeatures();
    // Add all PRBS profile features since we test all combinations
    featureVector.push_back(
        qsfp_production_features::QsfpProductionFeature::
            XPHY_SYSTEM_NRZ_PROFILE);
    featureVector.push_back(
        qsfp_production_features::QsfpProductionFeature::
            XPHY_SYSTEM_PAM4_PROFILE);
    featureVector.push_back(
        qsfp_production_features::QsfpProductionFeature::XPHY_LINE_NRZ_PROFILE);
    featureVector.push_back(
        qsfp_production_features::QsfpProductionFeature::
            XPHY_LINE_PAM4_PROFILE);
    return featureVector;
  }

 protected:
  std::vector<std::pair<PortID, cfg::PortProfileID>> findAvailableXphyPortsFor(
      phy::Side side,
      phy::IpModulation modulation) {
    const auto& origAvailablePorts =
        HwExternalPhyPortTest::findAvailableXphyPorts();
    return filterXphyPortsByModulation(
        origAvailablePorts, getHwQsfpEnsemble(), side, modulation);
  }

  void setupAndVerify(const PrbsTestConfig& config) {
    XLOG(INFO) << "Running PRBS test for side:"
               << apache::thrift::util::enumNameSafe(config.side)
               << ", modulation:"
               << apache::thrift::util::enumNameSafe(config.modulation)
               << ", enable:" << (config.enable ? "true" : "false");

    const auto& availableXphyPorts =
        findAvailableXphyPortsFor(config.side, config.modulation);
    if (availableXphyPorts.empty()) {
      XLOG(INFO) << "No available xphy ports for side:"
                 << apache::thrift::util::enumNameSafe(config.side)
                 << ", modulation:"
                 << apache::thrift::util::enumNameSafe(config.modulation);
      return;
    }

    auto* wedgeManager = getHwQsfpEnsemble()->getWedgeManager();
    auto platformType = wedgeManager->getPlatformType();
    auto portToProfileAndPoly = buildPortToProfileAndPoly(
        platformType, config.modulation, availableXphyPorts);

    setupPrbsOnPorts(getHwQsfpEnsemble(), config, portToProfileAndPoly);
    verifyPrbsOnPorts(getHwQsfpEnsemble(), config, portToProfileAndPoly);
  }

  void verifyAcrossWarmBootsWithIterations(
      const std::vector<PrbsTestConfig>& configs) {
    auto* wedgeManager = getHwQsfpEnsemble()->getWedgeManager();
    auto platformType = wedgeManager->getPlatformType();

    if (!didWarmBoot()) {
      for (const auto& config : configs) {
        XLOG(INFO) << "STAGE: cold boot setup() for side:"
                   << apache::thrift::util::enumNameSafe(config.side)
                   << ", modulation:"
                   << apache::thrift::util::enumNameSafe(config.modulation);

        const auto& availableXphyPorts =
            findAvailableXphyPortsFor(config.side, config.modulation);
        if (availableXphyPorts.empty()) {
          continue;
        }
        auto portToProfileAndPoly = buildPortToProfileAndPoly(
            platformType, config.modulation, availableXphyPorts);

        setupPrbsOnPorts(getHwQsfpEnsemble(), config, portToProfileAndPoly);

        XLOG(INFO) << "STAGE: cold boot verify() for side:"
                   << apache::thrift::util::enumNameSafe(config.side)
                   << ", modulation:"
                   << apache::thrift::util::enumNameSafe(config.modulation);
        verifyPrbsOnPorts(getHwQsfpEnsemble(), config, portToProfileAndPoly);
      }
    }

    if (FLAGS_setup_for_warmboot) {
      XLOG(INFO) << "STAGE: setupForWarmboot";
      getHwQsfpEnsemble()->setupForWarmboot();
    }

    if (didWarmBoot()) {
      for (const auto& config : configs) {
        XLOG(INFO) << "STAGE: warm boot verify() for side:"
                   << apache::thrift::util::enumNameSafe(config.side)
                   << ", modulation:"
                   << apache::thrift::util::enumNameSafe(config.modulation);

        const auto& availableXphyPorts =
            findAvailableXphyPortsFor(config.side, config.modulation);
        if (availableXphyPorts.empty()) {
          continue;
        }
        auto portToProfileAndPoly = buildPortToProfileAndPoly(
            platformType, config.modulation, availableXphyPorts);

        verifyPrbsOnPorts(getHwQsfpEnsemble(), config, portToProfileAndPoly);
      }
    }
  }

  void runAllTests() {
    const std::vector<PrbsTestConfig> allConfigs = {
        {phy::Side::SYSTEM, phy::IpModulation::NRZ, true},
        {phy::Side::LINE, phy::IpModulation::NRZ, true},
        {phy::Side::SYSTEM, phy::IpModulation::NRZ, false},
        {phy::Side::LINE, phy::IpModulation::NRZ, false},
        {phy::Side::SYSTEM, phy::IpModulation::PAM4, true},
        {phy::Side::LINE, phy::IpModulation::PAM4, true},
        {phy::Side::SYSTEM, phy::IpModulation::PAM4, false},
        {phy::Side::LINE, phy::IpModulation::PAM4, false},
    };

    // Run all 8 configurations with setupAndVerify (no warm boot)
    if (!didWarmBoot()) {
      for (const auto& config : allConfigs) {
        setupAndVerify(config);
      }
    }

    // Warm boot verification sequence with specific configurations:
    // Cold boot: Setup/Verify {SYSTEM, NRZ, true}, Setup/Verify {LINE, PAM4,
    // true} setupForWarmboot() Warm boot: Verify {SYSTEM, NRZ, true}, Verify
    // {LINE, PAM4, true}
    const std::vector<PrbsTestConfig> warmBootConfigs = {
        {phy::Side::SYSTEM, phy::IpModulation::NRZ, true},
        {phy::Side::LINE, phy::IpModulation::PAM4, true},
    };

    verifyAcrossWarmBootsWithIterations(warmBootConfigs);
  }
};

TEST_F(HwPortPrbsTestAll, TestAll) {
  runAllTests();
}

} // namespace facebook::fboss
