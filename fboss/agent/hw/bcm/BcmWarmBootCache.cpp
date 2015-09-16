/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "BcmWarmBootCache.h"
#include <limits>
#include <string>
#include <utility>
#include "fboss/agent/hw/bcm/BcmEgress.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/Utils.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/ArpTable.h"
#include "fboss/agent/state/NdpTable.h"
#include "fboss/agent/state/InterfaceMap.h"
#include "fboss/agent/state/Vlan.h"
#include "fboss/agent/state/VlanMap.h"

using std::make_pair;
using std::make_tuple;
using std::make_shared;
using std::numeric_limits;
using std::string;
using std::vector;
using std::shared_ptr;
using boost::container::flat_map;
using folly::ByteRange;
using folly::IPAddress;
using folly::MacAddress;
using boost::container::flat_map;
using boost::container::flat_set;
using namespace facebook::fboss;

namespace {
struct AddrTables {
  AddrTables() : arpTable(make_shared<ArpTable>()),
      ndpTable(make_shared<NdpTable>()) {}
  shared_ptr<facebook::fboss::ArpTable> arpTable;
  shared_ptr<facebook::fboss::NdpTable> ndpTable;
};
}

namespace facebook { namespace fboss {

BcmWarmBootCache::BcmWarmBootCache(const BcmSwitch* hw)
    : hw_(hw),
      dropEgressId_(BcmEgressBase::INVALID),
      toCPUEgressId_(BcmEgressBase::INVALID) {
}

shared_ptr<InterfaceMap> BcmWarmBootCache::reconstructInterfaceMap() const {
  auto intfMap = make_shared<InterfaceMap>();
  for (const auto& vlanMacAndIntf: vlanAndMac2Intf_) {
    const auto& bcmIntf = vlanMacAndIntf.second;
    // Note : missing addresses and inteface name. This should be
    // fixed with t4155406
    intfMap->addInterface(make_shared<Interface>(
            InterfaceID(bcmIntf.l3a_vid),
            RouterID(bcmIntf.l3a_vrf),
            VlanID(bcmIntf.l3a_vid),
            "",
            vlanMacAndIntf.first.second,
            bcmIntf.l3a_mtu));
  }
  return intfMap;
}

shared_ptr<VlanMap> BcmWarmBootCache::reconstructVlanMap() const {
  auto vlans = make_shared<VlanMap>();
  flat_map<VlanID, VlanFields> vlan2VlanFields;
  // Get vlan and port mapping
  for (auto vlanAndInfo: vlan2VlanInfo_) {
    // Note : missing vlan name. This should be
    // fixed with t4155406
    auto vlan = make_shared<Vlan>(vlanAndInfo.first, "");
    flat_set<int> untagged;
    int idx;
    OPENNSL_PBMP_ITER(vlanAndInfo.second.untagged, idx) {
      vlan->addPort(PortID(idx), false);
      untagged.insert(idx);
    }
    OPENNSL_PBMP_ITER(vlanAndInfo.second.allPorts, idx) {
      if (untagged.find(idx) != untagged.end()) {
        continue;
      }
      vlan->addPort(PortID(idx), true);
    }
    vlans->addVlan(vlan);
  }
  flat_map<VlanID, AddrTables> vlan2AddrTables;
  // Populate ARP and NDP tables of VLANs using egress
  // entries
  for (auto vrfIpAndEgress : vrfIp2Egress_) {
    const auto& bcmEgress = vrfIpAndEgress.second.second;
    if (bcmEgress.vlan == 0) {
      // Ignore to CPU egress entries which get mapped to vlan 0
      continue;
    }
    const auto& ip = vrfIpAndEgress.first.second;
    auto titr = vlan2AddrTables.find(VlanID(bcmEgress.vlan));
    if (titr == vlan2AddrTables.end()) {
      titr = vlan2AddrTables.insert(make_pair(VlanID(bcmEgress.vlan),
          AddrTables())).first;
    }

    // If we have a drop entry programmed for an existing host, it is a
    // pending entry
    if (ip.isV4()) {
      auto arpTable = titr->second.arpTable;
      if (BcmEgress::programmedToDrop(bcmEgress)) {
        arpTable->addPendingEntry(ip.asV4(), InterfaceID(bcmEgress.vlan));
      } else {
        arpTable->addEntry(ip.asV4(), macFromBcm(bcmEgress.mac_addr),
                           PortID(bcmEgress.port), InterfaceID(bcmEgress.vlan));
      }
    } else {
      auto ndpTable = titr->second.ndpTable;
      if (BcmEgress::programmedToDrop(bcmEgress)) {
        ndpTable->addPendingEntry(ip.asV6(), InterfaceID(bcmEgress.vlan));
      } else {
        ndpTable->addEntry(ip.asV6(), macFromBcm(bcmEgress.mac_addr),
                           PortID(bcmEgress.port), InterfaceID(bcmEgress.vlan));
      }
    }
  }
  for (auto vlanAndAddrTable: vlan2AddrTables) {
    auto vlan = vlans->getVlanIf(vlanAndAddrTable.first);
    if(!vlan) {
      LOG(FATAL) << "Vlan: " << vlanAndAddrTable.first <<  " not found";
    }
    vlan->setArpTable(vlanAndAddrTable.second.arpTable);
    vlan->setNdpTable(vlanAndAddrTable.second.ndpTable);
  }
  return vlans;
}

void BcmWarmBootCache::populate() {
  opennsl_vlan_data_t* vlanList = nullptr;
  int vlanCount = 0;
  SCOPE_EXIT {
    opennsl_vlan_list_destroy(hw_->getUnit(), vlanList, vlanCount);
  };
  auto rv = opennsl_vlan_list(hw_->getUnit(), &vlanList, &vlanCount);
  bcmCheckError(rv, "Unable to get vlan information");
  for (auto i = 0; i < vlanCount; ++i) {
    opennsl_vlan_data_t& vlanData = vlanList[i];
    int portCount;
    OPENNSL_PBMP_COUNT(vlanData.port_bitmap, portCount);
    VLOG (1) << "Got vlan : " << vlanData.vlan_tag
      <<" with : " << portCount << " ports";
    // TODO: Investigate why port_bitmap contains
    // the untagged ports rather than ut_port_bitmap
    vlan2VlanInfo_.insert(make_pair
        (BcmSwitch::getVlanId(vlanData.vlan_tag),
         VlanInfo(VlanID(vlanData.vlan_tag), vlanData.port_bitmap,
           vlanData.port_bitmap)));
    opennsl_l3_intf_t l3Intf;
    opennsl_l3_intf_t_init(&l3Intf);
    // Implicit here is the assumption that we have a interface
    // per vlan (since we are looking up the inteface by vlan).
    // If this changes we will have to store extra information
    // somewhere (e.g. interface id or vlan, mac pairs for interfaces
    // created) and then use that for lookup during warm boot.
    l3Intf.l3a_vid = vlanData.vlan_tag;
    bool intfFound = false;
    rv = opennsl_l3_intf_find_vlan(hw_->getUnit(), &l3Intf);
    if (rv != OPENNSL_E_NOT_FOUND) {
      bcmCheckError(rv, "failed to find interface for ",
          vlanData.vlan_tag);
      intfFound = true;
      vlanAndMac2Intf_[make_pair(BcmSwitch::getVlanId(l3Intf.l3a_vid),
          macFromBcm(l3Intf.l3a_mac_addr))] = l3Intf;
      VLOG(1) << "Found l3 interface for vlan : " << vlanData.vlan_tag;
    }
    if (intfFound) {
      opennsl_l2_station_t l2Station;
      opennsl_l2_station_t_init(&l2Station);
      rv = opennsl_l2_station_get(hw_->getUnit(), l3Intf.l3a_vid, &l2Station);
      if (!OPENNSL_FAILURE(rv)) {
        VLOG (1) << " Found l2 station with id : " << l3Intf.l3a_vid;
        vlan2Station_[VlanID(vlanData.vlan_tag)] = l2Station;
      } else {
        // FIXME Why are we unable to find l2 stations on a warm boot ?.
        VLOG(1) << "Could not get l2 station for vlan : " << vlanData.vlan_tag;
      }
    }
  }
  opennsl_l3_info_t l3Info;
  opennsl_l3_info_t_init(&l3Info);
  opennsl_l3_info(hw_->getUnit(), &l3Info);
  // Traverse V4 hosts
  opennsl_l3_host_traverse(hw_->getUnit(), 0, 0, l3Info.l3info_max_host,
      hostTraversalCallback, this);
  // Traverse V6 hosts
  opennsl_l3_host_traverse(hw_->getUnit(), OPENNSL_L3_IP6, 0,
      // Diag shell uses this for getting # of v6 host entries
      l3Info.l3info_max_host / 2,
      hostTraversalCallback, this);
  // Get egress entries
  opennsl_l3_egress_traverse(hw_->getUnit(), egressTraversalCallback, this);
  // Traverse V4 routes
  opennsl_l3_route_traverse(hw_->getUnit(), 0, 0, l3Info.l3info_max_route,
      routeTraversalCallback, this);
  // Traverse V6 routes
  opennsl_l3_route_traverse(hw_->getUnit(), OPENNSL_L3_IP6, 0,
      // Diag shell uses this for getting # of v6 route entries
      l3Info.l3info_max_route / 2,
      routeTraversalCallback, this);
  // Traverse ecmp egress entries
  opennsl_l3_egress_ecmp_traverse(hw_->getUnit(), ecmpEgressTraversalCallback,
      this);
  // Clear internal egress id table which just gets used while populating
  // warm boot cache
  egressId2VrfIp_.clear();
}

bool BcmWarmBootCache::fillVlanPortInfo(Vlan* vlan) {
  auto vlanItr = vlan2VlanInfo_.find(vlan->getID());
  if (vlanItr != vlan2VlanInfo_.end()) {
    Vlan::MemberPorts memberPorts;
    opennsl_port_t idx;
    OPENNSL_PBMP_ITER(vlanItr->second.untagged, idx) {
      memberPorts.insert(make_pair(PortID(idx), false));
    }
    OPENNSL_PBMP_ITER(vlanItr->second.allPorts, idx) {
      if (memberPorts.find(PortID(idx)) == memberPorts.end()) {
        memberPorts.insert(make_pair(PortID(idx), true));
      }
    }
    vlan->setPorts(memberPorts);
    return true;
  }
  return false;
}

int BcmWarmBootCache::hostTraversalCallback(int unit, int index,
    opennsl_l3_host_t* host, void* userData) {
  BcmWarmBootCache* cache = static_cast<BcmWarmBootCache*>(userData);
  auto ip = host->l3a_flags & OPENNSL_L3_IP6 ?
    IPAddress::fromBinary(ByteRange(host->l3a_ip6_addr,
          sizeof(host->l3a_ip6_addr))) :
    IPAddress::fromLongHBO(host->l3a_ip_addr);
  cache->vrfIp2Host_[make_pair(host->l3a_vrf, ip)] = *host;
  VLOG(1) << "Adding egress id: " << host->l3a_intf << " to " << ip
    <<" mapping";
  cache->egressId2VrfIp_[host->l3a_intf] = make_pair(host->l3a_vrf, ip);
  return 0;
}

int BcmWarmBootCache::egressTraversalCallback(int unit, EgressId egressId,
    opennsl_l3_egress_t *egress, void *userData) {
  BcmWarmBootCache* cache = static_cast<BcmWarmBootCache*>(userData);
  auto itr = cache->egressId2VrfIp_.find(egressId);
  if (itr != cache->egressId2VrfIp_.end()) {
    VLOG(1) << "Adding bcm egress entry for : " << itr->second.second
      << " in VRF : " << itr->second.first;
    cache->vrfIp2Egress_[itr->second] = make_pair(egressId, *egress);
  } else {
    // found egress ID that is not used by any host entry, we shall
    // only have two of them. One is for drop and the other one is for TO CPU.
    if ((egress->flags & OPENNSL_L3_DST_DISCARD)) {
      if (cache->dropEgressId_ != BcmEgressBase::INVALID) {
        LOG(FATAL) << "duplicated drop egress found in HW. " << egressId
                   << " and " << cache->dropEgressId_;
      }
      VLOG(1) << "Found drop egress id " << egressId;
      cache->dropEgressId_ = egressId;
    } else if ((egress->flags & (OPENNSL_L3_L2TOCPU|OPENNSL_L3_COPY_TO_CPU))) {
      if (cache->toCPUEgressId_ != BcmEgressBase::INVALID) {
        LOG(FATAL) << "duplicated generic TO_CPU egress found in HW. "
                   << egressId << " and " << cache->toCPUEgressId_;
      }
      VLOG(1) << "Found generic TO CPU egress id " << egressId;
      cache->toCPUEgressId_ = egressId;
    } else {
      LOG (FATAL) << " vrf and ip not found for egress : " << egressId;
    }
  }

  return 0;
}


int BcmWarmBootCache::routeTraversalCallback(int unit, int index,
    opennsl_l3_route_t* route, void* userData) {
  BcmWarmBootCache* cache = static_cast<BcmWarmBootCache*>(userData);
  auto ip = route->l3a_flags & OPENNSL_L3_IP6 ?
    IPAddress::fromBinary(ByteRange(route->l3a_ip6_net,
          sizeof(route->l3a_ip6_net))) :
    IPAddress::fromLongHBO(route->l3a_subnet);
  auto mask = route->l3a_flags & OPENNSL_L3_IP6 ?
    IPAddress::fromBinary(ByteRange(route->l3a_ip6_mask,
          sizeof(route->l3a_ip6_mask))) :
    IPAddress::fromLongHBO(route->l3a_ip_mask);
  VLOG (1) << "In vrf : " << route->l3a_vrf << " adding route for : "
    << ip << " mask: " << mask;
  cache->vrfPrefix2Route_[make_tuple(route->l3a_vrf, ip, mask)] = *route;
  return 0;
}

int BcmWarmBootCache::ecmpEgressTraversalCallback(int unit,
    opennsl_l3_egress_ecmp_t *ecmp, int intfCount, opennsl_if_t *intfArray,
    void *userData) {
  if (intfCount == 0) {
    // ecmp egress table on BCM has holes, ignore these entries
    return 0;
  }
  BcmWarmBootCache* cache = static_cast<BcmWarmBootCache*>(userData);
  EgressIds egressIds = cache->toEgressIds(intfArray, intfCount);;
  CHECK(cache->egressIds2Ecmp_.find(egressIds) ==
      cache->egressIds2Ecmp_.end());
  cache->egressIds2Ecmp_[egressIds] = *ecmp;
  VLOG(1) << "Added ecmp egress id : " << ecmp->ecmp_intf <<
    " pointing to : " << toEgressIdsStr(egressIds) << " egress ids";
  return 0;
}

std::string BcmWarmBootCache::toEgressIdsStr(const EgressIds& egressIds) {
  string egressStr;
  int i = 0;
  for (auto egressId : egressIds) {
    egressStr += folly::to<string>(egressId);
    egressStr += ++i < egressIds.size() ?  ", " : "";
  }
  return egressStr;
}

void BcmWarmBootCache::clear() {
  // Get rid of all unclaimed entries. The order is important here
  // since we want to delete entries only after there are no more
  // references to them.
  VLOG(1) << "Warm boot : removing unreferenced entries";
  // Nothing references routes, but routes reference ecmp egress
  // and egress entries which are deleted later
  for (auto vrfPfxAndRoute : vrfPrefix2Route_) {
    VLOG(1) << "Deleting unreferenced route in vrf:" <<
        std::get<0>(vrfPfxAndRoute.first) << " for prefix : " <<
        std::get<1>(vrfPfxAndRoute.first) << "/" <<
        std::get<2>(vrfPfxAndRoute.first);
    auto rv = opennsl_l3_route_delete(hw_->getUnit(), &(vrfPfxAndRoute.second));
    bcmLogFatal(rv, hw_, "failed to delete unreferenced route in vrf:",
        std::get<0>(vrfPfxAndRoute.first) , " for prefix : " ,
        std::get<1>(vrfPfxAndRoute.first) , "/" ,
        std::get<2>(vrfPfxAndRoute.first));
  }
  vrfPrefix2Route_.clear();
  // Only routes refer ecmp egress objects. Ecmp egress objects in turn
  // refer to egress objects which we delete later
  for (auto idsAndEcmp : egressIds2Ecmp_) {
    auto& ecmp = idsAndEcmp.second;
    VLOG(1) << "Deleting ecmp egress object  " << ecmp.ecmp_intf
      << " pointing to : " << toEgressIdsStr(idsAndEcmp.first);
    auto rv = opennsl_l3_egress_ecmp_destroy(hw_->getUnit(), &ecmp);
    bcmLogFatal(rv, hw_, "failed to destroy ecmp egress object :",
        ecmp.ecmp_intf, " referring to ",
        toEgressIdsStr(idsAndEcmp.first));
  }
  egressIds2Ecmp_.clear();

  // Delete bcm host entries. Nobody references bcm hosts, but
  // hosts reference egress objects
  for (auto vrfIpAndHost : vrfIp2Host_) {
    VLOG(1)<< "Deleting host entry in vrf: " <<
        vrfIpAndHost.first.first << " for : " << vrfIpAndHost.first.second;
    auto rv = opennsl_l3_host_delete(hw_->getUnit(), &vrfIpAndHost.second);
    bcmLogFatal(rv, hw_, "failed to delete host entry in vrf: ",
        vrfIpAndHost.first.first, " for : ", vrfIpAndHost.first.second);
  }
  vrfIp2Host_.clear();
  // Delete bcm egress entries. These are referenced by routes, ecmp egress
  // and host objects all of which we deleted above. Egress objects in turn
  // my point to a interface which we delete later
  for (auto vrfIpAndEgress : vrfIp2Egress_) {

    VLOG(1) << "Deleting egress object  " << vrfIpAndEgress.second.first;
    auto rv = opennsl_l3_egress_destroy(hw_->getUnit(),
        vrfIpAndEgress.second.first);
    bcmLogFatal(rv, hw_, "failed to destroy egress object ",
        vrfIpAndEgress.second.first);
  }
  vrfIp2Egress_.clear();
  // Delete interfaces
  for (auto vlanMacAndIntf : vlanAndMac2Intf_) {
    VLOG(1) <<"Deletingl3 interface for vlan: " << vlanMacAndIntf.first.first
      <<" and mac : " << vlanMacAndIntf.first.second;
    auto rv = opennsl_l3_intf_delete(hw_->getUnit(), &vlanMacAndIntf.second);
    bcmLogFatal(rv, hw_, "failed to delete l3 interface for vlan: ",
        vlanMacAndIntf.first.first, " and mac : ", vlanMacAndIntf.first.second);
  }
  vlanAndMac2Intf_.clear();
  // Delete stations
  for (auto vlanAndStation : vlan2Station_) {
    VLOG(1) << "Deleting station for vlan : " << vlanAndStation.first;
    auto rv = opennsl_l2_station_delete(hw_->getUnit(), vlanAndStation.first);
    bcmLogFatal(rv, hw_, "failed to delete station for vlan : ",
        vlanAndStation.first);
  }
  vlan2Station_.clear();
  opennsl_vlan_t defaultVlan;
  auto rv = opennsl_vlan_default_get(hw_->getUnit(), &defaultVlan);
  bcmLogFatal(rv, hw_, "failed to get default VLAN");
  // Finally delete the vlans
  for (auto vlanItr = vlan2VlanInfo_.begin();
      vlanItr != vlan2VlanInfo_.end();) {
    if (defaultVlan == vlanItr->first) {
      ++vlanItr;
      continue; // Can't delete the default vlan
    }
    VLOG(1) << "Deleting vlan : " << vlanItr->first;
    auto rv = opennsl_vlan_destroy(hw_->getUnit(), vlanItr->first);
    bcmLogFatal(rv, hw_, "failed to destroy vlan: ", vlanItr->first);
    vlanItr = vlan2VlanInfo_.erase(vlanItr);
  }
}
}}
