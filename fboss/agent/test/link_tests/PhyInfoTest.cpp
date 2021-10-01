// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <gtest/gtest.h>
#include "fboss/agent/PlatformPort.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/test/link_tests/LinkTest.h"
#include "fboss/lib/CommonUtils.h"
#include "fboss/lib/phy/gen-cpp2/phy_types.h"
#include "fboss/qsfp_service/lib/QsfpCache.h"

using namespace ::testing;
using namespace facebook::fboss;

namespace {
void validatePhyInfo(
    const phy::PhyInfo& prev,
    const phy::PhyInfo& curr,
    phy::DataPlanePhyChipType chipType) {
  // Assert that a phy info update happened
  EXPECT_TRUE(*curr.timeCollected_ref() > *prev.timeCollected_ref());
  EXPECT_EQ(*curr.phyChip_ref()->type_ref(), chipType);
  EXPECT_EQ(*prev.phyChip_ref()->type_ref(), chipType);
  EXPECT_EQ(curr.linkState_ref().value_or({}), true);
  EXPECT_EQ(prev.linkState_ref().value_or({}), true);
  // Assert that fec uncorrectable error count didn't increase
  auto prevRsFecInfo =
      prev.line_ref()->pcs_ref().value_or({}).rsFec_ref().value_or({});
  auto currRsFecInfo =
      curr.line_ref()->pcs_ref().value_or({}).rsFec_ref().value_or({});
  // Expect 0 Uncorrected codewords and >=0 corrected codewords count
  EXPECT_TRUE(
      *prevRsFecInfo.uncorrectedCodewords_ref() ==
      *currRsFecInfo.uncorrectedCodewords_ref());
  EXPECT_TRUE(
      *prevRsFecInfo.correctedCodewords_ref() <=
      *currRsFecInfo.correctedCodewords_ref());
}
} // namespace

TEST_F(LinkTest, iPhyInfoTest) {
  auto cabledPorts = getCabledPorts();
  std::map<PortID, const phy::PhyInfo> phyInfoBefore;

  auto phyInfoReady = [&phyInfoBefore, &cabledPorts, this]() {
    phyInfoBefore = sw()->getIPhyInfo(cabledPorts);
    for (const auto& port : cabledPorts) {
      if (phyInfoBefore.find(port) == phyInfoBefore.end() ||
          *(phyInfoBefore[port].timeCollected_ref()) == 0 ||
          !phyInfoBefore[port].linkState_ref().value_or({})) {
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
          *(phyInfoAfter[port].timeCollected_ref()) -
                  *(phyInfoBefore[port].timeCollected_ref()) <
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
