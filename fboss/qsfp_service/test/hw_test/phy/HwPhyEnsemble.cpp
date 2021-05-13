/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/qsfp_service/test/hw_test/phy/HwPhyEnsemble.h"

#include "fboss/agent/FbossError.h"
#include "fboss/agent/platforms/common/MultiPimPlatformMapping.h"
#include "fboss/lib/fpga/MultiPimPlatformSystemContainer.h"
#include "fboss/lib/phy/PhyManager.h"

namespace facebook::fboss {
HwPhyEnsemble::HwPhyEnsemble() {}

HwPhyEnsemble::~HwPhyEnsemble() {}

void HwPhyEnsemble::init(std::unique_ptr<HwPhyEnsembleInitInfo> initInfo) {
  initInfo_ = std::move(initInfo);

  phyManager_ = choosePhyManager();
  phyManager_->getSystemContainer()->initHW();

  // And then based on init info (pimType)to locate the first
  // available pim in the test environment to initialize.
  targetPimID_ = getFirstAvailablePimID();

  platformMapping_ = chooseMultiPimPlatformMapping();
}

int8_t HwPhyEnsemble::getFirstAvailablePimID() {
  auto pimStartNum = phyManager_->getSystemContainer()->getPimStartNum();
  for (auto i = 0; i < phyManager_->getNumOfSlot(); ++i) {
    if (initInfo_->pimType ==
        phyManager_->getSystemContainer()->getPimType(i + pimStartNum)) {
      return i + pimStartNum;
    }
  }
  throw FbossError(
      "Can't find pimType:",
      MultiPimPlatformPimContainer::getPimTypeStr(initInfo_->pimType),
      " pim in this platform");
}

std::vector<int> HwPhyEnsemble::getTargetPimXphyList(
    MultiPimPlatformMapping* platformMapping) const {
  auto pimPlatformMapping =
      platformMapping->getPimPlatformMapping(targetPimID_);
  std::vector<int> xphys;
  for (const auto& chip : pimPlatformMapping->getChips()) {
    if (chip.second.get_type() == phy::DataPlanePhyChipType::XPHY) {
      xphys.push_back(chip.second.get_physicalID());
    }
  }
  return xphys;
}
} // namespace facebook::fboss
