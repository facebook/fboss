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
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/InterfaceMap.h"
#include "fboss/agent/state/Vlan.h"
#include "fboss/agent/state/VlanMap.h"

#include "SaiWarmBootCache.h"
#include "SaiSwitch.h"
#include "SaiPortTable.h"
#include "SaiError.h"

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

SaiWarmBootCache::SaiWarmBootCache(const SaiSwitch *hw)
  : hw_(hw) {

  VLOG(4) << "Entering " << __FUNCTION__;
}

shared_ptr<InterfaceMap> SaiWarmBootCache::reconstructInterfaceMap() const {
  VLOG(4) << "Entering " << __FUNCTION__;

  auto intfMap = make_shared<InterfaceMap>();

  for (const auto& vlanMacAndIntf: vlanAndMac2Intf_) {
    const auto &saiIntf = vlanMacAndIntf.second;
    
    sai_attribute_t saiAttr[3] = {};

    saiAttr[0].id = SAI_ROUTER_INTERFACE_ATTR_VLAN_ID;
    saiAttr[1].id = SAI_ROUTER_INTERFACE_ATTR_VIRTUAL_ROUTER_ID;
    saiAttr[2].id = SAI_ROUTER_INTERFACE_TYPE_PORT;
    
    
    hw_->getSaiRouterIntfApi()->get_router_interface_attribute(saiIntf, 
    sizeof(saiAttr)/sizeof(sai_attribute_t), saiAttr);

    /*
    intfMap->addInterface(make_shared<Interface>(InterfaceID(saiIntf),
                          RouterID(saiAttr[1].value.u32), 
                          VlanID(saiAttr[0].value.u32), 
                          "",
                          vlanMacAndIntf.first.second));
    */
  }

  return intfMap;
}

shared_ptr<VlanMap> SaiWarmBootCache::reconstructVlanMap() const {
  VLOG(4) << "Entering " << __FUNCTION__;

  auto vlans = make_shared<VlanMap>();
  flat_map<VlanID, VlanFields> vlan2VlanFields;

  // Get vlan and port mapping
  for (auto vlanAndInfo: vlan2VlanInfo_) {
    auto vlan = make_shared<Vlan>(vlanAndInfo.first, "");

    for (uint32_t i = 0; i < vlanAndInfo.second.ports_.size(); ++i) {

      sai_vlan_port_t& port = vlanAndInfo.second.ports_[i];
      PortID portId {0}; 

      try {
        portId = hw_->getPortTable()->getPortId(port.port_id);
      } catch (const SaiError &e) {
        LOG(ERROR) << e.what();
        continue;
      }

      vlan->addPort(portId, port.tagging_mode == SAI_VLAN_PORT_TAGGED);
    }

    vlans->addVlan(vlan);
  }

  flat_map<VlanID, AddrTables> vlan2AddrTables;

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

void SaiWarmBootCache::populate() {
}

void SaiWarmBootCache::addVlanInfo(VlanID vlan, 
                                   const vector<sai_vlan_port_t> &ports) {
    
  vlan2VlanInfo_.insert(make_pair(vlan, VlanInfo(vlan, ports)));
}
 
bool SaiWarmBootCache::fillVlanPortInfo(Vlan *vlan) {
  VLOG(4) << "Entering " << __FUNCTION__;

  auto vlanItr = vlan2VlanInfo_.find(vlan->getID());

  if (vlanItr != vlan2VlanInfo_.end()) {
    Vlan::MemberPorts memberPorts;

    for (uint32_t i = 0; i < vlanItr->second.ports_.size(); ++i) {

      sai_vlan_port_t& port = vlanItr->second.ports_[i];
      PortID portId {0}; 

      try {
        portId = hw_->getPortTable()->getPortId(port.port_id);
      } catch (const SaiError &e) {
        LOG(ERROR) << e.what();
        continue;
      }

      memberPorts.insert(make_pair(portId, port.tagging_mode == SAI_VLAN_PORT_TAGGED));
    }

    vlan->setPorts(memberPorts);

    return true;
  }

  return false;
}

void SaiWarmBootCache::clear() {
  VLOG(4) << "Entering " << __FUNCTION__;
  // Get rid of all unclaimed entries. The order is important here
  // since we want to delete entries only after there are no more
  // references to them.
  VLOG(1) << "Warm boot : removing unreferenced entries";

  //TODO Add here clear of Warm boot cache entries

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

    hw_->getSaiRouteApi()->remove_route(&vrfPfxAndRoute.second);

    // Delete sai host entries.
    for (auto vrfIpAndHost : vrfIp2Host_) {
      VLOG(1)<< "Deleting host entry in vrf: " <<
             vrfIpAndHost.first.first << " for : " << vrfIpAndHost.first.second;

      hw_->getSaiNeighborApi()->remove_neighbor_entry(&vrfIpAndHost.second);
    }

    vrfIp2Host_.clear();

    // Delete interfaces
    for (auto vlanMacAndIntf : vlanAndMac2Intf_) {
      VLOG(1) <<"Deletingl3 interface for vlan: " << vlanMacAndIntf.first.first
              <<" and mac : " << vlanMacAndIntf.first.second;

      hw_->getSaiRouterIntfApi()->remove_router_interface(vlanMacAndIntf.second);
    }

    vlanAndMac2Intf_.clear();

    // Finally delete the vlans
    for (auto vlanItr = vlan2VlanInfo_.begin();
         vlanItr != vlan2VlanInfo_.end();) {
      VLOG(1) << "Deleting vlan : " << vlanItr->first;

      hw_->getSaiVlanApi()->remove_vlan(vlanItr->first);
      vlanItr = vlan2VlanInfo_.erase(vlanItr);
    }
  }

}
}} // facebook::fboss

