// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <gtest/gtest.h>
#include <chrono>
#include <optional>

#include <thrift/lib/cpp2/protocol/DebugProtocol.h>

#include <thrift/lib/cpp/util/EnumUtils.h>
#include "fboss/agent/PlatformPort.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/hw/test/HwTestPortUtils.h"
#include "fboss/agent/test/link_tests/LinkTest.h"
#include "fboss/lib/CommonUtils.h"
#include "fboss/lib/phy/gen-cpp2/phy_types.h"
#include "fboss/lib/phy/gen-cpp2/phy_types_custom_protocol.h"
#include "fboss/lib/thrift_service_client/ThriftServiceClient.h"
#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"

using namespace ::testing;
using namespace facebook::fboss;

static constexpr int kSecondsBetweenSnapshots = 20;
static constexpr int kRsFec528CodewordBins = 8;
static constexpr int kRsFec544CodewordBins = 15;

namespace {

void validateInterfaceAndMedium(
    phy::InterfaceType interfaceType,
    TransmitterTechnology medium) {
  static const std::map<TransmitterTechnology, std::set<phy::InterfaceType>>
      kMediumMap = {
          {TransmitterTechnology::COPPER,
           {
               phy::InterfaceType::CR,
               phy::InterfaceType::CR2,
               phy::InterfaceType::CR4,
               phy::InterfaceType::CR8,
               // Seems to only be expected on copper per BcmPortUtils.cpp
               phy::InterfaceType::GMII,
               phy::InterfaceType::SFI, // On fabric ports
               // Needed for Wedge400C
               phy::InterfaceType::KR8,
           }},
          {TransmitterTechnology::OPTICAL,
           {
               phy::InterfaceType::SR,
               phy::InterfaceType::SR4,
               phy::InterfaceType::SR8,
               // Choice for optical profiles in
               // BcmPortUtils::getSpeedToTransmitterTechAndMode
               phy::InterfaceType::CAUI,
               phy::InterfaceType::CAUI4,
               phy::InterfaceType::XLAUI,
               phy::InterfaceType::SFI,
               // appears to be used for 40G optical profiles in minipack
               // (minipack_16q.cinc)
               phy::InterfaceType::XLPPI,
               // Wedge400C prefers KR4 and KR8 for >100G optical profiles
               // for some reason...
               phy::InterfaceType::KR4,
               phy::InterfaceType::KR8,
           }},
          {TransmitterTechnology::BACKPLANE,
           {
               phy::InterfaceType::CR4,
               phy::InterfaceType::KR,
               phy::InterfaceType::KR2,
               phy::InterfaceType::KR4,
               phy::InterfaceType::KR8,
               // Results in backplane medium in
               // BcmPortUtils::getDesiredPhyLaneConfig
               phy::InterfaceType::CAUI4_C2C,
               phy::InterfaceType::CAUI4_C2M,
               phy::InterfaceType::AUI_C2C,
               phy::InterfaceType::AUI_C2M,
               phy::InterfaceType::SR2,
               phy::InterfaceType::SFI, // On fabric ports
           }},
      };

  if (interfaceType == phy::InterfaceType::NONE ||
      medium == TransmitterTechnology::UNKNOWN) {
    return;
  }

  auto validInterfaceTypes = kMediumMap.find(medium);
  CHECK(validInterfaceTypes != kMediumMap.end());
  EXPECT_FALSE(
      validInterfaceTypes->second.find(interfaceType) ==
      validInterfaceTypes->second.end())
      << "InterfaceType " << apache::thrift::util::enumNameSafe(interfaceType)
      << " is not compatible with medium "
      << apache::thrift::util::enumNameSafe(medium);
}

void validatePhyInfo(
    const phy::PhyInfo& prev,
    const phy::PhyInfo& curr,
    phy::DataPlanePhyChipType chipType) {
  SCOPED_TRACE(folly::to<std::string>("port ", *prev.state()->name()));
  SCOPED_TRACE(
      folly::to<std::string>("previous: ", apache::thrift::debugString(prev)));
  SCOPED_TRACE(
      folly::to<std::string>("current: ", apache::thrift::debugString(curr)));

  auto& currState = *curr.state();
  auto& currStats = *curr.stats();
  auto& prevState = *prev.state();
  auto& prevStats = *prev.stats();

  // Assert that a phy info update happened
  EXPECT_TRUE(*currState.timeCollected() > *prevState.timeCollected());
  EXPECT_TRUE(*currStats.timeCollected() > *prevStats.timeCollected());
  EXPECT_EQ(*currState.phyChip()->type(), chipType);
  EXPECT_EQ(*prevState.phyChip()->type(), chipType);
  if (chipType == phy::DataPlanePhyChipType::IPHY) {
    // both current and previous linkState should be true for iphy
    if (auto linkState = prevState.linkState()) {
      EXPECT_TRUE(*linkState);
    } else {
      throw FbossError("linkState of previous iphy info is not set");
    }
    if (auto linkState = currState.linkState()) {
      EXPECT_TRUE(*linkState);
    } else {
      throw FbossError("linkState of current iphy info is not set");
    }
  }

  if (auto prevPcsStats = prevStats.line()->pcs()) {
    // If previous has pcs info, current should also have such info
    EXPECT_TRUE(currStats.line()->pcs());
    if (auto prevRsFecInfo = prevPcsStats->rsFec()) {
      EXPECT_TRUE(currStats.line()->pcs()->rsFec());
      auto currRsFecInfo =
          apache::thrift::can_throw(currStats.line()->pcs()->rsFec());
      // Assert that fec uncorrectable error count didn't increase
      EXPECT_EQ(
          *prevRsFecInfo->uncorrectedCodewords(),
          *currRsFecInfo->uncorrectedCodewords());

      // Assert that fec correctable error count is the same or increased
      EXPECT_LE(
          *prevRsFecInfo->correctedCodewords(),
          *currRsFecInfo->correctedCodewords());
    }
  }

  auto checkSideState = [](const phy::PhySideState& sideState) {
    // PMD checks
    for (const auto& lane : *sideState.pmd()->lanes()) {
      SCOPED_TRACE(folly::to<std::string>("lane ", lane.first));
      if (auto cdrLiveStatus = lane.second.cdrLockLive()) {
        EXPECT_TRUE(*cdrLiveStatus);
      }
      // TODO: Also expect > 0 lanes on platforms that support the pmd apis with
      // sdk >= 6.5.24
    }

    // Check that interfaceType and medium dont conflict
    if (sideState.interfaceType().has_value()) {
      validateInterfaceAndMedium(
          *sideState.interfaceType(), *sideState.medium());
    }
  };

  auto checkSideStats = [](const phy::PhySideStats& sideStats) {
    // PMD checks
    for (const auto& lane : *sideStats.pmd()->lanes()) {
      SCOPED_TRACE(folly::to<std::string>("lane ", lane.first));
      if (const auto eyesRef = lane.second.eyes()) {
        for (const auto& eye : *eyesRef) {
          if (const auto widthRef = eye.width()) {
            EXPECT_GT(*widthRef, 0);
          }
          if (const auto heightRef = eye.height()) {
            EXPECT_GT(*heightRef, 0);
          }
        }
      }
      if (auto snrRef = lane.second.snr()) {
        EXPECT_NE(*snrRef, 0.0);
      }
      // TODO: Also expect > 0 lanes on platforms that support the pmd apis with
      // sdk >= 6.5.24
    }
  };

  {
    SCOPED_TRACE("line side");
    {
      SCOPED_TRACE("previous");
      EXPECT_EQ(prevState.line()->side(), phy::Side::LINE);
      EXPECT_EQ(prevStats.line()->side(), phy::Side::LINE);
      checkSideState(*prevState.line());
      checkSideStats(*prevStats.line());
    }
    {
      SCOPED_TRACE("current");
      EXPECT_EQ(currState.line()->side(), phy::Side::LINE);
      EXPECT_EQ(currStats.line()->side(), phy::Side::LINE);
      checkSideState(*currState.line());
      checkSideStats(*currStats.line());
    }
  }
  if (chipType == phy::DataPlanePhyChipType::XPHY) {
    SCOPED_TRACE("system side");
    if (auto sysState = prevState.system()) {
      checkSideState(*sysState);
      EXPECT_TRUE(currState.system());
      EXPECT_EQ(prevState.system()->side(), phy::Side::SYSTEM);
      EXPECT_EQ(currState.system()->side(), phy::Side::SYSTEM);
      checkSideState(*apache::thrift::can_throw(currState.system()));
    }
    if (auto sysStats = prevStats.system()) {
      checkSideStats(*sysStats);
      EXPECT_TRUE(currStats.system());
      EXPECT_EQ(prevStats.system()->side(), phy::Side::SYSTEM);
      EXPECT_EQ(currStats.system()->side(), phy::Side::SYSTEM);
      checkSideStats(*apache::thrift::can_throw(currStats.system()));
    }
    // TODO: Expect system side info always on XPHY when every XPHY supports
    // publishing phy infos
  }
}

std::optional<phy::PhyInfo> getXphyInfo(PortID portID) {
  try {
    phy::PhyInfo thriftPhyInfo;
    auto qsfpServiceClient = utils::createQsfpServiceClient();
    qsfpServiceClient->sync_getXphyInfo(thriftPhyInfo, portID);
    return thriftPhyInfo;
  } catch (const thrift::FbossBaseError& /* err */) {
    // If there's no phyInfo collected, it will throw fboss error.
    return std::nullopt;
  }
}
} // namespace

TEST_F(LinkTest, iPhyInfoTest) {
  auto cabledPorts = getCabledPorts();
  std::map<PortID, const phy::PhyInfo> phyInfoBefore;
  auto startTime = std::chrono::duration_cast<std::chrono::seconds>(
      std::chrono::system_clock::now().time_since_epoch());
  // Make sure the stats thread had a chance to update the iphy data before
  // starting the test
  WITH_RETRIES_N_TIMED(
      20 /* retries */, std::chrono::milliseconds(1000) /* msBetweenRetry */, {
        phyInfoBefore = sw()->getIPhyInfo(cabledPorts);
        for (const auto& port : cabledPorts) {
          auto phyIt = phyInfoBefore.find(port);
          ASSERT_EVENTUALLY_NE(phyIt, phyInfoBefore.end());
          EXPECT_EVENTUALLY_GT(
              phyIt->second.state()->timeCollected(), startTime.count());
          EXPECT_EVENTUALLY_GT(
              phyIt->second.stats()->timeCollected(), startTime.count());
          EXPECT_EVENTUALLY_TRUE(
              phyIt->second.state()->linkState().value_or({}));
        }
      });

  std::map<PortID, const phy::PhyInfo> phyInfoAfter;
  // Monitor the link for 35 seconds and collect phy stats
  WITH_RETRIES_N_TIMED(
      35 /* retries */, std::chrono::milliseconds(1000) /* msBetweenRetry */, {
        phyInfoAfter = sw()->getIPhyInfo(cabledPorts);
        for (const auto& port : cabledPorts) {
          auto phyIt = phyInfoAfter.find(port);
          ASSERT_EVENTUALLY_NE(phyIt, phyInfoAfter.end());
          EXPECT_EVENTUALLY_GE(
              *(phyInfoAfter[port].state()->timeCollected()) -
                  *(phyInfoBefore[port].state()->timeCollected()),
              20);
          EXPECT_EVENTUALLY_GE(
              *(phyInfoAfter[port].stats()->timeCollected()) -
                  *(phyInfoBefore[port].stats()->timeCollected()),
              20);
        }
      });

  // Validate PhyInfo
  for (const auto& port : cabledPorts) {
    validatePhyInfo(
        phyInfoBefore[port],
        phyInfoAfter[port],
        phy::DataPlanePhyChipType::IPHY);
  }
}

TEST_F(LinkTest, xPhyInfoTest) {
  auto cabledPorts = getCabledPorts();
  std::map<PortID, const phy::PhyInfo> phyInfoBefore;
  auto now = std::chrono::duration_cast<std::chrono::seconds>(
      std::chrono::system_clock::now().time_since_epoch());

  // Give Xphy ports some time to come up
  // sleep override
  std::this_thread::sleep_for(10s);

  // Wait for at least 1 snapshot from every active port before we start
  WITH_RETRIES_N_TIMED(
      kMaxNumXphyInfoCollectionCheck /* retries */,
      kSecondsBetweenXphyInfoCollectionCheck /* retry period */,
      {
        for (const auto& port : cabledPorts) {
          if (phyInfoBefore.count(port)) {
            continue;
          }
          auto phyInfo = getXphyInfo(port);
          // ASSERT_EVENTUALLY will retry the whole block if failed
          ASSERT_EVENTUALLY_TRUE(phyInfo.has_value())
              << getPortName(port) << " has no xphy info.";
          ASSERT_EVENTUALLY_TRUE(
              *phyInfo->state()->timeCollected() > now.count())
              << getPortName(port) << " didn't update phy state";
          ASSERT_EVENTUALLY_TRUE(
              *phyInfo->stats()->timeCollected() > now.count())
              << getPortName(port) << " didn't update phy stats";
          phyInfoBefore.emplace(port, *phyInfo);
        }
      });

  const auto timeStart = std::chrono::steady_clock::now();

  // Wait until every active port have an updated snapshot
  // sleep override
  std::this_thread::sleep_for(std::chrono::seconds(kSecondsBetweenSnapshots));

  std::map<PortID, const phy::PhyInfo> phyInfoAfter;
  WITH_RETRIES_N_TIMED(
      kMaxNumXphyInfoCollectionCheck /* retries */,
      kSecondsBetweenXphyInfoCollectionCheck /* retry period */,
      {
        for (const auto& port : cabledPorts) {
          if (phyInfoAfter.count(port)) {
            continue;
          }
          auto phyInfo = getXphyInfo(port);
          ASSERT_EVENTUALLY_TRUE(phyInfo.has_value())
              << getPortName(port) << " has no xphy info.";
          ASSERT_EVENTUALLY_GE(
              phyInfo->state()->get_timeCollected() -
                  phyInfoBefore[port].state()->get_timeCollected(),
              kSecondsBetweenSnapshots)
              << getPortName(port) << " has no updated xphy state.";
          ASSERT_EVENTUALLY_GE(
              phyInfo->stats()->get_timeCollected() -
                  phyInfoBefore[port].stats()->get_timeCollected(),
              kSecondsBetweenSnapshots)
              << getPortName(port) << " has no updated xphy stats.";
          phyInfoAfter.emplace(port, *phyInfo);
        }
      });

  const auto timeEnd = std::chrono::steady_clock::now();
  const auto timeTaken =
      std::chrono::duration_cast<std::chrono::seconds>(timeEnd - timeStart);
  XLOG(DBG2) << "Updating snapshots on all ports took " << timeTaken.count()
             << "s";

  // Validate PhyInfo
  for (const auto& port : cabledPorts) {
    XLOG(DBG2) << "Verifying port:" << getPortName(port);
    validatePhyInfo(
        phyInfoBefore[port],
        phyInfoAfter[port],
        phy::DataPlanePhyChipType::XPHY);
  }
}

/*
 * Test injects FEC errors on the NPU on one port and expects to see FEC
 * counters increment on the corresponding snaked port
 */
TEST_F(LinkTest, verifyIphyFecCounters) {
  auto cabledPorts = getCabledPorts();
  std::map<PortID, const phy::PhyInfo> phyInfoBefore;
  auto startTime = std::chrono::duration_cast<std::chrono::seconds>(
      std::chrono::system_clock::now().time_since_epoch());
  // Make sure the stats thread had a chance to update the iphy data before
  // starting the test
  WITH_RETRIES_N_TIMED(
      20 /* retries */, std::chrono::milliseconds(1000) /* msBetweenRetry */, {
        phyInfoBefore = sw()->getIPhyInfo(cabledPorts);
        for (const auto& port : cabledPorts) {
          auto phyIt = phyInfoBefore.find(port);
          ASSERT_EVENTUALLY_NE(phyIt, phyInfoBefore.end());
          EXPECT_EVENTUALLY_GT(
              phyIt->second.state()->timeCollected(), startTime.count());
          EXPECT_EVENTUALLY_GT(
              phyIt->second.stats()->timeCollected(), startTime.count());
          EXPECT_EVENTUALLY_TRUE(
              phyIt->second.state()->linkState().value_or({}));
        }
      });

  // Inject FEC errors from one end of the link, expect both FEC CW and UCW to
  // increment on the other end
  auto portPairsForTest = getPortPairsForFecErrInj();
  std::vector<int> hwPortsToInjectFecErrorFrom;
  std::vector<PortID> portsToVerifyFecCountersOn;
  for (const auto& [swPort1, swPort2] : portPairsForTest) {
    auto hwPortID = sw()->getHwLogicalPortId(swPort1);
    CHECK(hwPortID);
    hwPortsToInjectFecErrorFrom.push_back(*hwPortID);
    portsToVerifyFecCountersOn.push_back(swPort2);
  }

  CHECK(!hwPortsToInjectFecErrorFrom.empty());
  CHECK(!portsToVerifyFecCountersOn.empty());
  facebook::fboss::utility::injectFecError(
      hwPortsToInjectFecErrorFrom,
      platform()->getHwSwitch(),
      true /* correctable */);
  facebook::fboss::utility::injectFecError(
      hwPortsToInjectFecErrorFrom,
      platform()->getHwSwitch(),
      false /* correctable */);

  std::map<PortID, const phy::PhyInfo> phyInfoAfter;
  WITH_RETRIES_N_TIMED(
      35 /* retries */, std::chrono::milliseconds(1000) /* msBetweenRetry */, {
        phyInfoAfter = sw()->getIPhyInfo(cabledPorts);
        for (const auto& port : cabledPorts) {
          auto phyIt = phyInfoAfter.find(port);
          ASSERT_EVENTUALLY_NE(phyIt, phyInfoAfter.end());
          EXPECT_EVENTUALLY_GE(
              *(phyInfoAfter[port].stats()->timeCollected()) -
                  *(phyInfoBefore[port].stats()->timeCollected()),
              20);
        }
        for (auto port : portsToVerifyFecCountersOn) {
          XLOG(INFO) << "Verifying port " << port;
          auto fecStatsBefore = phyInfoBefore[port]
                                    .stats()
                                    ->line()
                                    ->pcs()
                                    .value_or({})
                                    .rsFec()
                                    .value_or({});
          auto fecStatsAfter = phyInfoAfter[port]
                                   .stats()
                                   ->line()
                                   ->pcs()
                                   .value_or({})
                                   .rsFec()
                                   .value_or({});
          EXPECT_EVENTUALLY_GT(
              fecStatsAfter.get_correctedBits(),
              fecStatsBefore.get_correctedBits());
          EXPECT_EVENTUALLY_GT(
              fecStatsAfter.get_correctedCodewords(),
              fecStatsBefore.get_correctedCodewords());
          EXPECT_EVENTUALLY_GT(
              fecStatsAfter.get_uncorrectedCodewords(),
              fecStatsBefore.get_uncorrectedCodewords());
        }
      });
}

TEST_F(LinkTest, verifyIphyFecBerCounters) {
  /*
   * Collects 5 (500 for stress test) phyInfos and verifies
   * 1. No uncorrected codewords
   * 2. If there are corrected codewords, expect pre-FEC BER to be non-zero
   * 3. Pre-FEC BER should always be <= e-5 (e-4 is the FEC correction limit)
   * 4. If the hardware supports FEC histogram, we should always have something
   * populated in bin 0. Bin 0 indicates the number of codewords that didn't
   * require correction. Stress tests have a stricter threshold of 5.0e-7
   * 5. If the hardware supports FEC histogram and there are some corrected
   * codewords, we should expect to see some non-zero values in bins >= 1.
   */
  auto iterations = FLAGS_link_stress_test ? 500 : 5;
  auto preFecBerThreshold = FLAGS_link_stress_test ? 5.0e-7 : 1e-5;
  std::map<PortID, const phy::PhyInfo> previousPhyInfo;
  std::map<PortID, const phy::PhyInfo> currentPhyInfo;
  auto cabledPorts = getCabledPorts();
  auto getPhyInfo = [this, &cabledPorts](
                        time_t timeReference,
                        std::map<PortID, const phy::PhyInfo>& phyInfo) {
    WITH_RETRIES_N_TIMED(
        120 /* retries */,
        std::chrono::milliseconds(1000) /* msBetweenRetry */,
        {
          phyInfo = sw()->getIPhyInfo(cabledPorts);
          for (const auto& port : cabledPorts) {
            auto phyIt = phyInfo.find(port);
            ASSERT_EVENTUALLY_NE(phyIt, phyInfo.end());
            // If the time stamp hasn't advanced, no need to check the counters.
            // Thus this is an ASSERT_EVENTUALLY_GT and not EXPECT_EVENTUALLY_GT
            ASSERT_EVENTUALLY_GT(
                *(phyInfo[port].stats()->timeCollected()), timeReference);
          }
        });
  };

  std::time_t timeReference = std::time(nullptr);
  getPhyInfo(timeReference, previousPhyInfo);

  auto verify = [this,
                 iterations,
                 preFecBerThreshold,
                 &previousPhyInfo,
                 &currentPhyInfo,
                 getPhyInfo,
                 &cabledPorts]() {
    for (int i = 1; i <= iterations && !::testing::Test::HasFailure(); i++) {
      XLOG(INFO) << "Starting iteration " << i;
      getPhyInfo(std::time(nullptr), currentPhyInfo);
      for (const auto& port : cabledPorts) {
        auto phyIt = currentPhyInfo.find(port);
        // We always expect the port to be present in the map. So this is a
        // hard assert
        ASSERT_NE(phyIt, currentPhyInfo.end());
        auto pcsStatsBefore = previousPhyInfo[port].stats()->line()->pcs();
        auto pcsStatsNow = currentPhyInfo[port].stats()->line()->pcs();
        if (pcsStatsBefore.has_value() || pcsStatsNow.has_value()) {
          // We always expect pcsStats to be present in both the phyInfos
          // or in none
          ASSERT_TRUE(pcsStatsBefore.has_value());
          ASSERT_TRUE(pcsStatsNow.has_value());
        } else {
          // Port doesn't support PCS stats so no checks required
          continue;
        }
        auto rsFecBefore = pcsStatsBefore->rsFec();
        auto rsFecNow = pcsStatsNow->rsFec();
        if (rsFecBefore.has_value() || rsFecNow.has_value()) {
          // We always expect rsFec to be present in both the phyInfos
          // or in none
          ASSERT_TRUE(rsFecBefore.has_value());
          ASSERT_TRUE(rsFecNow.has_value());
        } else {
          // Port doesn't support FEC stats so no checks required
          continue;
        }

        XLOG(INFO)
            << "Before: RS FEC stats for port " << getPortName(port)
            << apache::thrift::SimpleJSONSerializer::serialize<std::string>(
                   rsFecBefore.value());
        XLOG(INFO)
            << "Now: RS FEC stats for port " << getPortName(port)
            << apache::thrift::SimpleJSONSerializer::serialize<std::string>(
                   rsFecNow.value());

        // Expect no uncorrected codewords
        EXPECT_EQ(
            rsFecNow->get_uncorrectedCodewords(),
            rsFecBefore->get_uncorrectedCodewords());
        // Expect pre-FEC BER to be lower than e-5 (or 5.0e-7 for stress test)
        // TODO: Make the threshold stricter once
        // 1) we start using a SAI version that supports FEC corrected bits.
        // Before 10.2 SAI, ASIC doesn't support FEC corrected bits and we
        // approximate pre-FEC BER using FEC corrected codewords.
        // 2) we cleanup existing bad links in the lab
        EXPECT_LT(rsFecNow->get_preFECBer(), preFecBerThreshold);
        // If there were corrected codewords in the interval, expect pre-FEC
        // BER non-zero
        bool hasCorrectedCodewords = rsFecNow->get_correctedCodewords() !=
            rsFecBefore->get_correctedCodewords();
        if (hasCorrectedCodewords) {
          EXPECT_NE(rsFecNow->get_preFECBer(), 0);
        }
        // If the codewordStats is supported, it should either have 9 keys or
        // 16. For Rs528, there are 8 codeword bins. For Rs544, there are 15.
        // We need to add 1 to the expected codewordStats keys since the first
        // key will be for codewords with 0 corrections
        if (!rsFecNow->get_codewordStats().empty()) {
          EXPECT_TRUE(
              rsFecNow->codewordStats()->size() ==
                  (kRsFec528CodewordBins + 1) ||
              rsFecNow->codewordStats()->size() == (kRsFec544CodewordBins + 1));
          // Expect bin 0 to be always non-zero since there are always bits
          // that didn't need correction.
          EXPECT_TRUE(
              rsFecNow->codewordStats()->find(0) !=
              rsFecNow->codewordStats()->end());

          // If there were corrected codewords in the interval, then expect
          // one of the other bins to be non-zero
          int countNonZeroBins = 0;
          for (auto it : *rsFecNow->codewordStats()) {
            if (it.first != 0 && it.second) {
              countNonZeroBins++;
            }
          }
          if (hasCorrectedCodewords) {
            EXPECT_GT(countNonZeroBins, 0);
          }
        }
      }
      previousPhyInfo = currentPhyInfo;
    }
  };
  verifyAcrossWarmBoots([]() {}, verify);
}
