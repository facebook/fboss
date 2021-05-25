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

#include "fboss/agent/FbossError.h"
#include "fboss/agent/platforms/common/PlatformProductInfo.h"
#include "fboss/lib/fpga/MultiPimPlatformSystemContainer.h"
#include "fboss/lib/phy/PhyManager.h"
#include "fboss/qsfp_service/test/hw_test/phy/HwPhyEnsemble.h"
#include "fboss/qsfp_service/test/hw_test/phy/HwPhyEnsembleFactory.h"

namespace facebook::fboss {

namespace {
// TODO(joseph5wu) Currently we haven't fully integrate firmware upgrade process
// w/ qsfp_service and hw_test. Therefore, we use static target firmware version
// for basic hw firmware test for now.
phy::PhyFwVersion getTargetFirmwareVersion() {
  phy::PhyFwVersion fw;
  fw.version_ref() = 91;
  fw.versionStr_ref() = "91.1";
  fw.minorVersion_ref() = 1;
  return fw;
}
} // namespace

void HwTest::SetUp() {
  auto initInfo = std::make_unique<HwPhyEnsemble::HwPhyEnsembleInitInfo>();
  initInfo->fwVersion = getTargetFirmwareVersion();
  ensemble_ = createHwEnsemble(std::move(initInfo));
}

void HwTest::TearDown() {
  // TODO(joseph5wu) Add entra step before reset the ensemble
  ensemble_.reset();
}

MultiPimPlatformPimContainer* HwTest::getPimContainer(int pimID) {
  return ensemble_->getPhyManager()->getSystemContainer()->getPimContainer(
      pimID);
}
} // namespace facebook::fboss
