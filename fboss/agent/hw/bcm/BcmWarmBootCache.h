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
#include <folly/dynamic.h>
#include <folly/IPAddress.h>
#include <folly/MacAddress.h>
#include <folly/Optional.h>
#include "fboss/agent/types.h"
#include "fboss/agent/state/RouteTypes.h"

namespace facebook { namespace fboss {
class AclMap;
class AclRange;
class BcmSwitch;
class BcmSwitchIf;
class InterfaceMap;
class RouteTableMap;
class Vlan;
class VlanMap;
class SwitchState;

class BcmWarmBootCache {
 public:
  explicit BcmWarmBootCache(const BcmSwitchIf* hw);
  void populate();
  struct VlanInfo {
    VlanInfo(VlanID _vlan, opennsl_pbmp_t _untagged, opennsl_pbmp_t _allPorts,
             InterfaceID _intfID):
        vlan(_vlan),
        intfID(_intfID) {
      OPENNSL_PBMP_ASSIGN(untagged, _untagged);
      OPENNSL_PBMP_ASSIGN(allPorts, _allPorts);
    }

    // This constructor defaults to setting the interfaceID to be the same
    // as the vlan ID. This is our current behavior. If we want to support
    // interface IDs that are different than their corresponding vlan ID
    // the various call-sites need to use the other constructor.
    VlanInfo(VlanID _vlan, opennsl_pbmp_t _untagged, opennsl_pbmp_t _allPorts):
        VlanInfo(_vlan, _untagged, _allPorts, InterfaceID(_vlan)) {}

    VlanID vlan;
    opennsl_pbmp_t untagged;
    opennsl_pbmp_t allPorts;
    InterfaceID intfID;
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
   * Reconstruct route table map
   */
  std::shared_ptr<RouteTableMap> reconstructRouteTables() const;
  /*
   * Reconstruct acl map
   */
  std::shared_ptr<AclMap> reconstructAclMap() const;

  /*
   * Get all cached ecmp egress Ids
   */
  const Ecmp2EgressIds&  ecmp2EgressIds() const {
    return hwSwitchEcmp2EgressIds_;
  }
 private:
  using Egress = opennsl_l3_egress_t;
  using EcmpEgress = opennsl_l3_egress_ecmp_t;
  using IntfId = opennsl_if_t;
  using EgressIdAndEgress = std::pair<EgressId, Egress>;
  using VlanAndMac = std::pair<VlanID, folly::MacAddress>;
  using IntfIdAndMac = std::pair<IntfId, folly::MacAddress>;
  using HostKey = std::tuple<
    opennsl_vrf_t, folly::IPAddress, folly::Optional<opennsl_if_t>>;
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
  typedef boost::container::flat_map<VrfAndPrefix, opennsl_l3_route_t>
    VrfAndPrefix2Route;
  typedef boost::container::flat_map<EgressIds, EcmpEgress> EgressIds2Ecmp;
  using VrfAndIP2Route =
      boost::container::flat_map<VrfAndIP, opennsl_l3_route_t>;
  using EgressAndBool = std::pair<Egress, bool>;
  using EgressId2EgressAndBool =
      boost::container::flat_map<EgressId, EgressAndBool>;
  using HostTableInWarmBootFile = boost::container::flat_map<HostKey, EgressId>;

  // current h/w acl ranges: value = <BcmAclRangeHandle, ref_count>
  using BcmAclRangeHandle = uint32_t;
  using BcmAclEntryHandle = int;
  using AclRange2BcmAclRangeHandle = boost::container::flat_map<
        AclRange, std::pair<BcmAclRangeHandle, uint32_t>>;
  // current h/w acls: key = priority, value = BcmAclEntryHandle
  using Priority2BcmAclEntryHandle = boost::container::flat_map<
        int, BcmAclEntryHandle>;

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

  /**
   * Helper functions for populate AclEntry and AclRange since we don't have
   * OpenNSL support for Field Processor
   */
  // retrieve all bcm acls of the specified group
  void populateAcls(const int groupId, AclRange2BcmAclRangeHandle& ranges,
                    Priority2BcmAclEntryHandle& acls);
  // retrieve bcm acl ranges for the specified acl
  void populateAclRanges(const BcmAclEntryHandle acl,
                         AclRange2BcmAclRangeHandle& ranges);
  // remove bcm acl directly from h/w
  void removeBcmAcl(BcmAclEntryHandle handle);
  // remove bcm acl range directly from h/w
  void removeBcmAclRange(BcmAclRangeHandle handle);

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
  EgressId getDropEgressId() const {
    return dropEgressId_;
  }
  EgressId getToCPUEgressId() const {
    return toCPUEgressId_;
  }
  /**
   * Iterators and find functions for finding egress objects.
   */
  using EgressId2EgressAndBoolCitr = EgressId2EgressAndBool::const_iterator;
  EgressId2EgressAndBoolCitr egressId2EgressAndBool_begin() const {
    return egressId2EgressAndBool_.begin();
  }
  EgressId2EgressAndBoolCitr egressId2EgressAndBool_end() const {
    return egressId2EgressAndBool_.end();
  }
  EgressId2EgressAndBoolCitr findEgress(EgressId id) const {
    return egressId2EgressAndBool_.find(id);
  }
  EgressId2EgressAndBoolCitr findEgressFromHost(
      opennsl_vrf_t vrf,
      const folly::IPAddress& addr,
      folly::Optional<opennsl_if_t> intf);

  void programmed(EgressId2EgressAndBoolCitr citr) {
    VLOG(1) << "Programmed egress entry: " << citr->first
            << ". Marking at used in warmboot cache.";
    auto itr = egressId2EgressAndBool_.find(citr->first);
    itr->second.second = true;
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

  /**
   * Iterators and find functions for fully qualified routes in the route table.
   */
  using VrfAndIP2RouteCitr = VrfAndIP2Route::const_iterator;
  VrfAndIP2RouteCitr vrfAndIP2Route_begin() const {
    return vrfAndIP2Route_.begin();
  }
  VrfAndIP2RouteCitr vrfAndIP2Route_end() const {
    return vrfAndIP2Route_.end();
  }
  VrfAndIP2RouteCitr findHostRouteFromRouteTable(
      opennsl_vrf_t vrf, const folly::IPAddress& ip) const {
    return vrfAndIP2Route_.find(VrfAndIP(vrf, ip));
  }
  void programmed(VrfAndIP2RouteCitr citr) {
    VLOG(1) << "Programmed host route, removing from warm boot cache. "
            << "vrf: " << citr->first.first << " "
            << "ip: " << citr->first.second;
    vrfAndIP2Route_.erase(citr);
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
    // Remove from ecmp->egressId mapping since now a BcmEcmpEgress object
    // exists which has the egress id info.
    //
    // Note: This should be done before erasing the iterator.
    hwSwitchEcmp2EgressIds_.erase(eeitr->second.ecmp_intf);
    egressIds2Ecmp_.erase(eeitr);
  }

  /*
   * Iterators and find functions for acls and aclRanges
   */
  using Prio2BcmAclItr = Priority2BcmAclEntryHandle::const_iterator;
  Prio2BcmAclItr priority2BcmAclEntryHandle_begin() {
    return priority2BcmAclEntryHandle_.begin();
  }
  Prio2BcmAclItr priority2BcmAclEntryHandle_end() {
    return priority2BcmAclEntryHandle_.end();
  }
  Prio2BcmAclItr findAcl(int priority) {
    return priority2BcmAclEntryHandle_.find(priority);
  }
  void programmed(Prio2BcmAclItr itr) {
    VLOG(1) << "Programmed AclEntry, removing from warm boot cache. "
            << "priority=" << itr->first << " acl entry=" << itr->second;
    priority2BcmAclEntryHandle_.erase(itr);
  }

  using Range2BcmHandlerItr = AclRange2BcmAclRangeHandle::iterator;
  Range2BcmHandlerItr aclRange2BcmAclRangeHandle_begin() {
    return aclRange2BcmAclRangeHandle_.begin();
  }
  Range2BcmHandlerItr aclRange2BcmAclRangeHandle_end() {
    return aclRange2BcmAclRangeHandle_.end();
  }
  Range2BcmHandlerItr findBcmAclRange(const AclRange& aclRange) {
    return aclRange2BcmAclRangeHandle_.find(aclRange);
  }
  void programmed(Range2BcmHandlerItr itr);

  /*
   * owner is done programming its entries remove any entries
   * from hw that had owner as their only remaining owner
   */
  void clear();
  bool fillVlanPortInfo(Vlan* vlan);
  /*
   * Serialize to folly::dynamic
   */
  folly::dynamic toFollyDynamic() const;

  const BcmSwitchIf* getHw() const {
    return hw_;
  }

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
  const BcmSwitchIf* hw_;
  Vlan2VlanInfo vlan2VlanInfo_;
  Vlan2Station vlan2Station_;
  VlanAndMac2Intf vlanAndMac2Intf_;

  // This is the set of egress ids pointed by BcmHost in warm boot file.
  EgressIds egressIdsFromBcmHostInWarmBootFile_;
  // Mapping from <vrf, ip, intf> to the egress,
  // based on the BcmHost in warm boot file.
  HostTableInWarmBootFile vrfIp2EgressFromBcmHostInWarmBootFile_;

  // The host table in HW
  VrfAndIP2Host vrfIp2Host_;
  // These are routes from defip table that are not fully qualified (not /32 or
  // /128).
  VrfAndPrefix2Route vrfPrefix2Route_;
  // These are the fully qualified routes stored in defip table (/32 and /128
  // routes).
  VrfAndIP2Route vrfAndIP2Route_;
  EgressId2EgressAndBool egressId2EgressAndBool_;
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

  // acls and acl ranges
  AclRange2BcmAclRangeHandle aclRange2BcmAclRangeHandle_;
  Priority2BcmAclEntryHandle priority2BcmAclEntryHandle_;

  std::unique_ptr<SwitchState> dumpedSwSwitchState_;
};
}} // facebook::fboss
