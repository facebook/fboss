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
#include "fboss/qsfp_service/lib/QsfpCache.h"

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

  // Assert that a phy info update happened
  EXPECT_TRUE(*curr.timeCollected() > *prev.timeCollected());
  EXPECT_EQ(*curr.phyChip()->type(), chipType);
  EXPECT_EQ(*prev.phyChip()->type(), chipType);
  if (chipType == phy::DataPlanePhyChipType::IPHY) {
    // both current and previous linkState should be true for iphy
    if (auto linkState = prev.linkState()) {
      EXPECT_TRUE(*linkState);
    } else {
      throw FbossError("linkState of previous iphy info is not set");
    }
    if (auto linkState = curr.linkState()) {
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

  {
    SCOPED_TRACE("line side");
    {
      SCOPED_TRACE("previous");
      EXPECT_EQ(prev.line()->side(), phy::Side::LINE);
      checkSideInfo(*prev.line());
    }
    {
      SCOPED_TRACE("current");
      EXPECT_EQ(curr.line()->side(), phy::Side::LINE);
      checkSideInfo(*curr.line());
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
  // Make sure the stats thread had a chance to update the iphy data before
  // starting the test
  WITH_RETRIES_N_TIMED(
      {
        phyInfoBefore = sw()->getIPhyInfo(cabledPorts);
        for (const auto& port : cabledPorts) {
          auto phyIt = phyInfoBefore.find(port);
          ASSERT_EVENTUALLY_NE(phyIt, phyInfoBefore.end());
          EXPECT_EVENTUALLY_NE(phyIt->second.timeCollected(), 0);
          EXPECT_EVENTUALLY_NE(phyIt->second.timeCollected(), 0);
          EXPECT_EVENTUALLY_TRUE(phyIt->second.linkState().value_or({}));
        }
      },
      20 /* retries */,
      std::chrono::milliseconds(1000) /* msBetweenRetry */);

  std::map<PortID, const phy::PhyInfo> phyInfoAfter;
  // Monitor the link for 35 seconds and collect phy stats
  WITH_RETRIES_N_TIMED(
      {
        phyInfoAfter = sw()->getIPhyInfo(cabledPorts);
        for (const auto& port : cabledPorts) {
          auto phyIt = phyInfoAfter.find(port);
          ASSERT_EVENTUALLY_NE(phyIt, phyInfoAfter.end());
          EXPECT_EVENTUALLY_GE(
              *(phyInfoAfter[port].timeCollected()) -
                  *(phyInfoBefore[port].timeCollected()),
              20);
        }
      },
      35 /* retries */,
      std::chrono::milliseconds(1000) /* msBetweenRetry */);

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

  // Give Xphy ports some time to come up
  // sleep override
  std::this_thread::sleep_for(10s);

  // Wait for at least 1 snapshot from every active port before we start
  WITH_RETRIES_N_TIMED(
      {
        for (const auto& port : cabledPorts) {
          if (phyInfoBefore.count(port)) {
            continue;
          }
          auto phyInfo = getXphyInfo(port);
          // ASSERT_EVENTUALLY will retry the whole block if failed
          ASSERT_EVENTUALLY_TRUE(phyInfo.has_value())
              << getPortName(port) << " has no xphy info.";
          phyInfoBefore.emplace(port, *phyInfo);
        }
      },
      kMaxNumXphyInfoCollectionCheck /* retries */,
      kSecondsBetweenXphyInfoCollectionCheck /* retry period */);

  const auto timeStart = std::chrono::steady_clock::now();

  // Wait until every active port have an updated snapshot
  // sleep override
  std::this_thread::sleep_for(std::chrono::seconds(kSecondsBetweenSnapshots));

  std::map<PortID, const phy::PhyInfo> phyInfoAfter;
  WITH_RETRIES_N_TIMED(
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
          phyInfoAfter.emplace(port, *phyInfo);
        }
      },
      kMaxNumXphyInfoCollectionCheck /* retries */,
      kSecondsBetweenXphyInfoCollectionCheck /* retry period */);

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
