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
#include "fboss/lib/phy/gen-cpp2/phy_types.h"
#include "fboss/qsfp_service/QsfpServiceHandler.h"
#include "fboss/qsfp_service/platforms/wedge/WedgeManager.h"

namespace apache::thrift {
class ThriftServer;
}
namespace facebook::fboss {

class PhyManager;
class WedgeManager;
class MultiPimPlatformMapping;
class PlatformMapping;

namespace phy {
class ExternalPhy;
}

/*
 * HwQsfpEnsemble will be the hw_test ensemble class to manager PhyManager.
 * This ensemble will be used in qsfp_service phy related hw_test to provide
 * all phy related helper function accross different phy implementation.
 * Currently we have three major implementations of PhyManager:
 * - Broadcom XPHY (Minipack1 and Fuji)
 * - Yamp XPHY
 * - SAI XPHY (Elbert)
 * This base class can provide the definition of all the phy related helper
 * functions, and then all the above three implementation can extend
 * HwQsfpEnsemble to implement these functions in their own way with their own
 * PhyManager.
 * So the directory of phy related hw_test will be:
 * - phy
 * --- HwQsfpEnsemble.h
 * --- sai
 * ----- SaiPhyEnsemble.h
 * --- bcm
 * ----- BcmPhyEnsemble.h
 * --- yamp
 * ----- YampPhyEnsemble.h
 */

class HwQsfpEnsemble {
 public:
  void init();
  ~HwQsfpEnsemble();

  PhyManager* getPhyManager();
  phy::ExternalPhy* getExternalPhy(PortID portID);

  const PlatformMapping* getPlatformMapping() const;
  WedgeManager* getWedgeManager();
  const WedgeManager* getWedgeManager() const {
    return const_cast<HwQsfpEnsemble*>(this)->getWedgeManager();
  }
  std::shared_ptr<QsfpServiceHandler> getQsfpServiceHandler() const {
    return qsfpServiceHandler_;
  }

  bool isXphyPlatform() const;
  bool didWarmBoot() const {
    return getWedgeManager()->canWarmBoot();
  }
  void setupForWarmboot();

  bool isSaiPlatform() const;

 private:
  std::shared_ptr<QsfpServiceHandler> qsfpServiceHandler_;
  std::shared_ptr<apache::thrift::ThriftServer> server_;
};
} // namespace facebook::fboss
