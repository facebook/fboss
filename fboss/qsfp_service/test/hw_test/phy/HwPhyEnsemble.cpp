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
#include "fboss/lib/config/PlatformConfigUtils.h"
#include "fboss/lib/fpga/MultiPimPlatformSystemContainer.h"
#include "fboss/lib/phy/PhyManager.h"

namespace facebook::fboss {
HwPhyEnsemble::HwPhyEnsemble() {}

HwPhyEnsemble::~HwPhyEnsemble() {}

void HwPhyEnsemble::init(std::unique_ptr<HwPhyEnsembleInitInfo> initInfo) {
  initInfo_ = std::move(initInfo);

  auto multiPimPlatformMapping = chooseMultiPimPlatformMapping();

  phyManager_ = choosePhyManager(multiPimPlatformMapping.get());
  phyManager_->getSystemContainer()->initHW();

  // And then based on init info (pimType)to locate the first
  // available pim in the test environment to initialize.
  targetPimID_ = getFirstAvailablePimID();

  platformMapping_ =
      multiPimPlatformMapping->getPimPlatformMappingUniquePtr(targetPimID_);

  // Find the first xphy for this pim
  for (const auto& chip : platformMapping_->getChips()) {
    if (chip.second.get_type() == phy::DataPlanePhyChipType::XPHY) {
      isXphySupported_ = true;
      targetGlobalXphyID_ = chip.second.get_physicalID();
      // Fetch all the sw port ids for this xphy
      const auto& ports = utility::getPlatformPortsByChip(
          platformMapping_->getPlatformPorts(), chip.second);
      targetPorts_.clear();
      for (const auto& port : ports) {
        targetPorts_.push_back(PortID(port.get_mapping().get_id()));
      }
      break;
    }
  }
}

PimID HwPhyEnsemble::getFirstAvailablePimID() {
  auto pimStartNum = phyManager_->getSystemContainer()->getPimStartNum();
  for (auto i = 0; i < phyManager_->getNumOfSlot(); ++i) {
    if (initInfo_->pimType ==
        phyManager_->getSystemContainer()->getPimType(i + pimStartNum)) {
      return PimID(i + pimStartNum);
    }
  }
  throw FbossError(
      "Can't find pimType:",
      MultiPimPlatformPimContainer::getPimTypeStr(initInfo_->pimType),
      " pim in this platform");
}

phy::ExternalPhy* HwPhyEnsemble::getTargetExternalPhy() {
  return phyManager_->getExternalPhy(targetGlobalXphyID_);
}
} // namespace facebook::fboss
