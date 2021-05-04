// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <map>
#include <vector>
#include "fboss/agent/FbossError.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/lib/phy/ExternalPhy.h"
#include "fboss/mdio/Mdio.h"

namespace facebook {
namespace fboss {

class MultiPimPlatformSystemContainer;

class PhyManager {
 public:
  virtual ~PhyManager() {}
  /*
   * This function initializes all the PHY objects for a given chassis. The PHY
   * objects are kept per slot, per MDIO controller, per phy address. This
   * needs to be defined by inheriting classes.
   */
  virtual bool initExternalPhyMap() = 0;

  virtual MultiPimPlatformSystemContainer* getSystemContainer() = 0;

  int getNumOfSlot() const {
    return numOfSlot_;
  }

  /*
   * This function returns the ExternalPhy object for the given slot number,
   * mdio controller id and phy id.
   */
  phy::ExternalPhy* getExternalPhy(int slotId, int mdioId, int phyId);

  /*
   * This function calls ExternalPhy function programOnePort for the given
   * slot id, mdio id, phy id
   */
  void
  programOnePort(int slotId, int mdioId, int phyId, phy::PhyPortConfig config);

  virtual void programOnePort(
      int32_t /* portId */,
      cfg::PortProfileID /* portProfileId */) {}

  /*
   * This function calls ExternalPhy function setPortPrbs for the given
   * slot id, mdio id, phy id
   */
  void setPortPrbs(
      int slotId,
      int mdioId,
      int phyId,
      phy::PhyPortConfig config,
      phy::Side side,
      bool enable,
      int32_t polynominal);

  /*
   * This function calls ExternalPhy function getPortStats for the given
   * slot id, mdio id, phy id
   */
  phy::ExternalPhyPortStats
  getPortStats(int slotId, int mdioId, int phyId, phy::PhyPortConfig config);

  /*
   * This function calls ExternalPhy function getPortPrbsStats for the given
   * slot id, mdio id, phy id
   */
  phy::ExternalPhyPortStats getPortPrbsStats(
      int slotId,
      int mdioId,
      int phyId,
      phy::PhyPortConfig config);

  /*
   * This function calls ExternalPhy function getLaneSpeed for the given
   * slot id, mdio id, phy id
   */
  float_t getLaneSpeed(
      int slotId,
      int mdioId,
      int phyId,
      phy::PhyPortConfig config,
      phy::Side side);

  /*
   * A virtual function for the ExternalPhy obejcts. The sub-class needs to
   * implement this function. The implementation will be different for
   * Minipack abd Yamp. If the Phy code is in same process then that should
   * called PhyManager function otherwise it should  be a thrift call to port
   * service process
   */
  virtual void initializeSlotPhys(int /* slotId */, bool /* warmboot */) {}

 protected:
  // Number of slot in the platform
  int numOfSlot_;
  // Number of MDIO controller in each slot
  std::map<int, int> numMdioController_;

  // List of ExternalPhy objects for the chassis
  // This is 3D array of ExternalPhy objects which is per PIM, per MDIO
  // controller, Per PHY endpoint
  std::map<int, std::vector<std::vector<std::unique_ptr<phy::ExternalPhy>>>>
      externalPhyMap_;
};

} // namespace fboss
} // namespace facebook
