/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/HwSwitch.h"
#include "fboss/agent/hw/test/dataplane_tests/HwOverflowTest.h"
#include "fboss/agent/platforms/common/PlatformProductInfo.h"
#include "fboss/agent/test/RouteScaleGenerators.h"

namespace facebook::fboss {

TEST_F(HwOverflowTest, overflowRoutes) {
  setupEcmp();
  std::shared_ptr<SwitchState> desiredState;
  switch (getPlatform()->getMode()) {
    case PlatformMode::WEDGE:
      /*
       * On BRCM SAI TD2 scales better then TH in ALPM
       * mode. Hence we need RSW + Hgrid scale to actually
       * cause overflow
       */
      desiredState = utility::FSWRouteScaleGenerator(getProgrammedState())
                         .getSwitchStates()
                         .back();
      desiredState = utility::HgridUuRouteScaleGenerator(desiredState)
                         .getSwitchStates()
                         .back();
      break;

    case PlatformMode::GALAXY_FC:
    case PlatformMode::GALAXY_LC:
    case PlatformMode::WEDGE100:
      desiredState = utility::HgridUuRouteScaleGenerator(getProgrammedState())
                         .getSwitchStates()
                         .back();
      break;
    case PlatformMode::WEDGE400:
    case PlatformMode::MINIPACK:
    case PlatformMode::YAMP:
      /*
       * A route distribution 200,000 /128 does overflow the ASIC tables
       * but it takes 15min to generate, program and clean up such a
       * distribution. Since the code paths for overflow in our implementation
       * are platform independent, skip overflow on TH3 in the interest
       * of time.
       * Caveat: if for whatever reason, SDK stopped returning E_FULL on
       * table full on TH3, skipping overflow test here would hide such
       * a bug.
       */
      break;

    case PlatformMode::FAKE_WEDGE:
    case PlatformMode::FAKE_WEDGE40:
      // No limits to overflow for fakes
      break;
    case PlatformMode::WEDGE400C:
    case PlatformMode::WEDGE400C_SIM:
      XLOG(WARNING) << " No overflow test for 400C yet";
      break;
    case PlatformMode::FUJI:
    case PlatformMode::ELBERT:
      // No overflow test for TH4 yet
      break;
  }
  if (!desiredState) {
    return;
  }
  getHwSwitchEnsemble()->setAllowPartialStateApplication(true);
  if (getHwSwitch()->transactionsSupported()) {
    applyNewStateTransaction(desiredState);
  } else {
    applyNewState(desiredState);
  }
  EXPECT_NE(getProgrammedState(), desiredState);
  verifyInvariantsPostOverflow();
}

} // namespace facebook::fboss
