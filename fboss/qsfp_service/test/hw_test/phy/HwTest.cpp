/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/qsfp_service/test/hw_test/phy/HwTest.h"

#include "fboss/agent/FbossError.h"
#include "fboss/agent/platforms/common/PlatformProductInfo.h"
#include "fboss/lib/fpga/MultiPimPlatformSystemContainer.h"
#include "fboss/lib/phy/PhyManager.h"
#include "fboss/qsfp_service/test/hw_test/phy/HwPhyEnsemble.h"
#include "fboss/qsfp_service/test/hw_test/phy/HwPhyEnsembleFactory.h"

DEFINE_string(
    target_pim_type,
    "",
    "Target pim type for hw_test. "
    "[MINIPACK_16Q/ MINIPACK_16O/ YAMP_16Q/ FUJI_16Q/ ELBERT_16Q/ ELBERT_8DD]");

namespace facebook::fboss {

void HwTest::SetUp() {
  auto initInfo = std::make_unique<HwPhyEnsemble::HwPhyEnsembleInitInfo>();
  // If user doesn't specify the pim type for testing, we use productInfo
  if (FLAGS_target_pim_type.empty()) {
    auto productInfo =
        std::make_unique<PlatformProductInfo>(FLAGS_fruid_filepath);
    productInfo->initialize();
    auto platformMode = productInfo->getMode();
    switch (platformMode) {
      case PlatformMode::ELBERT:
        initInfo->pimType = MultiPimPlatformPimContainer::PimType::ELBERT_8DD;
        break;
      default:
        throw FbossError(
            "Current phy hw_test doesn't support PlatformMode:", platformMode);
    }
  } else {
    initInfo->pimType =
        MultiPimPlatformPimContainer::getPimTypeFromStr(FLAGS_target_pim_type);
  }
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
