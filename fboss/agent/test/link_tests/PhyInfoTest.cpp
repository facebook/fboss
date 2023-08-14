// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <gtest/gtest.h>
#include <chrono>
#include <optional>

#include <thrift/lib/cpp2/protocol/DebugProtocol.h>

#include <thrift/lib/cpp/util/EnumUtils.h>
#include "fboss/agent/PlatformPort.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/test/link_tests/LinkTest.h"
#include "fboss/lib/CommonUtils.h"
#include "fboss/lib/phy/gen-cpp2/phy_types.h"
#include "fboss/lib/phy/gen-cpp2/phy_types_custom_protocol.h"
#include "fboss/lib/thrift_service_client/ThriftServiceClient.h"
#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"

using namespace ::testing;
using namespace facebook::fboss;

static constexpr int kSecondsBetweenSnapshots = 20;

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
               // Seems to only be expected on copper per BcmPortUtils.cpp
               phy::InterfaceType::GMII,
               phy::InterfaceType::SFI, // On fabric ports
           }},
          {TransmitterTechnology::OPTICAL,
           {
               phy::InterfaceType::SR,
               phy::InterfaceType::SR4,
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
  SCOPED_TRACE(folly::to<std::string>("port ", *prev.name()));
  SCOPED_TRACE(
      folly::to<std::string>("previous: ", apache::thrift::debugString(prev)));
  SCOPED_TRACE(
      folly::to<std::string>("current: ", apache::thrift::debugString(curr)));

  auto& currState = *curr.state();
  auto& currStats = *curr.stats();
  auto& prevState = *prev.state();
  auto& prevStats = *prev.stats();

  // Assert that a phy info update happened
  EXPECT_TRUE(*curr.timeCollected() > *prev.timeCollected());
  EXPECT_TRUE(*currState.timeCollected() > *prevState.timeCollected());
  EXPECT_TRUE(*currStats.timeCollected() > *prevStats.timeCollected());
  CHECK(curr.phyChip().has_value());
  CHECK(prev.phyChip().has_value());
  EXPECT_EQ(*curr.phyChip()->type(), chipType);
  EXPECT_EQ(*prev.phyChip()->type(), chipType);
  EXPECT_EQ(*currState.phyChip()->type(), chipType);
  EXPECT_EQ(*prevState.phyChip()->type(), chipType);
  if (chipType == phy::DataPlanePhyChipType::IPHY) {
    // both current and previous linkState should be true for iphy
    if (auto linkState = prev.linkState()) {
      EXPECT_TRUE(*linkState);
    } else {
      throw FbossError("linkState of previous iphy info is not set");
    }
    if (auto linkState = prevState.linkState()) {
      EXPECT_TRUE(*linkState);
    } else {
      throw FbossError("linkState of previous iphy info is not set");
    }
    if (auto linkState = curr.linkState()) {
      EXPECT_TRUE(*linkState);
    } else {
      throw FbossError("linkState of current iphy info is not set");
    }
    if (auto linkState = currState.linkState()) {
      EXPECT_TRUE(*linkState);
    } else {
      throw FbossError("linkState of current iphy info is not set");
    }
  }

  if (auto prevPcsInfo = prev.line()->pcs()) {
    // If previous has pcs info, current should also have such info
    EXPECT_TRUE(curr.line()->pcs());
    if (auto prevRsFecInfo = prevPcsInfo->rsFec()) {
      EXPECT_TRUE(curr.line()->pcs()->rsFec());
      auto currRsFecInfo =
          apache::thrift::can_throw(curr.line()->pcs()->rsFec());
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

  auto checkSideInfo = [](const phy::PhySideInfo& sideInfo) {
    // PMD checks
    for (const auto& lane : *sideInfo.pmd()->lanes()) {
      SCOPED_TRACE(folly::to<std::string>("lane ", lane.first));
      if (auto cdrLiveStatus = lane.second.cdrLockLive()) {
        EXPECT_TRUE(*cdrLiveStatus);
      }
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

    // Check that interfaceType and medium dont conflict
    if (sideInfo.interfaceType().has_value()) {
      validateInterfaceAndMedium(*sideInfo.interfaceType(), *sideInfo.medium());
    }
  };

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
      EXPECT_EQ(prev.line()->side(), phy::Side::LINE);
      checkSideInfo(*prev.line());
    }
    {
      SCOPED_TRACE("previous");
      EXPECT_EQ(prevState.line()->side(), phy::Side::LINE);
      EXPECT_EQ(prevStats.line()->side(), phy::Side::LINE);
      checkSideState(*prevState.line());
      checkSideStats(*prevStats.line());
    }
    {
      SCOPED_TRACE("current");
      EXPECT_EQ(curr.line()->side(), phy::Side::LINE);
      checkSideInfo(*curr.line());
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
    if (auto sysInfo = prev.system()) {
      checkSideInfo(*sysInfo);
      EXPECT_TRUE(curr.system());
      EXPECT_EQ(prev.system()->side(), phy::Side::SYSTEM);
      EXPECT_EQ(curr.system()->side(), phy::Side::SYSTEM);
      checkSideInfo(*apache::thrift::can_throw(curr.system()));
    }
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
              phyIt->second.timeCollected(), startTime.count());
          EXPECT_EVENTUALLY_TRUE(phyIt->second.linkState().value_or({}));
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
              *(phyInfoAfter[port].timeCollected()) -
                  *(phyInfoBefore[port].timeCollected()),
              20);
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
          ASSERT_EVENTUALLY_TRUE(*phyInfo->timeCollected() > now.count())
              << getPortName(port) << " didn't update phyInfo";
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
              phyInfo->get_timeCollected() -
                  phyInfoBefore[port].get_timeCollected(),
              kSecondsBetweenSnapshots)
              << getPortName(port) << " has no updated xphy info.";
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
