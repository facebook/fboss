// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/lib/phy/ExternalPhy.h"

namespace facebook::fboss {

class Port;

class PhyInterfaceHandler {
 public:
  virtual ~PhyInterfaceHandler() {}

  /*
   * initExternalPhyMap
   * A virtual function for initialzing the ExternalPhy obejcts. This function
   * needs to be implemented by inheriting class. If it has to be implemented
   * locally then that should called PhyManager function. If it has to be in
   * separate process then that inheriting class implemntation should  be
   * a thrift call to port service process
   */
  virtual bool initExternalPhyMap() = 0;

  /*
   * programOnePort
   * A virtual function for the ExternalPhy obejcts. The inheriting class
   * need to implement this function. If the Phy code is in same process
   * then that should called PhyManager function otherwise it should  be
   * a thrift call to port service process
   *
   * Note: PhyPortConfig needs to be removed once all Phy code is moved to
   * qsfp_service as based on portId, profileID the qsfp_service can construct
   * the phyPortConfig
   */
  virtual void programOnePort(
      int /* phyPortIdentifier */,
      int32_t /* portId */,
      cfg::PortProfileID /* portProfileId */,
      phy::PhyPortConfig /* config */) = 0;

  /*
   * setPortPrbs
   * A virtual function for the ExternalPhy obejcts. The inheriting class
   * need to implement this function. If the Phy code is in same process
   * then that should called PhyManager function otherwise it should  be
   * a thrift call to port service process
   *
   * Note: PhyPortConfig needs to be removed once all Phy code is moved to
   * qsfp_service as based on portId, profileID the qsfp_service can construct
   * the phyPortConfig
   */
  virtual void setPortPrbs(
      int /* phyPortIdentifier */,
      int32_t /* portId */,
      cfg::PortProfileID /* portProfileId */,
      phy::PhyPortConfig /* config */,
      phy::Side /* side */,
      bool /* enable */,
      int32_t /* polynominal */) = 0;

  /*
   * getPortStats
   * A virtual function for the ExternalPhy obejcts. The inheriting class
   * need to implement this function. If the Phy code is in same process
   * then that should called PhyManager function otherwise it should  be
   * a thrift call to port service process
   *
   * Note: PhyPortConfig needs to be removed once all Phy code is moved to
   * qsfp_service as based on portId, profileID the qsfp_service can construct
   * the phyPortConfig
   */
  virtual phy::ExternalPhyPortStats getPortStats(
      int /* phyPortIdentifier */,
      int32_t /* portId */,
      cfg::PortProfileID /* portProfileId */,
      phy::PhyPortConfig /* config */) = 0;

  /*
   * getPortPrbsStats
   * A virtual function for the ExternalPhy obejcts. The inheriting class
   * need to implement this function. If the Phy code is in same process
   * then that should called PhyManager function otherwise it should  be
   * a thrift call to port service process
   *
   * Note: PhyPortConfig needs to be removed once all Phy code is moved to
   * qsfp_service as based on portId, profileID the qsfp_service can construct
   * the phyPortConfig
   */
  virtual phy::ExternalPhyPortStats getPortPrbsStats(
      int /* phyPortIdentifier */,
      int32_t /* portId */,
      cfg::PortProfileID /* portProfileId */,
      phy::PhyPortConfig /* config */) = 0;

  /*
   * getPortInfo
   * A virtual function for the ExternalPhy obejcts. The inheriting class
   * need to implement this function. If the Phy code is in same process
   * then that should called PhyManager function otherwise it should  be
   * a thrift call to port service process
   *
   * Note: PhyPortConfig needs to be removed once all Phy code is moved to
   * qsfp_service as based on portId, profileID the qsfp_service can construct
   * the phyPortConfig
   */
  virtual phy::PhyInfo getPortInfo(
      int /* phyPortIdentifier */,
      phy::PhyPortConfig /* config */) {
    throw facebook::fboss::FbossError(
        "Port info not supported on this platform");
  }

  virtual void publishXphyInfoSnapshots(PortID /* portID */) const {
    throw facebook::fboss::FbossError(
        "Port info not supported on this platform");
  }
  virtual void updateXphyInfo(
      PortID /* portID */,
      const phy::PhyInfo& /* phyInfo */) {
    throw facebook::fboss::FbossError(
        "Port info not supported on this platform");
  }
  virtual std::optional<phy::PhyInfo> getXphyInfo(PortID /* portID */) const {
    throw facebook::fboss::FbossError(
        "Port info not supported on this platform");
  }

  /*
   * initializeSlotPhys
   * A virtual function for the ExternalPhy obejcts. The sub-class needs to
   * implement this function. The implementation will be different for
   * Minipack abd Yamp. If the Phy code is in same process then that should
   * called PhyManager function otherwise it should  be a thrift call to port
   * service process
   */
  virtual void initializeSlotPhys(PimID pimID, bool warmboot) = 0;

  /*
   * This function provides the slot id, mdio id, phy id for a global xphy id
   */
  virtual phy::PhyIDInfo getPhyIDInfo(GlobalXphyID xphyID) = 0;

 protected:
  std::vector<LaneID> getSideLanes(
      const phy::PhyPortConfig& config,
      phy::Side side) const {
    const auto& lanes = (side == phy::Side::SYSTEM) ? config.config.system.lanes
                                                    : config.config.line.lanes;
    std::vector<LaneID> sideLanes;
    for (const auto& it : lanes) {
      sideLanes.push_back(it.first);
    }
    return sideLanes;
  }
};

} // namespace facebook::fboss
