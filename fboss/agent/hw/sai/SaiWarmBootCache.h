/*
 * Copyright (c) 2004-present, Facebook, Inc.
 * Copyright (c) 2016, Cavium, Inc.
 * All rights reserved.
 * 
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 * 
 */
#pragma once

#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>

#include <folly/MacAddress.h>
#include <folly/IPAddress.h>

#include "fboss/agent/types.h"

extern "C" {
#include "sai.h"
}

namespace facebook { namespace fboss {

class SaiSwitch;
class InterfaceMap;
class Vlan;
class VlanMap;

class SaiWarmBootCache {
public:
  explicit SaiWarmBootCache(const SaiSwitch *hw);

  void populate();
  
  struct VlanInfo {
    VlanInfo(VlanID vlan, const std::vector<sai_vlan_port_t> &ports):
      vlan_(vlan), ports_(ports) {
    }

    VlanInfo(const VlanInfo& info):
      vlan_(info.vlan_), ports_(info.ports_) {
    }

    VlanID vlan_;
    std::vector<sai_vlan_port_t> ports_;
  };
  
  void addVlanInfo(VlanID vlan, const std::vector<sai_vlan_port_t> &ports);
  
  typedef sai_object_id_t EcmpEgressId;
  typedef sai_object_id_t EgressId;
  typedef boost::container::flat_set<EgressId> EgressIds;
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
  
private:
  typedef std::pair<EgressId, sai_object_id_t> EgressIdAndEgress;
  typedef std::pair<EcmpEgressId, sai_object_id_t>
      EcmpEgressIdAndEgress;
  typedef std::pair<VlanID, folly::MacAddress> VlanAndMac;
  typedef std::pair<sai_object_id_t, folly::MacAddress> IntfIdAndMac;
  /*
   * VRF, IP, Mask
   */
  typedef std::tuple<sai_object_id_t, folly::IPAddress,
          folly::IPAddress> VrfAndPrefix;
  typedef std::pair<sai_object_id_t, folly::IPAddress> VrfAndIP;
  /*
   * Cache containers
   */
  typedef boost::container::flat_map<VlanID, VlanInfo> Vlan2VlanInfo;
  typedef boost::container::flat_map<VlanAndMac, sai_object_id_t>
  VlanAndMac2Intf;
  typedef boost::container::flat_map<VrfAndIP,
          EgressIdAndEgress> VrfAndIP2Egress;
  typedef boost::container::flat_map<EgressId, VrfAndIP> EgressId2VrfAndIP;
  typedef boost::container::flat_map<EgressIds,
          sai_object_id_t> EgressIds2Ecmp;
  typedef boost::container::flat_map<VrfAndIP, sai_neighbor_entry_t>
  VrfAndIP2Host;
  typedef boost::container::flat_map<VrfAndPrefix, sai_unicast_route_entry_t>
  VrfAndPrefix2Route;
  
public:
  /*
   * Iterators and find functions for finding VlanInfo
   */
  typedef Vlan2VlanInfo::const_iterator Vlan2VlanInfoCitr;
  Vlan2VlanInfoCitr vlan2VlanInfo_beg() const {
    return vlan2VlanInfo_.begin();
  }
  Vlan2VlanInfoCitr vlan2VlanInfo_end() const {
    return vlan2VlanInfo_.end();
  }
  Vlan2VlanInfoCitr findVlanInfo(VlanID vlan) const {
    return vlan2VlanInfo_.find(vlan);
  }
  void programmed(Vlan2VlanInfoCitr vitr) {
    VLOG(1) << "Programmed vlan: " << vitr->first
            << " removing from warm boot cache";
    vlan2VlanInfo_.erase(vitr);
  }

  /*
  * Iterators and find functions for finding sai_router_interface_id_t
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

  /*
  * Iterators and find functions for finding sai_neighbor_entry_t
  */
  typedef VrfAndIP2Host::const_iterator VrfAndIP2HostCitr;
  VrfAndIP2HostCitr vrfAndIP2Host_beg() const {
    return vrfIp2Host_.begin();
  }
  VrfAndIP2HostCitr vrfAndIP2Host_end() const {
    return vrfIp2Host_.end();
  }
  VrfAndIP2HostCitr findHost(sai_object_id_t vrf,
                             const folly::IPAddress &ip) const {
    return vrfIp2Host_.find(VrfAndIP(vrf, ip));
  }
  void programmed(VrfAndIP2HostCitr vrhitr) {
    VLOG(1) << "Programmed host for vrf : " << vrhitr->first.first << " ip : "
            << vrhitr->first.second << " removing from warm boot cache ";
    vrfIp2Host_.erase(vrhitr);
  }

  /*
   * Iterators and find functions for finding sai_unicast_route_entry_t
   */
  typedef VrfAndPrefix2Route::const_iterator VrfAndPfx2RouteCitr;
  VrfAndPfx2RouteCitr vrfAndPrefix2Route_beg() const {
    return vrfPrefix2Route_.begin();
  }
  VrfAndPfx2RouteCitr vrfAndPrefix2Route_end() const {
    return vrfPrefix2Route_.end();
  }
  VrfAndPfx2RouteCitr findRoute(sai_object_id_t vrf, const folly::IPAddress &ip,
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

public:
  //TODO add function to check if VLANs exist and if it's changed
  /*
   * owner is done programming its entries remove any entries
   * from hw that had owner as their only remaining owner
   */
  void clear();
  bool fillVlanPortInfo(Vlan *vlan);

private:
  // No copy or assignment.
  SaiWarmBootCache(const SaiWarmBootCache &) = delete;
  SaiWarmBootCache &operator=(const SaiWarmBootCache &) = delete;
  const SaiSwitch *hw_;
  Vlan2VlanInfo vlan2VlanInfo_;
  VlanAndMac2Intf vlanAndMac2Intf_;
  VrfAndIP2Host vrfIp2Host_;
  VrfAndPrefix2Route vrfPrefix2Route_;
  EgressId2VrfAndIP egressId2VrfIp_;
  VrfAndIP2Egress vrfIp2Egress_;
  EgressIds2Ecmp egressIds2Ecmp_;
  sai_object_id_t dropEgressId_;
  sai_object_id_t toCPUEgressId_;
};

}} // facebook::fboss
