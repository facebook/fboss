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

namespace facebook::fboss {

class PhyManager;

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
    // For XPHY platforms, they usually have multiple pims but our hw_test
    // doesn't really care to test on all pims. Using this info to control
    // how we will initialize pims and the phys on such pims for hw_test.
    std::optional<std::set<int>> pimIdsInfo;
  };

  HwPhyEnsemble();
  // TODO(joseph5wu) Might need some extra logic to handle desctrutor
  virtual ~HwPhyEnsemble();

  virtual void init(const HwPhyEnsembleInitInfo& info) = 0;

  PhyManager* getPhyManager() const {
    return phyManager_.get();
  }

 protected:
  std::unique_ptr<PhyManager> phyManager_;
};
} // namespace facebook::fboss
