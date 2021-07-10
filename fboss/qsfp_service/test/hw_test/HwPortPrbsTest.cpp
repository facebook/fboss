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

namespace facebook::fboss {

template <phy::Side Side, int32_t Polynominal>
class HwPortPrbsTest : public HwExternalPhyPortTest {
 public:
  const std::vector<phy::ExternalPhy::Feature>& neededFeatures()
      const override {
    static const std::vector<phy::ExternalPhy::Feature> kNeededFeatures = {
        phy::ExternalPhy::Feature::PRBS};
    return kNeededFeatures;
  }

 protected:
  void runTest(bool enable) {
    auto* wedgeManager = getHwQsfpEnsemble()->getWedgeManager();
    const auto& availableXphyPorts = findAvailableXphyPorts();
    // Find any available xphy port
    for (const auto& [port, profile] : availableXphyPorts) {
      // First program the xphy port
      wedgeManager->programXphyPort(port, profile);

      // Then try to program xphy prbs
      phy::PortPrbsState prbs;
      prbs.enabled_ref() = enable;
      prbs.polynominal_ref() = Polynominal;
      wedgeManager->programXphyPortPrbs(port, Side, prbs);
    }

    // Verify all programmed xphy prbs matching with the desired values
    for (const auto& [port, _] : availableXphyPorts) {
      const auto& hwPrbs = wedgeManager->getXphyPortPrbs(port, Side);
      EXPECT_EQ(*hwPrbs.enabled_ref(), enable)
          << "Port:" << port << " has undesired prbs enable state";
      EXPECT_EQ(*hwPrbs.polynominal_ref(), Polynominal)
          << "Port:" << port << " has undesired prbs polynominal";
    }
  }
};

#define TEST_NAME(SIDE, POLYNOMINAL, ENABLE) \
  BOOST_PP_CAT(                              \
      HwPortPrbsTest, BOOST_PP_CAT(SIDE, BOOST_PP_CAT(POLYNOMINAL, ENABLE)))

#define TEST_SET_PRBS(SIDE, POLYNOMINAL, ENABLE)                \
  struct TEST_NAME(SIDE, POLYNOMINAL, ENABLE)                   \
      : public HwPortPrbsTest<phy::Side::SIDE, POLYNOMINAL> {}; \
  TEST_F(TEST_NAME(SIDE, POLYNOMINAL, ENABLE), SetPrbs) {       \
    runTest(ENABLE);                                            \
  }

TEST_SET_PRBS(SYSTEM, 7, true);
TEST_SET_PRBS(LINE, 7, true);
TEST_SET_PRBS(SYSTEM, 7, false);
TEST_SET_PRBS(LINE, 7, false);

TEST_SET_PRBS(SYSTEM, 9, true);
TEST_SET_PRBS(LINE, 9, true);
TEST_SET_PRBS(SYSTEM, 9, false);
TEST_SET_PRBS(LINE, 9, false);

TEST_SET_PRBS(SYSTEM, 10, true);
TEST_SET_PRBS(LINE, 10, true);
TEST_SET_PRBS(SYSTEM, 10, false);
TEST_SET_PRBS(LINE, 10, false);

TEST_SET_PRBS(SYSTEM, 11, true);
TEST_SET_PRBS(LINE, 11, true);
TEST_SET_PRBS(SYSTEM, 11, false);
TEST_SET_PRBS(LINE, 11, false);

TEST_SET_PRBS(SYSTEM, 13, true);
TEST_SET_PRBS(LINE, 13, true);
TEST_SET_PRBS(SYSTEM, 13, false);
TEST_SET_PRBS(LINE, 13, false);

TEST_SET_PRBS(SYSTEM, 15, true);
TEST_SET_PRBS(LINE, 15, true);
TEST_SET_PRBS(SYSTEM, 15, false);
TEST_SET_PRBS(LINE, 15, false);

TEST_SET_PRBS(SYSTEM, 20, true);
TEST_SET_PRBS(LINE, 20, true);
TEST_SET_PRBS(SYSTEM, 20, false);
TEST_SET_PRBS(LINE, 20, false);

TEST_SET_PRBS(SYSTEM, 23, true);
TEST_SET_PRBS(LINE, 23, true);
TEST_SET_PRBS(SYSTEM, 23, false);
TEST_SET_PRBS(LINE, 23, false);

TEST_SET_PRBS(SYSTEM, 31, true);
TEST_SET_PRBS(LINE, 31, true);
TEST_SET_PRBS(SYSTEM, 31, false);
TEST_SET_PRBS(LINE, 31, false);

TEST_SET_PRBS(SYSTEM, 49, true);
TEST_SET_PRBS(LINE, 49, true);
TEST_SET_PRBS(SYSTEM, 49, false);
TEST_SET_PRBS(LINE, 49, false);

TEST_SET_PRBS(SYSTEM, 58, true);
TEST_SET_PRBS(LINE, 58, true);
TEST_SET_PRBS(SYSTEM, 58, false);
TEST_SET_PRBS(LINE, 58, false);
} // namespace facebook::fboss
