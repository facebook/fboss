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
#include "fboss/qsfp_service/platforms/wedge/WedgeManagerInit.h"

namespace facebook::fboss {

void HwPhyEnsemble::init() {
  wedgeManager_ = createWedgeManager();
  // Initialize the I2c bus
  wedgeManager_->initTransceiverMap();
  // Initialize the PhyManager all ExternalPhy for the system
  wedgeManager_->initExternalPhyMap();
}

PhyManager* HwPhyEnsemble::getPhyManager() {
  return wedgeManager_->getPhyManager();
}

const PlatformMapping* HwPhyEnsemble::getPlatformMapping() const {
  return wedgeManager_->getPlatformMapping();
}

phy::ExternalPhy* HwPhyEnsemble::getExternalPhy(PortID port) {
  auto phyManager = getPhyManager();
  return phyManager->getExternalPhy(phyManager->getGlobalXphyIDbyPortID(port));
}
} // namespace facebook::fboss
