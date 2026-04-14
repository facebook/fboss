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

#include "fboss/lib/phy/PhyManager.h"
#include "fboss/lib/phy/SaiPhyManager.h"
#include "fboss/qsfp_service/test/hw_test/HwPortUtils.h"
#include "fboss/qsfp_service/test/hw_test/HwQsfpEnsemble.h"

#include <folly/logging/xlog.h>

namespace facebook::fboss {

class HwXphyPortReprogramTest : public HwExternalPhyPortTest {
 public:
  const std::vector<phy::ExternalPhy::Feature>& neededFeatures()
      const override {
    static const std::vector<phy::ExternalPhy::Feature> kNeeded = {};
    return kNeeded;
  }
};

/*
 * Repeatedly remove and re-create all XPHY ports (SAI objects) for 3
 * iterations, verifying correct teardown and reprogramming each cycle.
 */
TEST_F(HwXphyPortReprogramTest, ProgramRemoveReprogramCycle) {
  auto verify = [this]() {
    auto xphyPorts = findAvailableXphyPorts();
    ASSERT_FALSE(xphyPorts.empty())
        << "No available XPHY ports found for testing";

    auto* phyManager = getHwQsfpEnsemble()->getPhyManager();
    auto* saiPhyManager = dynamic_cast<SaiPhyManager*>(phyManager);
    ASSERT_NE(saiPhyManager, nullptr)
        << "PhyManager is not a SaiPhyManager, cannot run this test";

    constexpr int kNumIterations = 3;

    // Ports are already programmed by the test setup
    // (waitTillCabledTcvrProgrammed). Verify they are programmed before
    // starting the remove/re-add cycles.
    XLOG(INFO) << "Verifying " << xphyPorts.size()
               << " XPHY ports are programmed by setup";
    for (const auto& [portId, profile] : xphyPorts) {
      EXPECT_TRUE(phyManager->getProgrammedProfile(portId).has_value())
          << "Port " << portId << " should already be programmed by setup";
      utility::verifyXphyPort(
          portId, profile, std::nullopt, getHwQsfpEnsemble());
    }

    for (int iter = 0; iter < kNumIterations; iter++) {
      XLOG(INFO) << "=== Iteration " << (iter + 1) << "/" << kNumIterations
                 << " ===";

      // Phase 1: Remove all XPHY ports (true SAI object deletion)
      XLOG(INFO) << "Phase 1: Removing " << xphyPorts.size() << " XPHY ports";
      for (const auto& [portId, profile] : xphyPorts) {
        EXPECT_NO_THROW(saiPhyManager->removeOnePort(portId));
      }

      // Phase 2: Verify removal (cache should be cleared)
      XLOG(INFO) << "Phase 2: Verifying ports are removed";
      for (const auto& [portId, profile] : xphyPorts) {
        EXPECT_FALSE(phyManager->getProgrammedProfile(portId).has_value())
            << "Port " << portId << " profile should be cleared after removal";
        EXPECT_FALSE(phyManager->getProgrammedSpeed(portId).has_value())
            << "Port " << portId << " speed should be cleared after removal";
      }

      // Phase 3: Re-program all XPHY ports from scratch
      XLOG(INFO) << "Phase 3: Re-programming " << xphyPorts.size()
                 << " XPHY ports";
      for (const auto& [portId, profile] : xphyPorts) {
        EXPECT_NO_THROW(saiPhyManager->programOnePort(
            portId, profile, std::nullopt, /*needResetDataPath=*/false));
      }

      // Phase 4: Verify all ports are programmed correctly
      XLOG(INFO) << "Phase 4: Verifying re-programmed ports";
      for (const auto& [portId, profile] : xphyPorts) {
        EXPECT_TRUE(phyManager->getProgrammedProfile(portId).has_value())
            << "Port " << portId << " should have a programmed profile";
        utility::verifyXphyPort(
            portId, profile, std::nullopt, getHwQsfpEnsemble());
      }
    }
  };

  verifyAcrossWarmBoots([]() {}, verify);
}

} // namespace facebook::fboss
