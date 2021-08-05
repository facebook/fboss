/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/qsfp_service/test/hw_test/HwTest.h"

#include <folly/logging/xlog.h>
#include "fboss/agent/FbossError.h"
#include "fboss/agent/platforms/common/PlatformProductInfo.h"
#include "fboss/lib/fpga/MultiPimPlatformSystemContainer.h"
#include "fboss/lib/phy/PhyManager.h"
#include "fboss/qsfp_service/test/hw_test/HwQsfpEnsemble.h"

DEFINE_bool(
    setup_for_warmboot,
    false,
    "Set up test for QSFP warmboot. Useful for testing individual "
    "tests doing a full process warmboot and verifying expectations");

namespace facebook::fboss {

void HwTest::SetUp() {
  // Set initializing pim xphys to 1 so we always initialize xphy chips
  gflags::SetCommandLineOptionWithMode(
      "init_pim_xphys", "1", gflags::SET_FLAGS_DEFAULT);

  ensemble_ = std::make_unique<HwQsfpEnsemble>();
  ensemble_->init();

  // Allow back to back refresh and customizations in test
  gflags::SetCommandLineOptionWithMode(
      "qsfp_data_refresh_interval", "0", gflags::SET_FLAGS_DEFAULT);
  gflags::SetCommandLineOptionWithMode(
      "customize_interval", "0", gflags::SET_FLAGS_DEFAULT);

  // Do an initial refresh so that the customization is done before the test
  // starts. It also takes ~5 seconds sometimes for the CMIS modules to be
  // functional after a data path deinit, that can happen in the customize call
  ensemble_->getWedgeManager()->refreshTransceivers();
  if (!didWarmBoot()) {
    // Warmboot shouldn't configure the transceiver again so we shouldn't
    // require a wait here
    sleep(5);
  }
}

void HwTest::TearDown() {
  ensemble_.reset();
}

bool HwTest::didWarmBoot() const {
  return ensemble_->didWarmBoot();
}

MultiPimPlatformPimContainer* HwTest::getPimContainer(int pimID) {
  return ensemble_->getPhyManager()->getSystemContainer()->getPimContainer(
      pimID);
}

void HwTest::setupForWarmboot() const {
  ensemble_->setupForWarmboot();
}
} // namespace facebook::fboss
