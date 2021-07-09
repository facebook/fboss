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
#include "fboss/lib/CommonFileUtils.h"
#include "fboss/lib/fpga/MultiPimPlatformSystemContainer.h"
#include "fboss/lib/phy/PhyManager.h"
#include "fboss/qsfp_service/test/hw_test/HwQsfpEnsemble.h"

DEFINE_bool(
    setup_for_warmboot,
    false,
    "Set up test for QSFP warmboot. Useful for testing individual "
    "tests doing a full process warmboot and verifying expectations");

DECLARE_string(qsfp_service_volatile_dir);

namespace {
auto constexpr kQsfpTestWarmnbootFile = "qsfp_test_warmboot";
}
namespace facebook::fboss {

void HwTest::SetUp() {
  warmBoot_ = removeFile(folly::to<std::string>(
      FLAGS_qsfp_service_volatile_dir, "/", kQsfpTestWarmnbootFile));
  if (!warmBoot_) {
    createFile(WedgeManager::forceColdBootFileName());
  }
  ensemble_ = std::make_unique<HwQsfpEnsemble>();
  ensemble_->init();

  // Allow back to back refresh and customizations in test
  gflags::SetCommandLineOptionWithMode(
      "qsfp_data_refresh_interval", "0", gflags::SET_FLAGS_DEFAULT);
  gflags::SetCommandLineOptionWithMode(
      "customize_interval", "0", gflags::SET_FLAGS_DEFAULT);
}

void HwTest::TearDown() {
  ensemble_.reset();
  if (FLAGS_setup_for_warmboot) {
    createFile(folly::to<std::string>(
        FLAGS_qsfp_service_volatile_dir, "/", kQsfpTestWarmnbootFile));
  }
}

MultiPimPlatformPimContainer* HwTest::getPimContainer(int pimID) {
  return ensemble_->getPhyManager()->getSystemContainer()->getPimContainer(
      pimID);
}
} // namespace facebook::fboss
