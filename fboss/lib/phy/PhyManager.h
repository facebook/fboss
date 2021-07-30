// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/FbossError.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/types.h"
#include "fboss/lib/phy/ExternalPhy.h"
#include "fboss/lib/phy/ExternalPhyPortStatsUtils.h"
#include "fboss/lib/phy/gen-cpp2/phy_types.h"
#include "fboss/mdio/Mdio.h"
#include "fboss/mka_service/if/gen-cpp2/mka_types.h"

#include <folly/Synchronized.h>
#include <folly/futures/Future.h>
#include <map>
#include <optional>
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
  virtual ~PhyManager();

  GlobalXphyID getGlobalXphyIDbyPortID(PortID portID) const;
  virtual phy::PhyIDInfo getPhyIDInfo(GlobalXphyID xphyID) const = 0;
  virtual GlobalXphyID getGlobalXphyID(
      const phy::PhyIDInfo& phyIDInfo) const = 0;

  /*
   * This function initializes all the PHY objects for a given chassis. The PHY
   * objects are kept per slot, per MDIO controller, per phy address. This
   * needs to be defined by inheriting classes.
   */
  virtual bool initExternalPhyMap() = 0;

  /*
   * A virtual function for the ExternalPhy obejcts. The sub-class needs to
   * implement this function. The implementation will be different for
   * Minipack and Yamp. If the Phy code is in same process then that should
   * called PhyManager function otherwise it should be a thrift call to port
   * service process
   */
  virtual void initializeSlotPhys(PimID pimID, bool warmboot) = 0;

  virtual MultiPimPlatformSystemContainer* getSystemContainer() = 0;

  int getNumOfSlot() const {
    return numOfSlot_;
  }

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

  phy::PhyPortConfig getHwPhyPortConfig(PortID portId);

  virtual void programOnePort(
      PortID portId,
      cfg::PortProfileID portProfileId,
      std::optional<TransceiverInfo> transceiverInfo);

  // Macsec is only supported on SAI, so only SaiPhyManager will override this.
  virtual void sakInstallTx(const mka::MKASak& /* sak */) {
    throw FbossError("Attempted to call sakInstallTx from non-SaiPhyManager");
  }
  virtual void sakInstallRx(
      const mka::MKASak& /* sak */,
      const mka::MKASci& /* sci */) {
    throw FbossError("Attempted to call sakInstallRx from non-SaiPhyManager");
  }
  virtual void sakDeleteRx(
      const mka::MKASak& /* sak */,
      const mka::MKASci& /* sci */) {
    throw FbossError("Attempted to call sakDeleteRx from non-SaiPhyManager");
  }
  virtual void sakDelete(const mka::MKASak& /* sak */) {
    throw FbossError("Attempted to call sakDelete from non-SaiPhyManager");
  }

  folly::EventBase* getPimEventBase(PimID pimID) const;

  void
  setPortPrbs(PortID portID, phy::Side side, const phy::PortPrbsState& prbs);

  phy::PortPrbsState getPortPrbs(PortID portID, phy::Side side);

  // This is to provide the to-be cached warmboot state, which should include
  // the current portToCacheInfo_ map, so that during warmboot, we can use that
  // to recover the already programmed lane vector information from the cached
  // warmboot state.
  folly::dynamic getWarmbootState() const;

  void restoreFromWarmbootState(const folly::dynamic& phyWarmbootState);

 protected:
  const PlatformMapping* getPlatformMapping() {
    return platformMapping_;
  }

  void setupPimEventMultiThreading(PimID pimID);

  void setPortToLanesInfo(PortID portID, const phy::PhyPortConfig& portConfig);

  // Number of slot in the platform
  int numOfSlot_;

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
  virtual void createExternalPhy(
      const phy::PhyIDInfo& phyIDInfo,
      MultiPimPlatformPimContainer* pimContainer) = 0;

  const std::vector<LaneID>& getCachedLanes(PortID portID, phy::Side side)
      const;

  // Update PortCacheInfo::stats
  void updateStats(PortID portID);

  const PlatformMapping* platformMapping_;

  // For PhyManager programming xphy, each pim can program their xphys at the
  // same time without worrying affecting other pim's operation.
  struct PimEventMultiThreading {
    PimID pim;
    std::unique_ptr<folly::EventBase> eventBase;
    std::unique_ptr<std::thread> thread;

    explicit PimEventMultiThreading(PimID pimID);
    ~PimEventMultiThreading();
  };
  std::unordered_map<PimID, std::unique_ptr<PimEventMultiThreading>>
      pimToThread_;

  struct PortCacheInfo {
    // PhyManager is in the middle of changing its apis to accept PortID instead
    // of asking users to get all three Pim/MDIO Controller/PHY id.
    // Using a global PortID will make it easy for the communication b/w
    // wedge_agent and qsfp_service.
    // As for PhyManager, we need to use the GlobalXphyID to locate the exact
    // ExternalPhy so that we can call ExternalPhy apis to program the xphy.
    // This map will cache the two global ID: PortID and GlobalXphyID
    GlobalXphyID xphyID;
    // Based on current ExternalPhy design, it's hard to get which lanes that
    // a SW port is using. Because all the ExternalPhy are using lanes to
    // program the configs directly instead of a software port. Since we always
    // use PhyManager to programOnePort(), we can cache the LaneID list of both
    // system and line sides in PhyManager. And then for all the following xphy
    // related logic like, programming prbs, getting stats, we can just use this
    // cached info to pass in the xphy lanes directly.
    std::vector<LaneID> systemLanes;
    std::vector<LaneID> lineLanes;
    // Xphy port related stats and prbs stats
    folly::Synchronized<std::unique_ptr<ExternalPhyPortStatsUtils>> stats;
    std::optional<folly::Future<folly::Unit>> ongoingStatCollection;
    std::optional<folly::Future<folly::Unit>> ongoingPrbsStatCollection;
  };
  std::unordered_map<PortID, std::unique_ptr<PortCacheInfo>> portToCacheInfo_;
};

} // namespace fboss
} // namespace facebook
