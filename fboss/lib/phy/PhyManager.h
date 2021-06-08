// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/FbossError.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/types.h"
#include "fboss/lib/phy/ExternalPhy.h"
#include "fboss/mdio/Mdio.h"

#include <map>
#include <vector>

namespace facebook {
namespace fboss {

class MultiPimPlatformSystemContainer;
class MultiPimPlatformPimContainer;
class PlatformMapping;
class TransceiverInfo;

class PhyManager {
 public:
  explicit PhyManager(const PlatformMapping* platformMapping);
  virtual ~PhyManager() {}

  GlobalXphyID getGlobalXphyIDbyPortID(PortID portID) const;
  virtual phy::PhyIDInfo getPhyIDInfo(GlobalXphyID xphyID) const = 0;
  virtual GlobalXphyID getGlobalXphyID(const phy::PhyIDInfo& phyIDInfo) const;

  virtual void createExternalPhy(
      GlobalXphyID /* xphyID */,
      MultiPimPlatformPimContainer* /* pimContainer */) {}

  /*
   * This function initializes all the PHY objects for a given chassis. The PHY
   * objects are kept per slot, per MDIO controller, per phy address. This
   * needs to be defined by inheriting classes.
   */
  virtual bool initExternalPhyMap() = 0;

  virtual void initializeExternalPhy(
      GlobalXphyID /* xphyID */,
      bool /* warmBoot */) {}

  /*
   * A virtual function for the ExternalPhy obejcts. The sub-class needs to
   * implement this function. The implementation will be different for
   * Minipack abd Yamp. If the Phy code is in same process then that should
   * called PhyManager function otherwise it should  be a thrift call to port
   * service process
   */
  virtual void initializeSlotPhys(int /* slotId */, bool /* warmboot */) {}

  virtual MultiPimPlatformSystemContainer* getSystemContainer() = 0;

  int getNumOfSlot() const {
    return numOfSlot_;
  }

  /*
   * TODO(joseph5wu) Will deprecate the following logic once we can move all
   * PhyManager to use XphyMap xphyMap_ instead of 3D array externalPhyMap_
   * Check if the slot id is valid
   * This function returns the ExternalPhy object for the given slot number,
   * mdio controller id and phy id.
   */
  phy::ExternalPhy* getExternalPhy(int slotId, int mdioId, int phyId);

  /*
   * This function returns the ExternalPhy object for the giving global xphy id
   */
  phy::ExternalPhy* getExternalPhy(GlobalXphyID xphyID);

  /*
   * This function returns the ExternalPhy object for the giving software port
   * id
   */
  phy::ExternalPhy* getExternalPhy(PortID portID) {
    return getExternalPhy(getGlobalXphyIDbyPortID(portID));
  }

  phy::PhyPortConfig getDesiredPhyPortConfig(
      PortID portId,
      cfg::PortProfileID portProfileId,
      std::optional<TransceiverInfo> transceiverInfo);

  /*
   * This function calls ExternalPhy function programOnePort for the given
   * slot id, mdio id, phy id
   */
  void
  programOnePort(int slotId, int mdioId, int phyId, phy::PhyPortConfig config);

  virtual void programOnePort(
      PortID portId,
      cfg::PortProfileID portProfileId,
      std::optional<TransceiverInfo> transceiverInfo);

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

  // Since we're planning to allow PhyManager to use SW PortID to change
  // the xphy config for a FBOSS port, and current PlatformMapping has this
  // PortID : XPHY chip = 1 : 1 relationship, we should consider to use a
  // simplified map (2D vs 3D externalPhyMap_).
  // Besides there's no strong use case we need to deal with all the ports from
  // the same MDIO.
  // This design is also easier to let user to create one random XPHY for the
  // system(i.e. hw_test) without using vectors like externalPhyMap_ and
  // being cautious about the order of the mdio and phy.
  using PimXphyMap = std::map<GlobalXphyID, std::unique_ptr<phy::ExternalPhy>>;
  // Technically we can just use 1D map to store all the XPHY as we have
  // a unique xphy id for each one. But because we might need some pim-level
  // operations, like initializing all phys in the same slot(initializeSlotPhys)
  // or supporting multi-threading per pim. So we divide the whole xphy map
  // based on PimID
  using XphyMap = std::map<PimID, PimXphyMap>;
  XphyMap xphyMap_;

 private:
  // PhyManager is in the middle of changing its apis to accept PortID instead
  // of asking users to get all three Pim/MDIO Controller/PHY id.
  // Using a global PortID will make it easy for the communication b/w
  // wedge_agent and qsfp_service.
  // As for PhyManager, we need to use the GlobalXphyID to locate the exact
  // ExternalPhy so that we can call ExternalPhy apis to program the xphy.
  // This map will cache the two global ID: PortID and GlobalXphyID
  std::unordered_map<PortID, GlobalXphyID> portToGlobalXphyID_;
  const PlatformMapping* platformMapping_;
};

} // namespace fboss
} // namespace facebook
