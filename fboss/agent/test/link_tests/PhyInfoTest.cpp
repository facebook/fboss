// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <gtest/gtest.h>
#include <chrono>
#include <optional>
#include "fboss/agent/PlatformPort.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/test/link_tests/LinkTest.h"
#include "fboss/lib/CommonUtils.h"
#include "fboss/lib/phy/gen-cpp2/phy_types.h"
#include "fboss/lib/thrift_service_client/ThriftServiceClient.h"
#include "fboss/qsfp_service/lib/QsfpCache.h"

using namespace ::testing;
using namespace facebook::fboss;

static constexpr int kSecondsBetweenSnapshots = 20;

namespace {
void validatePhyInfo(
    const phy::PhyInfo& prev,
    const phy::PhyInfo& curr,
    phy::DataPlanePhyChipType chipType) {
  SCOPED_TRACE(folly::to<std::string>("port ", *prev.name()));

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

  // PMD checks
  auto checkPmd = [](phy::PmdInfo pmdInfo) {
    for (const auto& lane : *pmdInfo.lanes()) {
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
  };

  {
    SCOPED_TRACE("line side");
    checkPmd(prev.line()->get_pmd());
    checkPmd(curr.line()->get_pmd());
  }
  if (chipType == phy::DataPlanePhyChipType::XPHY) {
    SCOPED_TRACE("system side");
    if (auto sysInfo = prev.system()) {
      checkPmd(sysInfo->get_pmd());
      EXPECT_TRUE(curr.system());
      checkPmd(apache::thrift::can_throw(curr.system())->get_pmd());
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

  auto phyInfoReady = [&phyInfoBefore, &cabledPorts, this]() {
    phyInfoBefore = sw()->getIPhyInfo(cabledPorts);
    for (const auto& port : cabledPorts) {
      if (phyInfoBefore.find(port) == phyInfoBefore.end() ||
          *(phyInfoBefore[port].timeCollected()) == 0 ||
          !phyInfoBefore[port].linkState().value_or({})) {
        return false;
      }
    }
    return true;
  };
  // Make sure the stats thread had a chance to update the iphy data before
  // starting the test
  checkWithRetry(
      phyInfoReady,
      20 /* retries */,
      std::chrono::milliseconds(1000) /* msBetweenRetry */);

  std::map<PortID, const phy::PhyInfo> phyInfoAfter;
  auto phyInfoUpdated = [&phyInfoBefore, &cabledPorts, &phyInfoAfter, this]() {
    phyInfoAfter = sw()->getIPhyInfo(cabledPorts);
    for (const auto& port : cabledPorts) {
      if (phyInfoAfter.find(port) == phyInfoAfter.end() ||
          *(phyInfoAfter[port].timeCollected()) -
                  *(phyInfoBefore[port].timeCollected()) <
              20) {
        return false;
      }
    }
    return true;
  };
  // Monitor the link for 20 seconds and collect phy stats
  checkWithRetry(
      phyInfoUpdated,
      30 /* retries */,
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

  auto phyInfoReady =
      [&phyInfoBefore, &cabledPorts, this](bool logErrors = false) {
        for (const auto& port : cabledPorts) {
          if (phyInfoBefore.count(port)) {
            continue;
          }
          if (auto phyInfo = getXphyInfo(port)) {
            phyInfoBefore.emplace(port, *phyInfo);
          } else {
            if (logErrors) {
              XLOG(ERR) << getPortName(port) << " has no xphy info.";
            }
            return false;
          }
        }

        return true;
      };

  // Wait for at least 1 snapshot from every active port before we start
  try {
    checkWithRetry(
        phyInfoReady,
        kMaxNumXphyInfoCollectionCheck /* retries */,
        kSecondsBetweenXphyInfoCollectionCheck /* retry period */);
  } catch (...) {
    XLOG(ERR) << "Failed to wait for all ports to have snapshots:";
    phyInfoReady(true);
    throw;
  }

  std::map<PortID, const phy::PhyInfo> phyInfoAfter;
  auto phyInfoUpdated = [&phyInfoBefore, &cabledPorts, &phyInfoAfter, this](
                            bool logErrors = false) {
    for (const auto& port : cabledPorts) {
      if (phyInfoAfter.count(port)) {
        continue;
      }

      if (auto phyInfo = getXphyInfo(port); phyInfo.has_value() &&
          (phyInfo->get_timeCollected() -
               phyInfoBefore[port].get_timeCollected() >=
           kSecondsBetweenSnapshots)) {
        phyInfoAfter.emplace(port, *phyInfo);
      } else {
        if (logErrors) {
          XLOG(ERR) << getPortName(port) << " has no updated xphy info.";
        }
        return false;
      }
    }
    return true;
  };

  const auto timeStart = std::chrono::steady_clock::now();

  // Wait until every active port have an updated snapshot
  // sleep override
  std::this_thread::sleep_for(std::chrono::seconds(kSecondsBetweenSnapshots));

  try {
    checkWithRetry(
        phyInfoUpdated,
        kMaxNumXphyInfoCollectionCheck /* retries */,
        kSecondsBetweenXphyInfoCollectionCheck /* retry period */);
  } catch (...) {
    XLOG(ERR) << "Failed to wait for all ports to have snapshot updates:";
    phyInfoUpdated(true);
    throw;
  }

  const auto timeEnd = std::chrono::steady_clock::now();
  const auto timeTaken =
      std::chrono::duration_cast<std::chrono::seconds>(timeEnd - timeStart);
  XLOG(INFO) << "Updating snapshots on all ports took " << timeTaken.count()
             << "s";

  // Validate PhyInfo
  for (const auto& port : cabledPorts) {
    XLOG(INFO) << "Verifying port:" << getPortName(port);
    validatePhyInfo(
        phyInfoBefore[port],
        phyInfoAfter[port],
        phy::DataPlanePhyChipType::XPHY);
  }
}
