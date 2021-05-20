/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once

#include <memory>
#include <optional>
#include <set>
#include <vector>

#include "fboss/agent/types.h"
#include "fboss/lib/fpga/MultiPimPlatformPimContainer.h"
#include "fboss/lib/phy/gen-cpp2/phy_types.h"

namespace facebook::fboss {

class PhyManager;
class WedgeManager;
class MultiPimPlatformMapping;
class PlatformMapping;

namespace phy {
class ExternalPhy;
}

/*
 * HwPhyEnsemble will be the hw_test ensemble class to manager PhyManager.
 * This ensemble will be used in qsfp_service phy related hw_test to provide
 * all phy related helper function accross different phy implementation.
 * Currently we have three major implementations of PhyManager:
 * - Broadcom XPHY (Minipack1 and Fuji)
 * - Yamp XPHY
 * - SAI XPHY (Elbert)
 * This base class can provide the definition of all the phy related helper
 * functions, and then all the above three implementation can extend
 * HwPhyEnsemble to implement these functions in their own way with their own
 * PhyManager.
 * So the directory of phy related hw_test will be:
 * - phy
 * --- HwPhyEnsemble.h
 * --- sai
 * ----- SaiPhyEnsemble.h
 * --- bcm
 * ----- BcmPhyEnsemble.h
 * --- yamp
 * ----- YampPhyEnsemble.h
 */
class HwPhyEnsemble {
 public:
  struct HwPhyEnsembleInitInfo {
    // Specify which pim type the HwTest want to run all the tests against.
    // We have some platforms using two different pim types, for example,
    // Elbert uses Elbert16Q and Elbert8DD, and these linecards have different
    // phy features. So we need user to define which linecard type to use
    // before running the hw_test.
    MultiPimPlatformPimContainer::PimType pimType;
    // Expected default fwVersion installed after phy initialized
    phy::PhyFwVersion fwVersion;
  };

  HwPhyEnsemble();
  // TODO(joseph5wu) Might need some extra logic to handle desctrutor
  virtual ~HwPhyEnsemble();

  virtual void init(std::unique_ptr<HwPhyEnsembleInitInfo> initInfo);

  PhyManager* getPhyManager();

  PimID getTargetPimID() const {
    return targetPimID_;
  }

  GlobalXphyID getTargetGlobalXphyID() const {
    return targetGlobalXphyID_;
  }

  phy::ExternalPhy* getTargetExternalPhy();

  const HwPhyEnsembleInitInfo& getInitInfo() const {
    return *initInfo_;
  }

  const PlatformMapping* getPlatformMapping() const {
    return platformMapping_.get();
  }

  const std::vector<PortID>& getTargetPorts() const {
    return targetPorts_;
  }

 private:
  virtual std::unique_ptr<MultiPimPlatformMapping>
  chooseMultiPimPlatformMapping() = 0;

  // Based on pimType to find the first available pim ID.
  // Will throw exception if such pimType doesn't exist
  PimID getFirstAvailablePimID();

  std::unique_ptr<HwPhyEnsembleInitInfo> initInfo_;
  std::unique_ptr<WedgeManager> wedgeManager_;
  PimID targetPimID_;
  // PIM PlatformMapping
  std::unique_ptr<PlatformMapping> platformMapping_;
  GlobalXphyID targetGlobalXphyID_;
  std::vector<PortID> targetPorts_;
};
} // namespace facebook::fboss
