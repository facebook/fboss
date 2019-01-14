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
#include <sstream>
#include <algorithm>
#include <string>
#include <utility>

#include <folly/Conv.h>
#include <folly/dynamic.h>
#include <folly/json.h>
#include <folly/logging/xlog.h>

#include "fboss/agent/Constants.h"
#include "fboss/agent/SysError.h"
#include "fboss/agent/hw/bcm/BcmAclTable.h"
#include "fboss/agent/hw/bcm/BcmEgress.h"
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmPlatform.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/bcm/BcmWarmBootHelper.h"
#include "fboss/agent/hw/bcm/BcmAddressFBConvertors.h"
#include "fboss/agent/state/ArpTable.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/InterfaceMap.h"
#include "fboss/agent/state/LoadBalancerMap.h"
#include "fboss/agent/state/MirrorMap.h"
#include "fboss/agent/state/NdpTable.h"
#include "fboss/agent/state/NeighborEntry.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/PortDescriptor.h"
#include "fboss/agent/state/SwitchState.h"
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
auto constexpr kEcmpObjects = "ecmpObjects";
auto constexpr kVlanForCPUEgressEntries = 0;
auto constexpr kACLFieldGroupID = 128;

struct AddrTables {
  AddrTables() : arpTable(make_shared<ArpTable>()),
      ndpTable(make_shared<NdpTable>()) {}
  shared_ptr<facebook::fboss::ArpTable> arpTable;
  shared_ptr<facebook::fboss::NdpTable> ndpTable;
};

folly::IPAddress getFullMaskIPv4Address() {
  return folly::IPAddress(folly::IPAddressV4(
      folly::IPAddressV4::fetchMask(folly::IPAddressV4::bitCount())));
}

folly::IPAddress getFullMaskIPv6Address() {
  return folly::IPAddress(folly::IPAddressV6(
      folly::IPAddressV6::fetchMask(folly::IPAddressV6::bitCount())));
}
}

namespace facebook { namespace fboss {

BcmWarmBootCache::BcmWarmBootCache(const BcmSwitchIf* hw)
    : hw_(hw),
      dropEgressId_(BcmEgressBase::INVALID),
      toCPUEgressId_(BcmEgressBase::INVALID) {}

shared_ptr<InterfaceMap> BcmWarmBootCache::reconstructInterfaceMap() const {
  std::shared_ptr<InterfaceMap> dumpedInterfaceMap =
      dumpedSwSwitchState_->getInterfaces();
  auto intfMap = make_shared<InterfaceMap>();
  for (const auto& vlanMacAndIntf: vlanAndMac2Intf_) {
    const auto& bcmIntf = vlanMacAndIntf.second;
    std::shared_ptr<Interface> dumpedInterface =
        dumpedInterfaceMap->getInterfaceIf(InterfaceID(bcmIntf.l3a_vid));
    // update with bcmIntf
    dumpedInterface->setRouterID(RouterID(bcmIntf.l3a_vrf));
    dumpedInterface->setVlanID(VlanID(bcmIntf.l3a_vid));
    dumpedInterface->setMac(vlanMacAndIntf.first.second);
    dumpedInterface->setMtu(bcmIntf.l3a_mtu);
    intfMap->addInterface(dumpedInterface);
  }
  return intfMap;
}

void BcmWarmBootCache::reconstructPortVlans(
  std::shared_ptr<SwitchState>* state) const {
  for (auto port : *dumpedSwSwitchState_->getPorts()) {
    auto newPort = (*state)->getPorts()->getPort(port->getID());
    for (auto vlanMember : port->getVlans()) {
      VlanID vlanID(vlanMember.first);
      newPort->addVlan(vlanID, vlanMember.second.tagged);
    }
  }
}

shared_ptr<VlanMap> BcmWarmBootCache::reconstructVlanMap() const {
  std::shared_ptr<VlanMap> dumpedVlans = dumpedSwSwitchState_->getVlans();
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
    vlan->setInterfaceID(vlanAndInfo.second.intfID);
    vlans->addVlan(vlan);
  }
  flat_map<VlanID, AddrTables> vlan2AddrTables;
  // Populate ARP and NDP tables of VLANs using egress entries
  for (const auto& hostEntry : vrfIp2EgressFromBcmHostInWarmBootFile_) {
    auto egressId = hostEntry.second;
    const auto egressIdAndEgress = findEgress(egressId);
    if (egressIdAndEgress == egressId2Egress_.end()) {
      // The host entry might be an ECMP egress entry.
      continue;
    }

    const auto& bcmEgress = egressIdAndEgress->second;
    if (bcmEgress.vlan == kVlanForCPUEgressEntries) {
      // Ignore to CPU egress entries which get mapped to vlan 0
      continue;
    }

    const auto& ip = std::get<1>(hostEntry.first);
    const auto& vlanId = VlanID(bcmEgress.vlan);
    // Is this ip a route or interface? If this is an entry for a route, then we
    // don't want to add this to the warm boot state.
    if (dumpedVlans) {
      const auto& dumpedVlan = dumpedVlans->getVlan(vlanId);
      if (ip.isV4() && !dumpedVlan->getArpTable()->getEntryIf(ip.asV4())) {
        continue;     // to next host entry
      }
      if (ip.isV6() && !dumpedVlan->getNdpTable()->getEntryIf(ip.asV6())) {
        continue;     // to next host entry
      }
    }
    auto titr = vlan2AddrTables.find(vlanId);
    if (titr == vlan2AddrTables.end()) {
      titr = vlan2AddrTables.insert(make_pair(vlanId, AddrTables())).first;
    }

    if (ip.isV4()) {
      auto arpTable = titr->second.arpTable;
      arpTable->addEntry(
          ip.asV4(),
          macFromBcm(bcmEgress.mac_addr),
          PortDescriptor(PortID(bcmEgress.port)),
          InterfaceID(bcmEgress.vlan),
          NeighborState::UNVERIFIED);
    } else {
      auto ndpTable = titr->second.ndpTable;
      ndpTable->addEntry(
          ip.asV6(),
          macFromBcm(bcmEgress.mac_addr),
          PortDescriptor(PortID(bcmEgress.port)),
          InterfaceID(bcmEgress.vlan),
          NeighborState::UNVERIFIED);
    }
    XLOG(DBG1) << "Reconstructed a neighbor entry."
               << " ip=" << ip << " mac=" << macFromBcm(bcmEgress.mac_addr)
               << " port=" << PortID(bcmEgress.port)
               << " vlan=" << bcmEgress.vlan;
  }
  for (auto vlanAndAddrTable: vlan2AddrTables) {
    auto vlan = vlans->getVlanIf(vlanAndAddrTable.first);
    if(!vlan) {
      XLOG(FATAL) << "Vlan: " << vlanAndAddrTable.first << " not found";
    }
    vlan->setArpTable(vlanAndAddrTable.second.arpTable);
    vlan->setNdpTable(vlanAndAddrTable.second.ndpTable);
  }
  return vlans;
}

std::shared_ptr<RouteTableMap>
BcmWarmBootCache::reconstructRouteTables() const {
  // This reconstruction is done from dumped state
  // rather than the HW tables as the host routes
  // are programmed into host tables on some platforms.
  // In the SwState (dumpedSwitchState_) these rightly
  // show up as routes, but in the HW host table there
  // is no way to distinguish a host entry from a host
  // route. Since we want to get all routes here (host
  // and LPM) we just get it from the dumped switch state.
  return dumpedSwSwitchState_->getRouteTables();
}

std::shared_ptr<AclMap>
BcmWarmBootCache::reconstructAclMap() const {
  return dumpedSwSwitchState_->getAcls();
}

std::shared_ptr<QosPolicyMap> BcmWarmBootCache::reconstructQosPolicies() const {
  return dumpedSwSwitchState_->getQosPolicies();
}

std::shared_ptr<LoadBalancerMap> BcmWarmBootCache::reconstructLoadBalancers()
    const {
  return dumpedSwSwitchState_->getLoadBalancers();
}

std::shared_ptr<MirrorMap> BcmWarmBootCache::reconstructMirrors()
    const {
  return dumpedSwSwitchState_->getMirrors();
}

void BcmWarmBootCache::programmed(Range2BcmHandlerItr itr) {
  XLOG(DBG1) << "Programmed AclRange, removing from warm boot cache."
             << " flags=" << itr->first.getFlags()
             << " min=" << itr->first.getMin() << " max=" << itr->first.getMax()
             << " handle= " << itr->second.first
             << " current ref count=" << itr->second.second;
  if (itr->second.second > 1) {
    itr->second.second--;
  } else {
    aclRange2BcmAclRangeHandle_.erase(itr);
  }
}

void BcmWarmBootCache::programmed(AclEntry2AclStatItr itr) {
  XLOG(DBG1) << "Programmed acl stat=" << itr->second.stat;
  itr->second.claimed = true;
}

BcmWarmBootCache::AclEntry2AclStatItr BcmWarmBootCache::findAclStat(
    const BcmAclEntryHandle& bcmAclEntry) {
  auto aclStatItr = aclEntry2AclStat_.find(bcmAclEntry);
  if (aclStatItr != aclEntry2AclStat_.end() && aclStatItr->second.claimed) {
    return aclEntry2AclStat_.end();
  }
  return aclStatItr;
}

const BcmWarmBootCache::EgressIds& BcmWarmBootCache::getPathsForEcmp(
    EgressId ecmp) const {
  static const EgressIds kEmptyEgressIds;
  if (hwSwitchEcmp2EgressIds_.empty()) {
    // We may have empty hwSwitchEcmp2EgressIds_ when
    // we exited with no ECMP entries.
    return kEmptyEgressIds;
  }
  auto itr = hwSwitchEcmp2EgressIds_.find(ecmp);
  if (itr == hwSwitchEcmp2EgressIds_.end()) {
    throw FbossError("Could not find ecmp ID : ", ecmp);
  }
  return itr->second;
}

folly::dynamic BcmWarmBootCache::toFollyDynamic() const {
  folly::dynamic warmBootCache = folly::dynamic::object;
  // For now we serialize only the hwSwitchEcmp2EgressIds_ table.
  // This is the only thing we need and may not be able to get
  // from HW in the case where we shut down before doing a FIB sync.
  folly::dynamic ecmps = folly::dynamic::array;
  for (auto& ecmpAndEgressIds : hwSwitchEcmp2EgressIds_) {
    folly::dynamic ecmp = folly::dynamic::object;
    ecmp[kEcmpEgressId] = ecmpAndEgressIds.first;
    folly::dynamic paths = folly::dynamic::array;
    for (const auto& path : ecmpAndEgressIds.second) {
      paths.push_back(path);
    }
    ecmp[kPaths] = std::move(paths);
    ecmps.push_back(std::move(ecmp));
  }
  warmBootCache[kEcmpObjects] = std::move(ecmps);
  return warmBootCache;
}

folly::dynamic BcmWarmBootCache::getWarmBootState() const {
  return hw_->getPlatform()->getWarmBootHelper()->getWarmBootState();
}

void BcmWarmBootCache::populateFromWarmBootState(const folly::dynamic&
    warmBootState) {

  dumpedSwSwitchState_ =
    SwitchState::uniquePtrFromFollyDynamic(warmBootState[kSwSwitch]);
  CHECK(dumpedSwSwitchState_)
      << "Was not able to recover software state after warmboot";

  // Extract ecmps for dumped host table
  auto hostTable = warmBootState[kHwSwitch][kHostTable];
  for (const auto& ecmpEntry : hostTable[kEcmpHosts]) {
    auto ecmpEgressId = ecmpEntry[kEcmpEgressId].asInt();
    if (ecmpEgressId == BcmEgressBase::INVALID) {
      continue;
    }
    // If the entry is valid, then there must be paths associated with it.
    for (auto path : ecmpEntry[kEcmpEgress][kPaths]) {
      EgressId e = path.asInt();
      hwSwitchEcmp2EgressIds_[ecmpEgressId].insert(e);
    }
  }
  // Extract ecmps from dumped warm boot cache. We
  // may have shut down before a FIB sync
  auto ecmpObjects = warmBootState[kHwSwitch][kWarmBootCache][kEcmpObjects];
  for (const auto& ecmpEntry : ecmpObjects) {
    auto ecmpEgressId = ecmpEntry[kEcmpEgressId].asInt();
    CHECK(ecmpEgressId != BcmEgressBase::INVALID);
    for (const auto& path : ecmpEntry[kPaths]) {
      EgressId e = path.asInt();
      hwSwitchEcmp2EgressIds_[ecmpEgressId].insert(e);
    }
  }
  XLOG(DBG1) << "Reconstructed following ecmp path map ";
  for (auto& ecmpIdAndEgress : hwSwitchEcmp2EgressIds_) {
    XLOG(DBG1) << ecmpIdAndEgress.first << " (from warmboot file) ==> "
               << toEgressIdsStr(ecmpIdAndEgress.second);
  }

  // Extract BcmHost and its egress object from the warm boot file
  for (const auto& hostEntry : hostTable[kHosts]) {
    auto egressId = hostEntry[kEgressId].asInt();
    if (egressId == BcmEgressBase::INVALID) {
      continue;
    }
    egressIdsFromBcmHostInWarmBootFile_.insert(egressId);

    folly::Optional<opennsl_if_t> intf{folly::none};
    auto ip = folly::IPAddress(hostEntry[kIp].stringPiece());
    if (ip.isV6() && ip.isLinkLocal()) {
      auto egressIt = hostEntry.find(kEgress);
      if (egressIt != hostEntry.items().end()) {
        // check if kIntfId is part of the key, if not. That means it is an ECMP
        // egress object. No interface in this case.
        auto intfIt = egressIt->second.find(kIntfId);
        if (intfIt != egressIt->second.items().end()) {
          intf = intfIt->second.asInt();
        }
      }
    }
    auto vrf = hostEntry[kVrf].asInt();
    auto key = std::make_tuple(vrf, ip, intf);
    vrfIp2EgressFromBcmHostInWarmBootFile_[key] = egressId;

    XLOG(DBG1) << "Construct a host entry (vrf=" << vrf << ",ip=" << ip
               << ",intf="
               << (intf.hasValue() ? folly::to<std::string>(intf.value())
                                   : "None")
               << ") pointing to the egress entry, id=" << egressId;
  }
}

BcmWarmBootCache::EgressId2EgressCitr
BcmWarmBootCache::findEgressFromHost(
    opennsl_vrf_t vrf,
    const folly::IPAddress &addr,
    folly::Optional<opennsl_if_t> intf) {

  // Do a cheap check of size to avoid construct the key for lookup.
  // That helps the case after warmboot is done.
  if (vrfIp2EgressFromBcmHostInWarmBootFile_.size() == 0) {
    return egressId2Egress_.end();
  }
  // only care about the intf if addr is v6 link-local
  if (!addr.isV6() || !addr.isLinkLocal()) {
    intf = folly::none;
  }
  auto key = std::make_tuple(vrf, addr, intf);
  auto it = vrfIp2EgressFromBcmHostInWarmBootFile_.find(key);
  if (it == vrfIp2EgressFromBcmHostInWarmBootFile_.cend()) {
    return egressId2Egress_.end();
  }
  return findEgress(it->second);
}

void BcmWarmBootCache::populate(folly::Optional<folly::dynamic> warmBootState) {
  if (warmBootState) {
    populateFromWarmBootState(*warmBootState);
  } else {
    populateFromWarmBootState(getWarmBootState());
  }
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
    XLOG(DBG1) << "Got vlan : " << vlanData.vlan_tag << " with : " << portCount
               << " ports";
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
      XLOG(DBG1) << "Found l3 interface for vlan : " << vlanData.vlan_tag;
    }
    if (intfFound) {
      opennsl_l2_station_t l2Station;
      opennsl_l2_station_t_init(&l2Station);
      rv = opennsl_l2_station_get(hw_->getUnit(), l3Intf.l3a_vid, &l2Station);
      if (!OPENNSL_FAILURE(rv)) {
        XLOG(DBG1) << " Found l2 station with id : " << l3Intf.l3a_vid;
        vlan2Station_[VlanID(vlanData.vlan_tag)] = l2Station;
      } else {
        XLOG(DBG1) << "Could not get l2 station for vlan : "
                   << vlanData.vlan_tag;
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
  // Traverse V4 routes
  opennsl_l3_route_traverse(hw_->getUnit(), 0, 0, l3Info.l3info_max_route,
      routeTraversalCallback, this);
  // Traverse V6 routes
  opennsl_l3_route_traverse(hw_->getUnit(), OPENNSL_L3_IP6, 0,
      // Diag shell uses this for getting # of v6 route entries
      l3Info.l3info_max_route / 2,
      routeTraversalCallback, this);
  // Get egress entries.
  opennsl_l3_egress_traverse(hw_->getUnit(), egressTraversalCallback, this);
  // Traverse ecmp egress entries
  opennsl_l3_egress_ecmp_traverse(hw_->getUnit(), ecmpEgressTraversalCallback,
      this);

  // populate acls, acl stats and acl ranges
  populateAcls(kACLFieldGroupID, this->aclRange2BcmAclRangeHandle_,
    this->aclEntry2AclStat_,
    this->priority2BcmAclEntryHandle_);

  populateRtag7State();
  populateMirrors();
  populateMirroredPorts();
  populateIngressQosMaps();
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

int BcmWarmBootCache::hostTraversalCallback(
    int /*unit*/,
    int /*index*/,
    opennsl_l3_host_t* host,
    void* userData) {
  BcmWarmBootCache* cache = static_cast<BcmWarmBootCache*>(userData);
  auto ip = host->l3a_flags & OPENNSL_L3_IP6 ?
    IPAddress::fromBinary(ByteRange(host->l3a_ip6_addr,
          sizeof(host->l3a_ip6_addr))) :
    IPAddress::fromLongHBO(host->l3a_ip_addr);
  cache->vrfIp2Host_[make_pair(host->l3a_vrf, ip)] = *host;
  XLOG(DBG1) << "Adding egress id: " << host->l3a_intf << " to " << ip
             << " mapping";
  return 0;
}

int BcmWarmBootCache::egressTraversalCallback(
    int /*unit*/,
    EgressId egressId,
    opennsl_l3_egress_t* egress,
    void* userData) {
  BcmWarmBootCache* cache = static_cast<BcmWarmBootCache*>(userData);
  CHECK(cache->egressId2Egress_.find(egressId) == cache->egressId2Egress_.end())
      << "Double callback for egress id: " << egressId;
  // Look up egressId in egressIdsFromBcmHostInWarmBootFile_
  // to populate both dropEgressId_ and toCPUEgressId_.
  auto egressIdItr = cache->egressIdsFromBcmHostInWarmBootFile_.find(egressId);
  if (egressIdItr != cache->egressIdsFromBcmHostInWarmBootFile_.cend()) {
    // May be: Add information to figure out how many host or route entry
    // reference it.
    XLOG(DBG1) << "Adding bcm egress entry for: " << *egressIdItr
               << " which is referenced by at least one host or route entry.";
    cache->egressId2Egress_[egressId] = *egress;
  } else {
    // found egress ID that is not used by any host entry, we shall
    // only have two of them. One is for drop and the other one is for TO CPU.
    if ((egress->flags & OPENNSL_L3_DST_DISCARD)) {
      if (cache->dropEgressId_ != BcmEgressBase::INVALID) {
        XLOG(FATAL) << "duplicated drop egress found in HW. " << egressId
                    << " and " << cache->dropEgressId_;
      }
      XLOG(DBG1) << "Found drop egress id " << egressId;
      cache->dropEgressId_ = egressId;
    } else if ((egress->flags &
                (OPENNSL_L3_L2TOCPU | OPENNSL_L3_COPY_TO_CPU))) {
      if (cache->toCPUEgressId_ != BcmEgressBase::INVALID) {
        XLOG(FATAL) << "duplicated generic TO_CPU egress found in HW. "
                    << egressId << " and " << cache->toCPUEgressId_;
      }
      XLOG(DBG1) << "Found generic TO CPU egress id " << egressId;
      cache->toCPUEgressId_ = egressId;
    } else {
      XLOG(FATAL) << "The egress: " << egressId
                  << " is not referenced by any host entry. vlan: "
                  << egress->vlan << " interface: " << egress->intf
                  << " flags: " << std::hex << egress->flags << std::dec;
    }
  }
  return 0;
}

int BcmWarmBootCache::routeTraversalCallback(
    int /*unit*/,
    int /*index*/,
    opennsl_l3_route_t* route,
    void* userData) {
  BcmWarmBootCache* cache = static_cast<BcmWarmBootCache*>(userData);
  bool isIPv6 = route->l3a_flags & OPENNSL_L3_IP6;
  auto ip = isIPv6 ? IPAddress::fromBinary(ByteRange(
                         route->l3a_ip6_net, sizeof(route->l3a_ip6_net)))
                   : IPAddress::fromLongHBO(route->l3a_subnet);
  auto mask = isIPv6 ? IPAddress::fromBinary(ByteRange(
                           route->l3a_ip6_mask, sizeof(route->l3a_ip6_mask)))
                     : IPAddress::fromLongHBO(route->l3a_ip_mask);
  if (cache->getHw()->getPlatform()->canUseHostTableForHostRoutes() &&
      ((isIPv6 && mask == getFullMaskIPv6Address()) ||
       (!isIPv6 && mask == getFullMaskIPv4Address()))) {
    // This is a host route.
    cache->vrfAndIP2Route_[make_pair(route->l3a_vrf, ip)] = *route;
    XLOG(DBG3) << "Adding host route found in route table. vrf: "
               << route->l3a_vrf << " ip: " << ip << " mask: " << mask;
  } else {
    // Other routes that cannot be put into host table / CAM.
    cache->vrfPrefix2Route_[make_tuple(route->l3a_vrf, ip, mask)] = *route;
    XLOG(DBG3) << "In vrf : " << route->l3a_vrf << " adding route for : " << ip
               << " mask: " << mask;
  }
  return 0;
}

int BcmWarmBootCache::ecmpEgressTraversalCallback(
    int /*unit*/,
    opennsl_l3_egress_ecmp_t* ecmp,
    int intfCount,
    opennsl_if_t* intfArray,
    void* userData) {
  BcmWarmBootCache* cache = static_cast<BcmWarmBootCache*>(userData);
  EgressIds egressIds;
  // Rather than using the egressId in the intfArray we use the
  // egressIds that we dumped as part of the warm boot state. IntfArray
  // does not include any egressIds that go over the ports that may be
  // down while the warm boot state we dumped does
  try {
    egressIds = cache->getPathsForEcmp(ecmp->ecmp_intf);
  } catch (const FbossError& ex) {
    // There was a bug in SDK where sometimes we got callback with invalid
    // ecmp id with zero number of interfaces. This happened for double wide
    // ECMP entries (when two "words" are used to represent one ECMP entry).
    // For example, if the entries were 200256 and 200258, we got callback
    // for 200257 also with zero interfaces associated with it. If this is
    // the case, we skip this entry.
    //
    // We can also get intfCount of zero with valid ecmp entry (when all the
    // links associated with egress of the ecmp are down. But in this case,
    // cache->getPathsForEcmp() call above should return a valid set of
    // egressIds.
    if (intfCount == 0) {
      return 0;
    }
    throw ex;
  }
  EgressIds egressIdsInHw;
  egressIdsInHw = cache->toEgressIds(intfArray, intfCount);
  XLOG(DBG1) << "ignoring paths for ecmp egress " << ecmp->ecmp_intf
             << " gotten from hardware: " << toEgressIdsStr(egressIdsInHw);

  CHECK(egressIds.size() > 0)
      << "There must be at least one egress pointed to by the ecmp egress id: "
      << ecmp->ecmp_intf;
  CHECK(cache->egressIds2Ecmp_.find(egressIds) == cache->egressIds2Ecmp_.end())
      << "Got a duplicated call for ecmp id: " << ecmp->ecmp_intf
      << " referencing: " << toEgressIdsStr(egressIds);
  cache->egressIds2Ecmp_[egressIds] = *ecmp;
  XLOG(DBG1) << "Added ecmp egress id : " << ecmp->ecmp_intf
             << " pointing to : " << toEgressIdsStr(egressIds) << " egress ids";
  return 0;
}

std::string BcmWarmBootCache::toEgressIdsStr(const EgressIds& egressIds) {
  std::stringstream ss;
  int i = 0;
  for (const auto& egressId : egressIds) {
    ss << egressId;
    ss << (++i < egressIds.size() ?  ", " : "");
  }
  return ss.str();
}

void BcmWarmBootCache::clear() {
  // Get rid of all unclaimed entries. The order is important here
  // since we want to delete entries only after there are no more
  // references to them.
  XLOG(DBG1) << "Warm boot: removing unreferenced entries";
  dumpedSwSwitchState_.reset();
  hwSwitchEcmp2EgressIds_.clear();
  // First delete routes (fully qualified and others).
  //
  // Nothing references routes, but routes reference ecmp egress and egress
  // entries which are deleted later
  for (auto vrfPfxAndRoute : vrfPrefix2Route_) {
    XLOG(DBG1) << "Deleting unreferenced route in vrf:"
               << std::get<0>(vrfPfxAndRoute.first)
               << " for prefix : " << std::get<1>(vrfPfxAndRoute.first) << "/"
               << std::get<2>(vrfPfxAndRoute.first);
    auto rv = opennsl_l3_route_delete(hw_->getUnit(), &(vrfPfxAndRoute.second));
    bcmLogFatal(rv, hw_, "failed to delete unreferenced route in vrf:",
        std::get<0>(vrfPfxAndRoute.first) , " for prefix : " ,
        std::get<1>(vrfPfxAndRoute.first) , "/" ,
        std::get<2>(vrfPfxAndRoute.first));
  }
  vrfPrefix2Route_.clear();
  for (auto vrfIPAndRoute: vrfAndIP2Route_) {
    XLOG(DBG1) << "Deleting fully qualified unreferenced route in vrf: "
               << vrfIPAndRoute.first.first
               << " prefix: " << vrfIPAndRoute.first.second;
    auto rv = opennsl_l3_route_delete(hw_->getUnit(), &(vrfIPAndRoute.second));
    bcmLogFatal(rv,
                hw_,
                "failed to delete fully qualified unreferenced route in vrf: ",
                vrfIPAndRoute.first.first,
                " prefix: ",
                vrfIPAndRoute.first.second);
  }
  vrfAndIP2Route_.clear();

  // Delete bcm host entries. Nobody references bcm hosts, but
  // hosts reference egress objects
  for (auto vrfIpAndHost : vrfIp2Host_) {
    XLOG(DBG1) << "Deleting host entry in vrf: " << vrfIpAndHost.first.first
               << " for : " << vrfIpAndHost.first.second;
    auto rv = opennsl_l3_host_delete(hw_->getUnit(), &vrfIpAndHost.second);
    bcmLogFatal(rv, hw_, "failed to delete host entry in vrf: ",
        vrfIpAndHost.first.first, " for : ", vrfIpAndHost.first.second);
  }
  vrfIp2Host_.clear();

  // Both routes and host entries (which have been deleted earlier) can refer
  // to ecmp egress objects.  Ecmp egress objects in turn refer to egress
  // objects which we delete later
  for (auto idsAndEcmp : egressIds2Ecmp_) {
    auto& ecmp = idsAndEcmp.second;
    XLOG(DBG1) << "Deleting ecmp egress object  " << ecmp.ecmp_intf
               << " pointing to : " << toEgressIdsStr(idsAndEcmp.first);
    auto rv = opennsl_l3_egress_ecmp_destroy(hw_->getUnit(), &ecmp);
    bcmLogFatal(rv, hw_, "failed to destroy ecmp egress object :",
        ecmp.ecmp_intf, " referring to ",
        toEgressIdsStr(idsAndEcmp.first));
  }
  egressIds2Ecmp_.clear();

  // Delete bcm egress entries. These are referenced by routes, ecmp egress
  // and host objects all of which we deleted above. Egress objects in turn
  // my point to a interface which we delete later
  for (auto egressIdAndEgress : egressId2Egress_) {
    // This is not used yet
    XLOG(DBG1) << "Deleting egress object: " << egressIdAndEgress.first;
    auto rv = opennsl_l3_egress_destroy(hw_->getUnit(),
                                        egressIdAndEgress.first);
      bcmLogFatal(rv,
                  hw_,
                  "failed to destroy egress object ",
                  egressIdAndEgress.first);
  }
  egressId2Egress_.clear();

  // Delete interfaces
  for (auto vlanMacAndIntf : vlanAndMac2Intf_) {
    XLOG(DBG1) << "Deleting l3 interface for vlan: "
               << vlanMacAndIntf.first.first
               << " and mac : " << vlanMacAndIntf.first.second;
    auto rv = opennsl_l3_intf_delete(hw_->getUnit(), &vlanMacAndIntf.second);
    bcmLogFatal(rv, hw_, "failed to delete l3 interface for vlan: ",
        vlanMacAndIntf.first.first, " and mac : ", vlanMacAndIntf.first.second);
  }
  vlanAndMac2Intf_.clear();
  // Delete stations
  for (auto vlanAndStation : vlan2Station_) {
    XLOG(DBG1) << "Deleting station for vlan : " << vlanAndStation.first;
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
    XLOG(DBG1) << "Deleting vlan : " << vlanItr->first;
    auto rv = opennsl_vlan_destroy(hw_->getUnit(), vlanItr->first);
    bcmLogFatal(rv, hw_, "failed to destroy vlan: ", vlanItr->first);
    vlanItr = vlan2VlanInfo_.erase(vlanItr);
  }

  egressIdsFromBcmHostInWarmBootFile_.clear();
  vrfIp2EgressFromBcmHostInWarmBootFile_.clear();

  // Detach the unclaimed bcm acl stats
  flat_set<BcmAclStatHandle> statsUsed;
  for (auto aclStatItr = aclEntry2AclStat_.begin();
       aclStatItr != aclEntry2AclStat_.end(); ++aclStatItr) {
    auto& aclStatStatus = aclStatItr->second;
    if (!aclStatStatus.claimed) {
      XLOG(DBG1) << "Detaching unclaimed acl_stat=" << aclStatStatus.stat <<
        "from acl=" << aclStatItr->first;
      detachBcmAclStat(aclStatItr->first, aclStatStatus.stat);
    } else {
      statsUsed.insert(aclStatStatus.stat);
    }
  }

  // Delete the unclaimed bcm acl stats
  for (auto statItr : aclEntry2AclStat_) {
    auto statHandle = statItr.second.stat;
    if (statsUsed.find(statHandle) == statsUsed.end()) {
      XLOG(DBG1) << "Deleting unclaimed acl_stat=" << statHandle;
      removeBcmAclStat(statHandle);
      // add the stat to the set to prevent this loop from attempting to
      // delete the same stat twice
      statsUsed.insert(statHandle);
    }
  }
  aclEntry2AclStat_.clear();

  // Delete acls and acl ranges, since acl(field process) doesn't support
  // opennsl, we call BcmAclTable to remove the unclaimed acls
  XLOG(DBG1) << "Unclaimed acl count=" << priority2BcmAclEntryHandle_.size();
  for (auto aclItr: priority2BcmAclEntryHandle_) {
    XLOG(DBG1) << "Deleting unclaimed acl: prio=" << aclItr.first
               << ", handle=" << aclItr.second;
    removeBcmAcl(aclItr.second);
  }
  priority2BcmAclEntryHandle_.clear();
  XLOG(DBG1) << "Unclaimed acl range count="
             << aclRange2BcmAclRangeHandle_.size();
  for (auto aclRangeItr: aclRange2BcmAclRangeHandle_) {
    XLOG(DBG1) << "Deleting unclaimed acl range=" << aclRangeItr.first.str()
               << ", handle=" << aclRangeItr.second.first;
    removeBcmAclRange(aclRangeItr.second.first);
  }
  aclRange2BcmAclRangeHandle_.clear();

  /* remove unclaimed mirrors and mirrored ports/acls, if any */
  removeUnclaimedMirrors();
  ingressQosMaps_.clear();
}

void BcmWarmBootCache::populateRtag7State() {
  auto unit = hw_->getUnit();

  moduleAState_ = BcmRtag7Module::retrieveRtag7ModuleState(
      unit, BcmRtag7Module::kModuleAControl());
  moduleBState_ = BcmRtag7Module::retrieveRtag7ModuleState(
      unit, BcmRtag7Module::kModuleBControl());
}

bool BcmWarmBootCache::unitControlMatches(
    char module,
    opennsl_switch_control_t switchControl,
    int arg) const {
  const BcmRtag7Module::ModuleState* state = nullptr;

  switch (module) {
    case 'A':
      state = &moduleAState_;
      break;
    case 'B':
      state = &moduleBState_;
      break;
    default:
      throw FbossError("Invalid module identifier ", module);
  }

  CHECK(state);
  auto it = state->find(switchControl);

  return it != state->end() && it->second == arg;
}

void BcmWarmBootCache::programmed(
    char module,
    opennsl_switch_control_t switchControl) {
  BcmRtag7Module::ModuleState* state = nullptr;

  switch (module) {
    case 'A':
      state = &moduleAState_;
      break;
    case 'B':
      state = &moduleBState_;
      break;
    default:
      throw FbossError("Invalid module identifier ", module);
  }

  CHECK(state);
  auto numErased = state->erase(switchControl);

  CHECK_EQ(numErased, 1);
}

BcmWarmBootCache::MirrorEgressPath2HandleCitr BcmWarmBootCache::findMirror(
    opennsl_gport_t port,
    const folly::Optional<MirrorTunnel>& tunnel) const {
  return mirrorEgressPath2Handle_.find(std::make_pair(port, tunnel));
}

BcmWarmBootCache::MirrorEgressPath2HandleCitr BcmWarmBootCache::mirrorsBegin()
    const {
  return mirrorEgressPath2Handle_.begin();
}
BcmWarmBootCache::MirrorEgressPath2HandleCitr BcmWarmBootCache::mirrorsEnd()
    const {
  return mirrorEgressPath2Handle_.end();
}

void BcmWarmBootCache::programmedMirror(MirrorEgressPath2HandleCitr itr) {
  const auto key = itr->first;
  const auto port = key.first;
  const auto& tunnel = key.second;
  if (tunnel) {
    XLOG(DBG1) << "Programmed ERSPAN mirror egressing through: " << port
               << " with "
               << "proto=" << tunnel->greProtocol
               << "source ip=" << tunnel->srcIp.str()
               << "source mac=" << tunnel->srcMac.toString()
               << "destination ip=" << tunnel->dstIp.str()
               << "destination mac=" << tunnel->dstMac.toString()
               << ", removing from warm boot cache";
  } else {
    XLOG(DBG1) << "Programmed SPAN mirror egressing through: " << port
               << ", removing from warm boot cache";
  }
  mirrorEgressPath2Handle_.erase(itr);
}

BcmWarmBootCache::MirroredPort2HandleCitr BcmWarmBootCache::mirroredPortsBegin()
    const {
  return mirroredPort2Handle_.begin();
}

BcmWarmBootCache::MirroredPort2HandleCitr BcmWarmBootCache::mirroredPortsEnd()
    const {
  return mirroredPort2Handle_.end();
}

BcmWarmBootCache::MirroredPort2HandleCitr BcmWarmBootCache::findMirroredPort(
    opennsl_gport_t port,
    MirrorDirection direction) const {
  return mirroredPort2Handle_.find(std::make_pair(port, direction));
}

void BcmWarmBootCache::programmedMirroredPort(MirroredPort2HandleCitr itr) {
  mirroredPort2Handle_.erase(itr);
}

BcmWarmBootCache::MirroredAcl2HandleCitr BcmWarmBootCache::mirroredAclsBegin()
    const {
  return mirroredAcl2Handle_.begin();
}

BcmWarmBootCache::MirroredAcl2HandleCitr BcmWarmBootCache::mirroredAclsEnd()
    const {
  return mirroredAcl2Handle_.end();
}

BcmWarmBootCache::MirroredAcl2HandleCitr BcmWarmBootCache::findMirroredAcl(
    BcmAclEntryHandle entry,
    MirrorDirection direction) const {
  return mirroredAcl2Handle_.find(std::make_pair(entry, direction));
}

void BcmWarmBootCache::programmedMirroredAcl(MirroredAcl2HandleCitr itr) {
  mirroredAcl2Handle_.erase(itr);
}

void BcmWarmBootCache::removeUnclaimedMirrors() {
  XLOG(DBG1) << "Unclaimed mirrored port count=" << mirroredPort2Handle_.size()
             << ", unclaimed mirrored acl count=" << mirroredAcl2Handle_.size()
             << ", unclaimed mirror count=" << mirrorEgressPath2Handle_.size();
  std::for_each(
      mirroredPort2Handle_.begin(),
      mirroredPort2Handle_.end(),
      [this](const auto& mirroredPort2Handle) {
        this->stopUnclaimedPortMirroring(
            mirroredPort2Handle.first.first,
            mirroredPort2Handle.first.second,
            mirroredPort2Handle.second);
      });

  std::for_each(
      mirroredAcl2Handle_.begin(),
      mirroredAcl2Handle_.end(),
      [this](const auto& mirroredAcl2Handle) {
        this->stopUnclaimedAclMirroring(
            mirroredAcl2Handle.first.first,
            mirroredAcl2Handle.first.second,
            mirroredAcl2Handle.second);
      });

  std::for_each(
      mirrorEgressPath2Handle_.begin(),
      mirrorEgressPath2Handle_.end(),
      [this](const auto& mirrorEgressPath2Handle) {
        this->removeUnclaimedMirror(mirrorEgressPath2Handle.second);
      });
}

void BcmWarmBootCache::reconstructPortMirrors(
    std::shared_ptr<SwitchState>* state) {
  auto* ports = (*state)->getPorts()->modify(state);
  for (const auto& cachedPort : *dumpedSwSwitchState_->getPorts()) {
    auto id = cachedPort->getID();
    auto port = ports->getPort(id);
    if (cachedPort->getIngressMirror()) {
      port->setIngressMirror(cachedPort->getIngressMirror());
    }
    if (cachedPort->getEgressMirror()) {
      port->setEgressMirror(cachedPort->getEgressMirror());
    }
  }
}

BcmWarmBootCache::IngressQosMapsItr BcmWarmBootCache::findIngressQosMap(
    const std::set<QosRule>& qosRules) {
  return std::find_if(
      ingressQosMaps_.begin(),
      ingressQosMaps_.end(),
      [&](const std::unique_ptr<BcmQosMap>& qosMap) -> bool {
        return qosMap->rulesMatch(qosRules);
      });
}

void BcmWarmBootCache::programmed(IngressQosMapsItr itr) {
  XLOG(DBG1) << "Programmed QosMap, removing from warm boot cache.";
  ingressQosMaps_.erase(itr);
}
}}
