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
extern "C" {
#include <opennsl/l2.h>
#include <opennsl/l3.h>
#include <opennsl/port.h>
#include <opennsl/vlan.h>
}
#include <algorithm>
#include <string>
#include <list>
#include <memory>
#include <vector>
#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>
#include <folly/IPAddress.h>
#include <folly/MacAddress.h>
#include "fboss/agent/types.h"
#include "fboss/agent/state/RouteTypes.h"

namespace facebook { namespace fboss {
class BcmSwitch;
class InterfaceMap;
class Vlan;
class VlanMap;

class BcmWarmBootCache {
 public:
  explicit BcmWarmBootCache(const BcmSwitch* hw);
  void populate();
  struct VlanInfo {
    VlanInfo(VlanID _vlan, opennsl_pbmp_t _untagged, opennsl_pbmp_t _allPorts):
    vlan(_vlan) {
      OPENNSL_PBMP_ASSIGN(untagged, _untagged);
      OPENNSL_PBMP_ASSIGN(allPorts, _allPorts);
    }
    VlanID vlan;
    opennsl_pbmp_t untagged;
    opennsl_pbmp_t allPorts;
  };
  typedef opennsl_if_t EcmpEgressId;
  typedef opennsl_if_t EgressId;
  typedef boost::container::flat_set<EgressId> EgressIds;
  typedef boost::container::flat_map<EgressId, EgressIds> Ecmp2EgressIds;
  static EgressIds toEgressIds(EgressId* egress, int count) {
    EgressIds egressIds;
    std::for_each(egress, egress + count,
        [&](EgressId egress) { egressIds.insert(egress);});
    return egressIds;
  }
  static std::string toEgressIdsStr(const EgressIds& egressIds);
  /*
   * Reconstruct interface map from contents of warm boot cache
   */
  std::shared_ptr<InterfaceMap> reconstructInterfaceMap() const;
  /*
   * Reconstruct vlan map from contents of warm boot cache
   */
  std::shared_ptr<VlanMap> reconstructVlanMap() const;
  /*
   * Get all cached ecmp egress Ids
   */
  const Ecmp2EgressIds&  ecmp2EgressIds() const {
    return hwSwitchEcmp2EgressIds_;
  }
 private:
  typedef std::pair<EgressId, opennsl_l3_egress_t> EgressIdAndEgress;
  typedef std::pair<EcmpEgressId, opennsl_l3_egress_ecmp_t>
      EcmpEgressIdAndEgress;
  typedef std::pair<VlanID, folly::MacAddress> VlanAndMac;
  typedef std::pair<opennsl_if_t, folly::MacAddress> IntfIdAndMac;
  /*
   * VRF, IP, Mask
   * TODO - Convert mask to mask len for efficient storage/lookup
   */
  typedef std::tuple<opennsl_vrf_t, folly::IPAddress,
          folly::IPAddress> VrfAndPrefix;
  typedef std::pair<opennsl_vrf_t, folly::IPAddress> VrfAndIP;
  /*
   * Cache containers
   */
  typedef boost::container::flat_map<VlanID, VlanInfo> Vlan2VlanInfo;
  typedef boost::container::flat_map<VlanID, opennsl_l2_station_t> Vlan2Station;
  typedef boost::container::flat_map<VlanAndMac, opennsl_l3_intf_t>
    VlanAndMac2Intf;
  typedef boost::container::flat_map<VrfAndIP,
          opennsl_l3_host_t> VrfAndIP2Host;
  typedef boost::container::flat_map<VrfAndIP,
          EgressIdAndEgress> VrfAndIP2Egress;
  typedef boost::container::flat_map<EgressId, VrfAndIP> EgressId2VrfAndIP;
  typedef boost::container::flat_map<VrfAndPrefix, opennsl_l3_route_t>
    VrfAndPrefix2Route;
  typedef boost::container::flat_map<EgressIds,
          opennsl_l3_egress_ecmp_t> EgressIds2Ecmp;
  /*
   * Callbacks for traversing entries in BCM h/w tables
   */
  static int hostTraversalCallback(int unit, int index,
      opennsl_l3_host_t *info, void *user_data);
  static int egressTraversalCallback(int unit, EgressId intf,
      opennsl_l3_egress_t *info, void *user_data);
  static int routeTraversalCallback(int unit, int index,
      opennsl_l3_route_t* info, void* userData);
  static int ecmpEgressTraversalCallback(int unit,
      opennsl_l3_egress_ecmp_t *ecmp, int intf_count, opennsl_if_t *intf_array,
      void *user_data);
 public:
  /*
   * Iterators and find functions for finding VlanInfo
   */
  typedef Vlan2VlanInfo::const_iterator Vlan2VlanInfoCitr;
  Vlan2VlanInfoCitr vlan2VlanInfo_beg() const {
    return vlan2VlanInfo_.begin();
  }
  Vlan2VlanInfoCitr vlan2VlanInfo_end() const { return vlan2VlanInfo_.end(); }
  Vlan2VlanInfoCitr findVlanInfo(VlanID vlan) const {
    return vlan2VlanInfo_.find(vlan);
  }
  void programmed(Vlan2VlanInfoCitr vitr) {
    VLOG(1) << "Programmed vlan: " << vitr->first
      << " removing from warm boot cache";
    vlan2VlanInfo_.erase(vitr);
  }
  /*
   * Iterators and find functions for finding opennsl_l2_station_t
   */
  typedef Vlan2Station::const_iterator Vlan2StationCitr;
  Vlan2StationCitr vlan2Station_beg() const { return vlan2Station_.begin(); }
  Vlan2StationCitr vlan2Station_end() const { return vlan2Station_.end(); }
  Vlan2StationCitr findVlanStation(VlanID vlan) {
    return vlan2Station_.find(vlan);
  }
  void programmed(Vlan2StationCitr vsitr) {
    VLOG(1) << "Programmed station : " << vsitr->first
      << " removing from warm boot cache";
    vlan2Station_.erase(vsitr);
  }
  /*
   * Iterators and find functions for finding opennsl_l3_intf_t
   */
  typedef VlanAndMac2Intf::const_iterator VlanAndMac2IntfCitr;
  VlanAndMac2IntfCitr vlanAndMac2Intf_beg() const {
    return vlanAndMac2Intf_.begin();
  }
  VlanAndMac2IntfCitr vlanAndMac2Intf_end() const {
    return vlanAndMac2Intf_.end();
  }
  VlanAndMac2IntfCitr findL3Intf(VlanID vlan, folly::MacAddress mac) {
    return vlanAndMac2Intf_.find(VlanAndMac(vlan, mac));
  }
  void programmed(VlanAndMac2IntfCitr vmitr) {
    VLOG(1) << "Programmed interface in vlan : " << vmitr->first.first
      << " and mac: " << vmitr->first.second
      << " removing from warm boot cache";
    vlanAndMac2Intf_.erase(vmitr);
  }
  /*
   * Iterators and find functions for finding opennsl_l3_egress_t
   */
  typedef VrfAndIP2Egress::const_iterator VrfAndIP2EgressCitr;
  VrfAndIP2EgressCitr vrfAndIP2Egress_beg() const {
    return vrfIp2Egress_.begin();
  }
  VrfAndIP2EgressCitr vrfAndIP2Egress_end() const {
    return vrfIp2Egress_.end();
  }
  VrfAndIP2EgressCitr findEgress(opennsl_vrf_t vrf,
    const folly::IPAddress& nhopIp) const {
    return vrfIp2Egress_.find(VrfAndIP(vrf, nhopIp));
  }
  void programmed(VrfAndIP2EgressCitr vrecitr) {
    VLOG(1) << "Programmed vrf : " << vrecitr->first.first << " ip : "
      << vrecitr->first.second <<" and egress id : " << vrecitr->second.first
      << " removing from warm boot cache ";
    vrfIp2Egress_.erase(vrecitr);
  }
  opennsl_if_t getDropEgressId() const {
    return dropEgressId_;
  }
  opennsl_if_t getToCPUEgressId() const {
    return toCPUEgressId_;
  }
  /*
   * Iterators and find functions for finding opennsl_l3_host_t
   */
  typedef VrfAndIP2Host::const_iterator VrfAndIP2HostCitr;
  VrfAndIP2HostCitr vrfAndIP2Host_beg() const { return vrfIp2Host_.begin(); }
  VrfAndIP2HostCitr vrfAndIP2Host_end() const { return vrfIp2Host_.end(); }
  VrfAndIP2HostCitr findHost(opennsl_vrf_t vrf,
      const folly::IPAddress& ip) const {
    return vrfIp2Host_.find(VrfAndIP(vrf, ip));
  }
  void programmed(VrfAndIP2HostCitr vrhitr) {
    VLOG(1) << "Programmed host for vrf : " << vrhitr->first.first << " ip : "
      << vrhitr->first.second << " removing from warm boot cache ";
    vrfIp2Host_.erase(vrhitr);
  }
  /*
   * Iterators and find functions for finding opennsl_l3_route_t
   */
  typedef VrfAndPrefix2Route::const_iterator VrfAndPfx2RouteCitr;
  VrfAndPfx2RouteCitr vrfAndPrefix2Route_beg() const {
    return vrfPrefix2Route_.begin();
  }
  VrfAndPfx2RouteCitr vrfAndPrefix2Route_end() const {
    return vrfPrefix2Route_.end();
  }
  VrfAndPfx2RouteCitr findRoute(opennsl_vrf_t vrf, const folly::IPAddress& ip,
      uint8_t mask) {
    using folly::IPAddress;
    using folly::IPAddressV4;
    using folly::IPAddressV6;
    if (ip.isV6()) {
      return vrfPrefix2Route_.find(VrfAndPrefix(vrf, ip,
            IPAddress(IPAddressV6(IPAddressV6::fetchMask(mask)))));
    }
    return vrfPrefix2Route_.find(VrfAndPrefix(vrf, ip,
       IPAddress(IPAddressV4(IPAddressV4::fetchMask(mask)))));
  }
  void programmed(VrfAndPfx2RouteCitr vrpitr) {
    VLOG(1) << "Programmed route in vrf : " << std::get<0>(vrpitr->first)
      << "  prefix: " << std::get<1>(vrpitr->first) << "/"
      <<  std::get<2>(vrpitr->first) << " removing from warm boot cache ";
    vrfPrefix2Route_.erase(vrpitr);
  }
  /*
   * Iterators and find functions for opennsl_l3_egress_ecmp_t
   */
  typedef EgressIds2Ecmp::const_iterator EgressIds2EcmpCItr;
  EgressIds2EcmpCItr egressIds2Ecmp_begin() {
    return egressIds2Ecmp_.begin();
  }
  EgressIds2EcmpCItr egressIds2Ecmp_end() {
    return egressIds2Ecmp_.end();
  }
  EgressIds2EcmpCItr findEcmp(const EgressIds& egressIds) {
    return egressIds2Ecmp_.find(egressIds);
  }
  void programmed(EgressIds2EcmpCItr eeitr) {
    VLOG(1) << "Programmed ecmp egress: " << eeitr->second.ecmp_intf
      << " removing from warm boot cache";
    egressIds2Ecmp_.erase(eeitr);
    // Remove from ecmp->egressId mapping since now a BcmEcmpEgress
    // object exists which has the egress id info.
    hwSwitchEcmp2EgressIds_.erase(eeitr->second.ecmp_intf);
  }
  /*
   * owner is done programming its entries remove any entries
   * from hw that had owner as their only remaining owner
   */
  void clear();
  bool fillVlanPortInfo(Vlan* vlan);
 private:
  /*
   * Get egress ids for a ECMP Id. Will throw FbossError
   * if ecmp id is not found in hwSwitchEcmp2EgressIds_
   * map
   */
  const EgressIds& getPathsForEcmp(EgressId ecmp) const;
  void populateStateFromWarmbootFile();
  // No copy or assignment.
  BcmWarmBootCache(const BcmWarmBootCache&) = delete;
  BcmWarmBootCache& operator=(const BcmWarmBootCache&) = delete;
  const BcmSwitch* hw_;
  Vlan2VlanInfo vlan2VlanInfo_;
  Vlan2Station vlan2Station_;
  VlanAndMac2Intf vlanAndMac2Intf_;
  EgressId2VrfAndIP egressId2VrfIp_;
  VrfAndIP2Host vrfIp2Host_;
  VrfAndIP2Egress vrfIp2Egress_;
  VrfAndPrefix2Route vrfPrefix2Route_;
  EgressIds2Ecmp egressIds2Ecmp_;
  opennsl_if_t dropEgressId_;
  opennsl_if_t toCPUEgressId_;
  // hwSwitchEcmp2EgressIds_ represents what the mapping
  // from ecmp -> set<egressId> in the in memory BcmEcmpEgress
  // entries (and related host tables).
  // This may be distinct from the state in actual h/w since on
  // port down events we immediately remove the corresponding
  // egress ids from the ECMP entry in HW, but not from the BcmEcmpEgress
  // entry in memory. This minimizes the packets lost by quickly
  // shrinking the ECMP group and letting the traffic flow over
  // other up ports.
  // 2 alternate approaches were considered and rejected
  // a) Wait for routing protocols (BGP for e.g.) to notice the port
  // down and time out its peerings. The routing protocol would then
  // update all the routes that have a next hop that is reached over this
  // down port. Fboss agent would then be told about the updated routes,
  // at which point Fboss agent would update the relevant routes and ECMP
  // entries. This is *too slow*
  // b) On noticing the port down update all the routes and ECMP entries
  // etc in FBOSS agent itself. There problems with this is that we may
  // have a lot of routes. So each port down would create a big update.
  // Now if we get back to back port down events, the updates for the
  // second port would be queued behind the updates for the downed port.
  // The delay can be multiple seconds.
  Ecmp2EgressIds hwSwitchEcmp2EgressIds_;
  // When going from version where this table was not dumped to
  // one where we do, the table wont exist in the warm boot file.
  // So don't look for egressIds in this table.
  bool hwSwitchEcmp2EgressIdsPopulated_{false};
};
}} // facebook::fboss
